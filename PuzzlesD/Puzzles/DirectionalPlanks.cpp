#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Directional Planks

    Summary
    Can't move

    Description
    1. Divide the board in areas of three tiles (planks).
    2. Each plank contains one number and the number tells you how many
       directions the Plank can move, when the board is completed.
*/

namespace puzzles::DirectionalPlanks{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

const vector<vector<Position>> planks_offset = {
    // L
    {{0, 0}, {0, 1}, {1, 0}},
    {{0, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}},
    {{0, 1}, {1, 0}, {1, 1}},
    // I
    {{0, 0}, {1, 0}, {2, 0}},
    {{0, 0}, {0, 1}, {0, 2}},
};

using puz_plank = pair<Position, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, pair<int, vector<puz_plank>>> m_pos2info;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_pos2info[{r, c}].first = ch - '0';
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < planks_offset.size(); ++i) {
                vector<Position> rng;
                for (auto& os : planks_offset[i]) {
                    auto p2 = p + os;
                    if (!is_valid(p2)) {
                        rng.clear();
                        break;
                    }
                    if (m_pos2info.contains(p2))
                        rng.push_back(p2);
                }
                if (rng.size() == 1)
                    m_pos2info.at(rng[0]).second.emplace_back(p, i);
            }
        }
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
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    bool check_move_planks() const;
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    map<Position, puz_plank> m_pos2planks;
    string m_cells;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& [p, info] : g.m_pos2info)
        for (int i = 0; i < info.second.size(); ++i)
            m_matches[p].push_back(i);
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perms] : m_matches) {
        auto& [num, planks] = m_game->m_pos2info.at(p);

        boost::remove_erase_if(perms, [&](int id) {
            auto& [p2, index] = planks[id];
            auto& rng = planks_offset[index];
            return boost::algorithm::any_of(rng, [&](const Position& os) {
                return cells(p2 + os) != PUZ_SPACE;
            });
        });

        if (!init)
            switch (perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perms[0]), 1;
            }
    }
    return check_move_planks() ? 2 : 0;
}

// 2. Each plank contains one number and the number tells you how many
// directions the Plank can move, when the board is completed.
bool puz_state::check_move_planks() const
{
    for (auto& [pn, plank] : m_pos2planks) {
        int num = m_game->m_pos2info.at(pn).first;
        auto& [p, index] = plank;
        auto& rng = planks_offset[index];
        char ch = cells(p + rng[0]);
        int n = boost::accumulate(offset, 0, [&](int acc, const Position& os) {
            auto p2 = p + os; 
            return acc + (boost::algorithm::all_of(rng, [&](const Position& os2) {
                auto p3 = p2 + os2;
                if (!is_valid(p3))
                    return false;
                char ch2 = cells(p3);
                return ch2 == PUZ_SPACE || ch == ch2;
            }) ? 1 : 0);
        });
        if (m_matches.empty() && n != num || n < num)
            return false;
    }
    return true;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [p2, index] = m_game->m_pos2info.at(p).second[n];
    for (auto& os : planks_offset[index])
        cells(p2 + os) = m_ch;
    m_pos2planks[p] = {p2, index};

    ++m_ch, ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perms] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perms) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (!is_valid(p2) || cells(p) != cells(p2))
                    walls.insert(p_wall);
            }
        }

    for (int r = 0;; ++r) {
        // draw horizontal walls
        for (int c = 0; c < sidelen(); ++c)
            out << (horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_DirectionalPlanks()
{
    using namespace puzzles::DirectionalPlanks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DirectionalPlanks.xml", "Puzzles/DirectionalPlanks.txt", solution_format::GOAL_STATE_ONLY);
}
