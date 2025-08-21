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
    2. Each cloud or empty Sky move contains a single number that is the extension of the region
       itself.
    3. On a region there can be other numbers. These will indicate how many empty (non-cloud) tiles
       around it (diagonal too) including itself.
*/

namespace puzzles::CloudsAndClears{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_CLOUD = 'C';

constexpr array<Position, 4> offset = Position::Directions4;

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
    Position m_p_num;
    bool m_is_cloud = false;
    set<Position> m_rng;
    set<Position> m_clouds, m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<int, vector<string>> m_num2perms;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2
{
    puz_state2(const puz_game* game, int num, bool is_cloud, const Position& p)
        : m_game(game), m_num(num), m_is_cloud(is_cloud) { make_move(p, num, -1); }
    bool operator<(const puz_state2& x) const {
        return tie(m_rng, m_clouds, m_empties) < tie(x.m_rng, x.m_clouds, x.m_empties);
    }

    bool is_goal_state() const { return m_rng.size() == m_num; }
    bool make_move(const Position& p, int num, int perm_id);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    int m_num;
    bool m_is_cloud;
    set<Position> m_rng, m_clouds, m_empties;
};

bool puz_state2::make_move(const Position& p, int num, int perm_id)
{
    m_rng.insert(p);
    (m_is_cloud ? m_clouds : m_empties).erase(p);
    if (perm_id != -1) {
        auto& perm = m_game->m_num2perms.at(num)[perm_id];
        for (int i = 0; i < 9; ++i) {
            auto p2 = p + offset2[i];
            bool is_cloud = perm[i] == PUZ_CLOUD;
            if (!m_game->is_valid(p2))
                if (is_cloud)
                    continue;
                else
                    return false;
            bool is_inside = m_rng.contains(p2);
            if (is_inside && is_cloud != m_is_cloud ||
                !is_inside && (is_cloud ? m_empties : m_clouds).contains(p2))
                return false; // invalid move
            if (!is_inside)
                (is_cloud ? m_clouds : m_empties).insert(p2);
        }
    }
    if (is_goal_state())
        for (auto& p2 : m_rng)
            for (auto& os : offset)
                if (auto p3 = p2 + os; m_game->is_valid(p3) && !m_rng.contains(p3))
                    (m_is_cloud ? m_empties : m_clouds).insert(p3);
    return true;
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& p : m_rng)
        for (auto& os : offset)
            if (auto p2 = p + os;
                m_game->is_valid(p2) && !m_rng.contains(p2) &&
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

    for (auto& [p, num] : m_pos2num)
        for (int i = 0; i < 2; ++i) {
            bool is_cloud = i == 0;
            puz_state2 sstart(this, num, is_cloud, p);
            list<list<puz_state2>> spaths;
            if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                for (auto& spath : spaths) {
                    auto& s = spath.back();
                    int n = m_moves.size();
                    m_moves.emplace_back(p, is_cloud, s.m_rng, s.m_clouds, s.m_empties);
                    for (auto& p2 : s.m_rng)
                        m_pos2move_ids[p2].push_back(n);
                }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int move_id);
    void make_move2(int move_id);
    int find_matches(bool init);

    //solve_puzzle interface
    // 6. The goal is to pick up every stone.
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_2, is_cloud, rng, clouds, empties] = m_game->m_moves[id];
            return boost::algorithm::any_of(rng, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != (is_cloud ? PUZ_CLOUD : PUZ_EMPTY);
            }) || boost::algorithm::any_of(clouds, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_CLOUD;
            }) || boost::algorithm::any_of(empties, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_EMPTY;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int move_id)
{
    auto& [_1, is_cloud, rng, clouds, empties] = m_game->m_moves[move_id];
    for (auto& p2 : rng)
        cells(p2) = is_cloud ? PUZ_CLOUD : PUZ_EMPTY, ++m_distance, m_matches.erase(p2);
    for (auto& p2 : clouds)
        cells(p2) = PUZ_CLOUD;
    for (auto& p2 : empties)
        cells(p2) = PUZ_EMPTY;
}

bool puz_state::make_move(int move_id)
{
    m_distance = 0;
    make_move2(move_id);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& move_id : move_ids)
        if (children.push_back(*this); !children.back().make_move(move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << cells(p);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ". ";
            else
                out << it->second << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CloudsAndClears()
{
    using namespace puzzles::CloudsAndClears;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CloudsAndClears.xml", "Puzzles/CloudsAndClears.txt", solution_format::GOAL_STATE_ONLY);
}
