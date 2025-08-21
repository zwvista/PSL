#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 17/Coffee And Sugar

    Summary
    How many ?

    Description
    1. Link the Coffee cup with two sugar cubes.
    2. The link should be T-shaped: two sugar cubes must be connected with a straight
       line and the cup forms the middle segment.

    Double Espresso Variant
    3. In some levels (when notified in the top right corner), this variant means you
       can also link two coffees and one sugar. Both configurations are good.
*/

namespace puzzles::CoffeeAndSugar{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_COFFEE = 'C';
constexpr auto PUZ_SUGAR = 'S';
constexpr auto PUZ_INTERSECT = 'O';
constexpr auto PUZ_HORZ = '-';
constexpr auto PUZ_VERT = '|';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

// t1 < t2
// 
// t1 --- t3 --- t2
//        |
//        |
//        t4
// t1
// |
// t3 ---------- t4
// |
// t2
struct puz_link
{
    Position m_t1, m_t2, m_os1;
    Position m_t3, m_t4, m_os2;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_link> m_links;
    vector<Position> m_coffees, m_sugars;
    string m_cells;
    bool m_is_double_espresso_variant;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
, m_is_double_espresso_variant(level.attribute("DoubleEspressoVariant").as_int() == 1)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; ; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            char ch = str[c - 1];
            m_cells.push_back(ch);
            Position p(r * 2 - 1, c * 2 - 1);
            if (ch == PUZ_COFFEE)
                m_coffees.push_back(p);
            else if (ch == PUZ_SUGAR)
                m_sugars.push_back(p);
            if (c == m_sidelen / 2) break;
            m_cells.push_back(PUZ_SPACE);
        }
        m_cells.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_cells.push_back(PUZ_BOUNDARY);
        m_cells.append(m_sidelen - 2, PUZ_SPACE);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    auto f = [&](const vector<Position>& rng1, const vector<Position>& rng2) {
        auto check_line = [&](const Position& p1, const Position& p2, const Position& os) {
            for (auto p = p1 + os; p != p2; p += os)
                if (cells(p) != PUZ_SPACE)
                    return false;
            return true;
        };

        for (int i = 0; i < rng1.size() - 1; ++i) {
            Position t1 = rng1[i];
            for (int j = i + 1; j < rng1.size(); ++j) {
                Position t2 = rng1[j];
                if (t1.first == t2.first) {
                    auto& os1 = offset[1];
                    if (!check_line(t1, t2, os1)) continue;
                    auto rng3 = rng2;
                    boost::remove_erase_if(rng3, [&](const Position& p) {
                        return p.second <= t1.second || p.second >= t2.second;
                    });
                    for (auto& p : rng3) {
                        Position t3(t1.first, p.second), t4(p);
                        auto& os2 = offset[t3.first < t4.first ? 2 : 0];
                        if (check_line(t3, t4, os2))
                            m_links.emplace_back(t1, t2, os1, t3, t4, os2);
                    }
                } else if (t1.second == t2.second) {
                    auto& os1 = offset[2];
                    if (!check_line(t1, t2, os1)) continue;
                    auto rng3 = rng2;
                    boost::remove_erase_if(rng3, [&](const Position& p) {
                        return p.first <= t1.first || p.first >= t2.first;
                    });
                    for (auto& p : rng3) {
                        Position t3(p.first, t1.second), t4(p);
                        auto& os2 = offset[t3.second < t4.second ? 1 : 3];
                        if (check_line(t3, t4, os2))
                            m_links.emplace_back(t1, t2, os1, t3, t4, os2);
                    }
                }
            }
        }
    };
    f(m_sugars, m_coffees);
    if (m_is_double_espresso_variant)
        f(m_coffees, m_sugars);
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
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the island
    // value.elem: the id of link that connects the object at this position
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_links.size(); ++i) {
        auto& [t1, t2, os1, t3, t4, os2] = g.m_links[i];
        m_matches[t1].push_back(i);
        m_matches[t2].push_back(i);
        m_matches[t4].push_back(i);
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perms] : m_matches) {
        boost::remove_erase_if(perms, [&](int id) {
            auto& [t1, t2, os1, t3, t4, os2] = m_game->m_links[id];
            auto check_line = [&](const Position& p1, const Position& p2, const Position& os) {
                for (auto p = p1 + os; p != p2; p += os)
                    if (cells(p) != PUZ_SPACE)
                        return false;
                return true;
            };
            return !m_matches.contains(t1)
                || !m_matches.contains(t2)
                || !m_matches.contains(t4)
                || !check_line(t1, t2, os1)
                || !check_line(t3, t4, os2);
        });

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(perms.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [t1, t2, os1, t3, t4, os2] = m_game->m_links[n];
    bool is_horz = os1 == offset[1];
    for (auto p = t1 + os1; p != t2; p += os1)
        cells(p) = is_horz ? PUZ_HORZ : PUZ_VERT;
    for (auto p = t3 + os2; p != t4; p += os2)
        cells(p) = is_horz ? PUZ_VERT : PUZ_HORZ;
    cells(t3) = PUZ_INTERSECT;

    m_distance += 3;
    m_matches.erase(t1);
    m_matches.erase(t2);
    m_matches.erase(t4);
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
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

    for (int n : perms)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c});
        println(out);
    }
    return out;
}

}

void solve_puz_CoffeeAndSugar()
{
    using namespace puzzles::CoffeeAndSugar;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CoffeeAndSugar.xml", "Puzzles/CoffeeAndSugar.txt", solution_format::GOAL_STATE_ONLY);
}
