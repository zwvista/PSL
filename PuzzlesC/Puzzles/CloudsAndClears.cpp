#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/Clouds and Clears

    Summary
    Holes in the sky

    Description
    1. Paint the clouds according to the numbers.
    2. Each cloud or empty Sky patch contains a single number that is the extension of the region
       itself.
    3. On a region there can be other numbers. These will indicate how many empty (non-cloud) tiles
       around it (diagonal too) including itself.
*/

namespace puzzles::CloudsAndClears{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_CLOUD = 'C';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // o
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

struct puz_move
{
    int m_dir; // direction to the next stone
    Position m_to;
    set<Position> m_on_path; // stones on the path
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    map<int, vector<string>> m_num2perms;
    map<Position, vector<set<Position>>> m_pos2patches;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

// first: true if the patch is a cloud, false if it is an empty sky patch
// second: the positions of the patch
struct puz_state2 : pair<bool, set<Position>>
{
    puz_state2(const puz_game& game, int num, bool is_cloud, const Position& p)
        : m_game(&game), m_num(num) { first = is_cloud, make_move(p, num, -1); }

    bool is_goal_state() const { return second.size() == m_num; }
    bool make_move(const Position& p, int num, int perm_id);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_num;
    set<Position> m_clouds, m_empties;
};

bool puz_state2::make_move(const Position& p, int num, int perm_id)
{
    second.insert(p);
    if (perm_id != -1) {
    }
    if (is_goal_state())
        for (auto& p : second)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (char ch2 = m_game->cells(p2); ch2 == PUZ_SPACE && !second.contains(p2))
                    (first ? m_empties : m_clouds).insert(p2);
            }
    return true; // move successful
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    //for (auto& p : *this)
    //    for (auto& os : offset) {
    //        auto p2 = p + os;
    //        if (char ch2 = m_game->cells(p2); ch2 == PUZ_SPACE && !contains(p2)) {
    //            children.push_back(*this);
    //            children.back().make_move(p2);
    //        }
    //    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            if (char ch = str[c - 1]; ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
            m_cells.push_back(PUZ_SPACE);
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [_1, num] : m_pos2num) {
        auto& perms = m_num2perms[num];
        if (!perms.empty())
            continue;
        auto perm = string(num, PUZ_EMPTY) + string(9 - num, PUZ_CLOUD);
        do
            perms.push_back(perm);
        while (boost::next_permutation(perm));
    }

    for (auto& [p, num] : m_pos2num) {
        auto& patches = m_pos2patches[p];
        for (int i = 0; i < 2; ++i) {
            puz_state2 sstart(*this, num, i == 0, p);
            list<list<puz_state2>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                for (auto& spath : spaths) {
                    auto& s = spath.back();
                    //patches.push_back({s.begin(), s.end()});
                }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool operator<(const puz_state& x) const {
        return tie(m_stone2move_ids, m_path) < tie(x.m_stone2move_ids, x.m_path);
    }
    bool make_move(const Position& p, int n);

    //solve_puzzle interface
    // 6. The goal is to pick up every stone.
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_stone2move_ids.size(); }
    unsigned int get_distance(const puz_state& child) const {
        return m_stone2move_ids.empty() ? 2 : 1;
    }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_stone2move_ids;
    vector<Position> m_path; // path from the first stone to the current stone
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
}

bool puz_state::make_move(const Position& p, int n)
{
    return true; // move successful
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto f = [&](const Position& p) {
        for (int n : m_stone2move_ids.at(p))
            if (children.push_back(*this); !children.back().make_move(p, n))
                children.pop_back();
    };
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0; r < sidelen(); ++r) {
    //    for (int c = 0; c < sidelen(); ++c)
    //        if (Position p(r, c); !m_game->m_stones.contains(p))
    //            out << ".. ";
    //        else {
    //            int n = boost::find(m_path, p) - m_path.begin() + 1;
    //            out << format("{:2} ", n);
    //        }
    //    println(out);
    //}
    return out;
}

}

void solve_puz_CloudsAndClears()
{
    using namespace puzzles::CloudsAndClears;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CloudsAndClears.xml", "Puzzles/CloudsAndClears.txt", solution_format::GOAL_STATE_ONLY);
}
