#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/Sentinels

    Summary
    This time it's one Garden and many Towers

    Description
    1. On the Board there are a few sentinels. These sentinels are marked with
       a number.
    2. The number tells you how many tiles that Sentinel can control (see) from
       there vertically and horizontally. This includes the tile where he is
       located.
    3. You must put Towers on the Boards in accordance with these hints, keeping
       in mind that a Tower blocks the Sentinel View.
    4. The restrictions are that there must be a single continuous Garden, and
       two Towers can't touch horizontally or vertically.
    5. Towers can't go over numbered squares. But numbered squares don't block
       Sentinel View.
*/

namespace puzzles::Sentinels{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SENTINEL = 'S';
constexpr auto PUZ_TOWER = 'T';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_SENTINEL);
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, const vector<int>& perm);
    bool make_move2(const Position& p, const vector<int>& perm);
    void make_move3(const Position& p, const vector<int>& perm, int i, bool stopped);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
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
: m_cells(g.m_cells), m_game(&g)
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
                else if (ch == PUZ_EMPTY || ch == PUZ_SENTINEL)
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
                return make_move2(p, perms.front()) ? 1 : 0;
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
            make_move3(p, perm, i, perm[i] == n);
        }
    return 1;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s);

    int sidelen() const { return m_state->sidelen(); }
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
    int i = boost::find_if(s.m_cells, [](char ch) {
        return ch != PUZ_BOUNDARY && ch != PUZ_TOWER;
    }) - s.m_cells.begin();
    make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (ch != PUZ_BOUNDARY && ch != PUZ_TOWER) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

void puz_state::make_move3(const Position& p, const vector<int>& perm, int i, bool stopped)
{
    auto& os = offset[i];
    int n = perm[i];
    auto p2 = p + os;
    for (int j = 0; j < n; ++j) {
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
        p2 += os;
    }
    if (char& ch = cells(p2); stopped && ch == PUZ_SPACE) {
        // we choose to stop here, so it must be in other direction
        ch = PUZ_TOWER;
        // Two Towers can't touch horizontally or vertically
        for (auto& os2 : offset) {
            char& ch2 = cells(p2 + os2);
            if (ch2 == PUZ_SPACE)
                ch2 = PUZ_EMPTY;
        }
    }
}

bool puz_state::make_move2(const Position& p, const vector<int>& perm)
{
    for (int i = 0; i < 4; ++i)
        make_move3(p, perm, i, true);

    ++m_distance;
    m_matches.erase(p);

    // There must be a single continuous garden
    auto smoves = puz_move_generator<puz_state2>::gen_moves(*this);
    return smoves.size() == boost::count_if(m_cells, [](char ch) {
        return ch != PUZ_BOUNDARY && ch != PUZ_TOWER;
    });
}

bool puz_state::make_move(const Position& p, const vector<int>& perm)
{
    m_distance = 0;
    if (!make_move2(p, perm))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perms] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<vector<int>>>& kv1,
        const pair<const Position, vector<vector<int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& perm : perms) {
        children.push_back(*this);
        if (!children.back().make_move(p, perm))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SENTINEL)
                out << format("{:2}", m_game->m_pos2num.at(p));
            else
                out << ' ' << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Sentinels()
{
    using namespace puzzles::Sentinels;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Sentinels.xml", "Puzzles/Sentinels.txt", solution_format::GOAL_STATE_ONLY);
}
