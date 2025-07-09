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

struct puz_patch
{
    set<Position> m_rng;
    bool m_is_cloud = false;
    set<Position> m_clouds, m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<int, vector<string>> m_num2perms;
    map<Position, vector<puz_patch>> m_pos2patches;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, int num, bool is_cloud, const Position& p)
        : m_game(&game), m_num(num), m_is_cloud(is_cloud) { make_move(p, num, -1); }

    bool is_goal_state() const { return size() == m_num; }
    bool make_move(const Position& p, int num, int perm_id);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_num;
    bool m_is_cloud;
    set<Position> m_clouds, m_empties;
};

bool puz_state2::make_move(const Position& p, int num, int perm_id)
{
    insert(p);
    (m_is_cloud ? m_clouds : m_empties).erase(p);
    if (perm_id != -1) {
        auto& perm = m_game->m_num2perms.at(num)[perm_id];
        for (int i = 0; i < 9; ++i) {
            auto p2 = p + offset2[i];
            bool is_inside = contains(p2), is_cloud = perm[i] == PUZ_CLOUD;
            if (is_cloud != (is_inside && m_is_cloud ||
                !is_inside && (m_is_cloud ? m_clouds : m_empties).contains(p2)))
                return false; // invalid move
            if (!is_inside)
                (is_cloud ? m_clouds : m_empties).insert(p2);
        }
    }
    if (is_goal_state())
        for (auto& p2 : *this)
            for (auto& os : offset)
                if (auto p3 = p2 + os; m_game->is_valid(p3) && !contains(p3))
                    (m_is_cloud ? m_empties : m_clouds).insert(p3);
    return true;
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& p : *this)
        for (auto& os : offset)
            if (auto p2 = p + os;
                m_game->is_valid(p2) && !contains(p2) &&
                !(m_is_cloud ? m_empties : m_clouds).contains(p2))
                if (auto it = m_game->m_pos2num.find(p2); it == m_game->m_pos2num.end()) {
                    children.push_back(*this);
                    children.back().make_move(p2, -1, -1);
                } else {
                    int num = it->second;
                    auto& perms = m_game->m_num2perms.at(num);
                    for (int i = 0; i < perms.size(); ++i)
                        if (children.push_back(*this); !children.back().make_move(p2, num, i))
                            children.pop_back();
                }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
    }

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
            bool is_cloud = i == 0;
            puz_state2 sstart(*this, num, is_cloud, p);
            list<list<puz_state2>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                for (auto& spath : spaths) {
                    auto& s = spath.back();
                    patches.push_back({{s.begin(), s.end()}, is_cloud, s.m_clouds, s.m_empties});
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
