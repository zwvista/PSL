#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Pond camping

    Summary
    Splash!

    Description
    1. The numbers are Ponds. From each Pond you can have a hike of that many
       tiles as the number marked on it.
*/

namespace puzzles::PondCamping2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FOREST = '=';
constexpr auto PUZ_POND = 'P';

constexpr auto offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_FOREST);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_FOREST);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_POND);
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        m_cells.push_back(PUZ_FOREST);
    }
    m_cells.append(m_sidelen, PUZ_FOREST);
}

struct puz_area
{
    set<Position> m_inner, m_outer;
    bool operator<(const puz_area& x) const {
        return m_outer.size() < x.m_outer.size();
    }
};

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(Position p);
    bool adjust_area(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_pos2area, 0, [&](int acc, const pair<const Position, puz_area>& kv) {
            return acc + m_game->m_pos2num.at(kv.first) - kv.second.m_inner.size();
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, puz_area> m_pos2area;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [p, _1] : g.m_pos2num)
        m_pos2area[p].m_inner.insert(p);
    adjust_area(true);
}

bool puz_state::adjust_area(bool init)
{
    for (auto it = m_pos2area.begin(); it != m_pos2area.end();) {
        auto& [pnum, area] = *it;
        int num = m_game->m_pos2num.at(pnum);
        auto& [inner, outer] = area;
        bool extending = false;
        do {
            extending = false;
            outer.clear();
            for (auto& p : inner)
                for (auto& os : offset) {
                    auto p2 = p + os;
                    if (char ch = cells(p2); !inner.contains(p2) && ch == PUZ_EMPTY)
                        inner.insert(p2), extending = true;
                    else if (ch == PUZ_SPACE)
                        outer.insert(p2);
                }
        } while (extending);

        if (!init) {
            if (int sz = inner.size() - 1; sz > num)
                return false;
            else if (sz == num) {
                for (auto& p : outer)
                    cells(p) = PUZ_FOREST;
                it = m_pos2area.erase(it);
            }
            else if (outer.empty())
                return false;
            else
                it++;
        } else
            it++;
    }
    return true;
}

bool puz_state::make_move(Position p)
{
    auto h = get_heuristic();
    cells(p) = PUZ_EMPTY;
    bool b = adjust_area(false);
    if (b)
        m_distance = h - get_heuristic();
    return b;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, area] = *boost::min_element(m_pos2area, [&](
        const pair<const Position, puz_area>& kv1,
        const pair<const Position, puz_area>& kv2) {
        auto f = [&](const pair<const Position, puz_area>& kv) {
            auto& [inner, outer] = kv.second;
            return pair(outer.size(), m_game->m_pos2num.at(kv.first) - inner.size());
        };
        return f(kv1) < f(kv2);
    });
    for (auto& p : area.m_outer)
        if (!children.emplace_back(*this).make_move(p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it != m_game->m_pos2num.end())
                out << format("{:<2}", it->second);
            else
                out << cells(p) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PondCamping2()
{
    using namespace puzzles::PondCamping2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PondCamping.xml", "Puzzles/PondCamping2.txt", solution_format::GOAL_STATE_ONLY);
}
