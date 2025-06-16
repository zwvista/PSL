#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 11/Wall Sentinels

    Summary
    It's midnight and all is well!

    Description
    1. On the board there is a single continuous castle wall, which you
       must discover.
    2. The numbers on the board represent Sentinels (in a similar way to
       'Sentinels'). The Sentinels can be placed on the Wall or Land.
    3. The number tells you how many tiles that Sentinel can control (see)
       from there vertically and horizontally - of his type of tile.
    4. That means the number of a Land Sentinel indicates how many Land tiles
       are visible from there, up to Wall tiles or the grid border.
    5. That works the opposite way for Wall Sentinels: they control all the
       Wall tiles up to Land tiles or the grid border.
    6. The number includes the tile itself, where the Sentinel is located
       (again, like 'Sentinels').
    7. Lastly there is a single, orthogonally contiguous, Wall and it cannot
       contain 2*2 Wall tiles - just like Nurikabe.
*/

namespace puzzles::WallSentinels{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_LAND = '.';
constexpr auto PUZ_LAND_S = 's';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_WALL_S = 'S';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (auto s = str.substr(c * 2 - 2, 2); s == "  ")
                m_start.push_back(PUZ_SPACE);
            else {
                m_start.push_back(s[0] == PUZ_LAND ? PUZ_LAND_S : PUZ_WALL_S);
                m_pos2num[{r, c}] = isdigit(s[1]) ? s[1] - '0' : s[1] - 'A' + 10;
            }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches); 
    }
    bool make_move_sentinel(const Position& p, const vector<int>& perm);
    bool make_move_sentinel2(const Position& p, const vector<int>& perm);
    bool make_move_sentinel3(const Position& p, const vector<int>& perm, int i, bool stopped);
    bool make_move_space(const Position& p, char ch);
    int find_matches(bool init);
    bool is_continuous() const;
    bool is_valid_square(const Position& p) const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the number that represents the sentinel
    // value.elem: respective numbers of the tiles visible from the position of
    //             the sentinel in all four directions
    map<Position, vector<vector<int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (auto& [p, n] : g.m_pos2num)
        m_matches[p];

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    bool matches_changed = init;
    for (auto& [p, perms] : m_matches) {
        auto perms_old = perms;
        perms.clear();
        bool is_wall = cells(p) == PUZ_WALL_S;

        // Exclude the tile where the sentinel is located
        int sum = m_game->m_pos2num.at(p) - 1;
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            int n = 0;
            auto& nums = dir_nums[i];
            for (auto p2 = p + os; n <= sum; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    // we can stop here
                    nums.push_back(n++);
                else if (!is_wall && (ch == PUZ_LAND || ch == PUZ_LAND_S) ||
                    is_wall && (ch == PUZ_WALL || ch == PUZ_WALL_S))
                    // we cannot stop here
                    ++n;
                else {
                    // we have to stop here
                    nums.push_back(n);
                    break;
                }
            }
        }

        // Compute the total number of the tiles the sentinel can see from the position
        // Record the combination if the sum is equal to the given number
        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum)
                            perms.push_back({n0, n1, n2, n3});

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_sentinel2(p, perms.front()) ? 1 : 0;
            default:
                matches_changed = matches_changed || perms != perms_old;
                break;
            }
    }
    if (!matches_changed)
        return 2;

    for (auto& [p, perms] : m_matches)
        for (int i = 0; i < 4; ++i) {
            auto f = [=](const vector<int>& v1, const vector<int>& v2) {
                return v1[i] < v2[i];
            };
            const auto& perm = *boost::min_element(perms, f);
            int n = boost::max_element(perms, f)->at(i);
            if (!make_move_sentinel3(p, perm, i, perm[i] == n))
                return 0;
        }
    return 1;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng, const Position& p_start) : m_rng(&rng) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

// There is a single, orthogonally contiguous, Wall
bool puz_state::is_continuous() const
{
    set<Position> rng;
    for (int r = 1; r < sidelen(); ++r)
        for (int c = 1; c < sidelen(); ++c) {
            Position p(r, c);
            if (char ch = cells(p);
                ch == PUZ_SPACE || ch == PUZ_WALL || ch == PUZ_WALL_S)
                rng.insert(p);
        }
    
    Position p_start = *boost::find_if(rng, [&](const Position& p) {
        return cells(p) != PUZ_SPACE;
    });

    auto smoves = puz_move_generator<puz_state2>::gen_moves({rng, p_start});
    for (auto& p : smoves)
        rng.erase(p);

    return boost::algorithm::all_of(rng, [&](const Position& p) {
        return cells(p) == PUZ_SPACE;
    });
}

// The wall cannot contain 2*2 wall tiles
bool puz_state::is_valid_square(const Position& p) const
{
    for (int dr = -1; dr <= 0; ++dr)
        for (int dc = -1; dc <= 0; ++dc)
            if (Position p2(p.first + dr, p.second + dc);
                boost::algorithm::all_of(offset2, [&](const Position& os) {
                    char ch = cells(p2 + os);
                    return ch == PUZ_WALL || ch == PUZ_WALL_S;
            }))
                return false;
    return true;
}

bool puz_state::make_move_sentinel3(const Position& p, const vector<int>& perm, int i, bool stopped)
{
    bool is_wall = cells(p) == PUZ_WALL_S;
    auto& os = offset[i];
    auto p2 = p + os;
    for (int j = 0, n = perm[i]; j < n; ++j, p2 += os)
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            if (ch = is_wall ? PUZ_WALL : PUZ_LAND, ++m_distance;
                ch == PUZ_WALL && !is_valid_square(p2))
                return false;

    if (char& ch = cells(p2); stopped && ch == PUZ_SPACE)
        // we choose to stop here, so it must be of other type
        if (ch = is_wall ? PUZ_LAND : PUZ_WALL, ++m_distance;
            ch == PUZ_WALL && !is_valid_square(p2))
            return false;

    return true;
}

bool puz_state::make_move_sentinel2(const Position& p, const vector<int>& perm)
{
    for (int i = 0; i < 4; ++i)
        if (!make_move_sentinel3(p, perm, i, true))
            return false;

    ++m_distance;
    m_matches.erase(p);
    return is_continuous();
}

bool puz_state::make_move_sentinel(const Position& p, const vector<int>& perm)
{
    m_distance = 0;
    if (!make_move_sentinel2(p, perm))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_space(const Position& p, char ch)
{
    cells(p) = ch;
    if (ch == PUZ_WALL && !is_valid_square(p))
        return false;
    m_distance = 1;
    return is_continuous();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, perms] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<vector<int>>>& kv1,
            const pair<const Position, vector<vector<int>>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (auto& perm : perms) {
            children.push_back(*this);
            if (!children.back().make_move_sentinel(p, perm))
                children.pop_back();
        }
    } else {
        int i = m_cells.find(PUZ_SPACE);
        for (char ch : {PUZ_LAND, PUZ_WALL}) {
            children.push_back(*this);
            if (!children.back().make_move_space({i / sidelen(), i % sidelen()}, ch))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_LAND_S || ch == PUZ_WALL_S)
                out  << (ch == PUZ_LAND_S ? PUZ_LAND : PUZ_WALL)
                << format("{:<2}", m_game->m_pos2num.at(p));
            else
                out << ch << "  ";
        }
        println(out);
    }
    return out;
}

}

void solve_puz_WallSentinels()
{
    using namespace puzzles::WallSentinels;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WallSentinels.xml", "Puzzles/WallSentinels.txt", solution_format::GOAL_STATE_ONLY);
}
