#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 3/Pouring Water

    Summary
    Communicating Vessels

    Description
    1. The board represents some communicating vessels.
    2. You have to fill some water in it, considering that water pours down
       and levels itself like in reality.
    3. Areas of the same level which are horizontally connected will have
       the same water level.
    4. The numbers on the border show you how many tiles of each row and
       column are filled.
*/

namespace puzzles::PouringWater{

constexpr auto PUZ_WATER = 'X';
constexpr auto PUZ_SPACE = ' ';

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_water
{
    set<Position> m_rng;
    map<int, int> m_area2num;
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<int, int> m_area2num;
    vector<puz_water> m_waters;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls), m_p_start(p_start) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
    Position m_p_start;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall) && p.first >= m_p_start.first) {
            children.emplace_back(*this).make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 - 1)
{
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
        }
    }

    auto f = [&](int area_id, char ch) {
        if (ch != ' ')
            m_area2num[area_id] = ch - '0';
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1][m_sidelen * 2 + 1]);
        f(i + m_sidelen, strs[m_sidelen * 2 + 1][i * 2 + 1]);
    }

    for (int r = m_sidelen - 1; r >= 0; --r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (boost::algorithm::any_of(m_waters, [&](const puz_water& o) {
                return o.m_rng.contains(p);
            }))
                continue;
            auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, p});
            puz_water o;
            for (auto& p2 : smoves) {
                o.m_rng.insert(p2);
                auto f = [&](int i) {
                    if (m_area2num.contains(i))
                        ++o.m_area2num[i];
                };
                f(p2.first);
                f(p2.second + m_sidelen);
            }
            if (boost::algorithm::all_of(o.m_area2num, [&](const pair<const int, int>& kv) {
                return kv.second <= m_area2num.at(kv.first);
            }))
                m_waters.push_back(o);
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int n);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_area2num, 0,
            [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_area2num;
    // value.elem: index of the water
    vector<int> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_area2num(g.m_area2num)
{
    m_matches.resize(g.m_waters.size());
    boost::iota(m_matches, 0);
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    auto& o = m_game->m_waters[n];
    for (auto& p : o.m_rng) {
        auto f = [&](int area_id) {
            if (m_area2num.contains(area_id))
                --m_area2num[area_id], ++m_distance;
        };
        f(p.first);
        f(p.second + sidelen());
        cells(p) = PUZ_WATER;
    }
    
    boost::remove_erase_if(m_matches, [&](int id) {
        auto& o = m_game->m_waters[id];
        return boost::algorithm::any_of(o.m_area2num, [&](const pair<const int, int>& kv) {
            return kv.second > m_area2num.at(kv.first);
        });
    });

    return is_goal_state() || !m_matches.empty();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int n : m_matches)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int area_id) {
        int cnt = 0;
        for (int i = 0; i < sidelen(); ++i)
            if (cells(area_id < sidelen() ? Position(area_id, i) : Position(i, area_id - sidelen())) == PUZ_WATER)
                ++cnt;
        out << (area_id < sidelen() ? format("{:<2}", cnt) : format("{:2}", cnt));
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) {
                f(r);
                break;
            }
            out << cells(p);
        }
        println(out);
    }
    for (int c = 0; c < sidelen(); ++c)
        f(c + sidelen());
    println(out);
    return out;
}

}

void solve_puz_PouringWater()
{
    using namespace puzzles::PouringWater;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PouringWater.xml", "Puzzles/PouringWater.txt", solution_format::GOAL_STATE_ONLY);
}
