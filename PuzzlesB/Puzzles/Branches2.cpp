#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 2/Branches

    Summary
    Fill the board with Branches departing from the numbers

    Description
    1. In Branches you must fill the board with straight horizontal and
       vertical lines(Branches) that stem from each number.
    2. The number itself tells you how many tiles its Branches fill up.
       The tile with the number doesn't count.
    3. There can't be blank tiles and Branches can't overlap, nor run over
       the numbers. Moreover Branches must be in a single straight line
       and can't make corners.
*/

namespace puzzles::Branches{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_NUMBER = 'N';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr string_view str_branch = "|-|-^>v<";

using puz_move = pair<Position, char>;

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
    string m_cells;
    vector<puz_perm> m_perms;
    map<Position, vector<int>> m_pos2perm_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
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
                m_cells.push_back(ch);
            else {
                m_cells.push_back(PUZ_NUMBER);
                int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_pos2num[{r, c}] = n;
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, sum] : m_pos2num) {
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            bool is_horz = i % 2 == 1;
            auto& os = offset[i];
            int n = 0;
            auto& nums = dir_nums[i];
            for (auto p2 = p + os; n <= sum; p2 += os)
                if (char ch = cells(p2); ch == PUZ_SPACE)
                    // we can stop here
                    nums.push_back(n++);
                else {
                    // we have to stop here
                    nums.push_back(n);
                    break;
                }
        }

        // Compute the total length of the branches connected to the number
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
                auto& os = offset[i];
                auto p2 = p + os;
                if (v[i] == 0) continue;
                for (int j = 1; j < v[i]; ++j, p2 += os) {
                    moves.emplace_back(p2, str_branch[i]);
                    m_pos2perm_ids[p2].push_back(n);
                }
                // branch head
                moves.emplace_back(p2, str_branch[i + 4]);
                m_pos2perm_ids[p2].push_back(n);
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

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: index of the permutations
    map<Position, vector<int>> m_matches;
    set<Position> m_used_hints;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches(g.m_pos2perm_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, perm_ids] : m_matches) {
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& [p, _2, moves] = m_game->m_perms[id];
            return m_used_hints.contains(p) ||
                !boost::algorithm::all_of(moves, [&](const puz_move& move) {
                    auto& [p2, _3] = move;
                    return cells(p2) == PUZ_SPACE;
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
    for (auto& [p2, ch2] : moves)
        if (cells(p2) = ch2; m_matches.erase(p2))
            ++m_distance;
    if (m_matches.erase(p))
        ++m_distance;
    m_used_hints.insert(p);
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

    for (auto& n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
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

void solve_puz_Branches()
{
    using namespace puzzles::Branches;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Branches.xml", "Puzzles/Branches.txt", solution_format::GOAL_STATE_ONLY);
}
