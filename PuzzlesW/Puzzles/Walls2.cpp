#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 5/Walls

    Summary
    Find the maze of Bricks

    Description
    1. In Walls you must fill the board with straight horizontal and
       vertical lines (walls) that stem from each number.
    2. The number itself tells you the total length of Wall segments
       connected to it.
    3. Wall pieces have two ways to be put, horizontally or vertically.
    4. Not every wall piece must be connected with a number, but the
       board must be filled with wall pieces.
*/

namespace puzzles::Walls2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_NUMBER = 'N';
constexpr auto PUZ_HORZ = '-';
constexpr auto PUZ_VERT = '|';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_move
{
    Position m_p;
    char m_char;
};

struct puz_perm
{
    Position m_p_hint;
    vector<int> m_nums = vector<int>(4);
    vector<puz_move> m_moves;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    vector<puz_perm> m_perms;
    map<Position, vector<int>> m_pos2perm_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_NUMBER);
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, sum] : m_pos2num) {
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            bool is_horz = i % 2 == 1;
            auto& os = offset[i];
            int n = 0;
            auto& nums = dir_nums[i];
            for (auto p2 = p + os; n <= sum; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    // we can stop here
                    nums.push_back(n++);
                else {
                    // we have to stop here
                    nums.push_back(n);
                    break;
                }
            }
        }

        // Compute the total length of the wall segments connected to the number
        // Record the combination if the sum is equal to the given number
        vector<vector<int>> nums2D;
        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum)
                            nums2D.push_back({n0, n1, n2, n3});

        for (auto& v : nums2D) {
            int n = m_perms.size();
            vector<puz_move> moves;
            m_pos2perm_ids[p].push_back(n);
            for (int i = 0; i < 4; ++i) {
                bool is_horz = i % 2 == 1;
                auto& os = offset[i];
                auto p2 = p + os;
                for (int j = 0; j < v[i]; ++j, p2 += os) {
                    moves.emplace_back(p2, is_horz ? PUZ_HORZ : PUZ_VERT);
                    m_pos2perm_ids[p2].push_back(n);
                }
                // 4. Not every wall piece must be connected with a number
                if (cells(p2) == PUZ_SPACE) {
                    moves.emplace_back(p2, is_horz ? PUZ_VERT : PUZ_HORZ);
                    m_pos2perm_ids[p2].push_back(n);
                }
            }
            m_perms.emplace_back(p, v, moves);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i);
    void make_move2(int i);
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
    // key: the position of the number
    // value.elem: index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
, m_matches(g.m_pos2perm_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    set<Position> spaces;
    for (auto& [_1, perm_ids] : m_matches) {
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& [_2, _3, moves] = m_game->m_perms[id];
            return !boost::algorithm::all_of(moves, [&](const puz_move& move) {
                auto& [p2, ch2] = move;
                char ch = cells(p2);
                return ch == PUZ_SPACE || ch == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i)
{
    auto& [p, _1, moves] = m_game->m_perms[i];
    for (auto& [p2, ch2] : moves) {
        cells(p2) = ch2;
        if (m_matches.contains(p2))
            ++m_distance, m_matches.erase(p2);
    }
    if (m_matches.contains(p))
        ++m_distance, m_matches.erase(p);
}

bool puz_state::make_move(int i)
{
    m_distance = 0;
    make_move2(i);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_NUMBER)
                out << format("{:2}", m_game->m_pos2num.at(p));
            else
                out << ' ' << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Walls2()
{
    using namespace puzzles::Walls2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Walls.xml", "Puzzles/Walls2.txt", solution_format::GOAL_STATE_ONLY);
}
