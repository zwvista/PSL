#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 6/Free Planks

    Summary
    Nail slavery

    Description
    1. Locate some pieces of wood (Planks).
    2. Planks are areas of exactly three cells and can be straight or angled.
    3. Each Plank contains one nail.
    4. After finding all the Planks, it must be possible to move each piece
       by one cell in at least one direction.
*/

namespace puzzles::FreePlanks{

#define PUZ_SPACE        ' '
#define PUZ_NAIL         'N'

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

const vector<vector<Position>> planks = {
    // L
    {{0, 0}, {0, 1}, {1, 0}},
    {{0, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}},
    {{0, 1}, {1, 0}, {1, 1}},
    // I
    {{0, 0}, {1, 0}, {2, 0}},
    {{0, 0}, {0, 1}, {0, 2}},
};

struct puz_plank
{
    vector<Position> m_offset;
    vector<Position> m_horz_walls, m_vert_walls;
};

typedef pair<Position, int> puz_lit;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_plank> m_planks;
    map<Position, vector<puz_lit>> m_nail2lits;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_planks(planks.size())
{
    for (int i = 0; i < m_planks.size(); ++i) {
        auto& t = m_planks[i];
        t.m_offset = planks[i];
        for (auto& p : t.m_offset)
            for (int k = 0; k < 4; ++k)
                if (boost::algorithm::none_of_equal(t.m_offset, p + offset[k]))
                    (k % 2 == 0 ? t.m_horz_walls : t.m_vert_walls).push_back(p + offset2[k]);
    }

    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch == PUZ_NAIL)
                m_nail2lits[{r, c}];
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < m_planks.size(); ++i) {
                auto& t = m_planks[i];
                vector<Position> rng;
                for (auto& os : t.m_offset) {
                    auto p2 = p + os;
                    if (!is_valid(p2)) {
                        rng.clear();
                        break;
                    }
                    if (m_nail2lits.contains(p2))
                        rng.push_back(p2);
                }
                if (rng.size() == 1)
                    m_nail2lits.at(rng[0]).emplace_back(p, i);
            }
        }
}

struct puz_state
{
    puz_state() {}
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
    bool make_move2(const Position& p, int n);
    bool can_move_planks() const;
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
    vector<puz_lit> m_lits;
    set<Position> m_horz_walls, m_vert_walls;
    string m_cells;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto&& [p, lits] : g.m_nail2lits)
        for (int i = 0; i < lits.size(); ++i)
            m_matches[p].push_back(i);
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perms = kv.second;
        auto& lits = m_game->m_nail2lits.at(p);

        boost::remove_erase_if(perms, [&](int id) {
            auto& lit = lits[id];
            auto& rng = m_game->m_planks[lit.second].m_offset;
            return boost::algorithm::any_of(rng, [&](const Position& os) {
                return cells(lit.first + os) != PUZ_SPACE;
            });
        });

        if (!init)
            switch (perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perms[0]) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::can_move_planks() const
{
    return boost::algorithm::all_of(m_lits, [&](const puz_lit& lit) {
        auto& p = lit.first;
        auto& rng = m_game->m_planks[lit.second].m_offset;
        char ch = cells(p + rng[0]);
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p2 = p + os;
            return boost::algorithm::all_of(rng, [&](const Position& os2) {
                auto p3 = p2 + os2;
                if (!is_valid(p3))
                    return false;
                char ch2 = cells(p3);
                return ch2 == PUZ_SPACE || ch == ch2;
            });
        });
    });
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& lit = m_game->m_nail2lits.at(p)[n];
    auto& p2 = lit.first;
    auto& t = m_game->m_planks[lit.second];

    for (auto& os : t.m_horz_walls)
        m_horz_walls.insert(p2 + os);
    for (auto& os : t.m_vert_walls)
        m_vert_walls.insert(p2 + os);
    for (auto& os : t.m_offset)
        cells(p2 + os) = m_ch;
    m_lits.emplace_back(p2, lit.second);

    ++m_ch, ++m_distance;
    m_matches.erase(p);
    
    return can_move_planks();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FreePlanks()
{
    using namespace puzzles::FreePlanks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FreePlanks.xml", "Puzzles/FreePlanks.txt", solution_format::GOAL_STATE_ONLY);
}
