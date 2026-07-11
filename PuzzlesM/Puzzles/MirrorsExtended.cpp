#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 4/Mirrors, extended

    Summary
    with lasers, of course

    Description
    1. On the border there are some lasers, marked with the letter and number.
    2. The letter tells you where that laser beam will start and end (it is paired with the same
       letter somewhere else).
    3. The number tells you how many mirrors the laser beam will bounce off before reaching the
       other letter.
    4. Each area contains one mirror.
    5. Each mirror reflects at least one laser beam.
*/

namespace puzzles::MirrorsExtended{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SLASH = '/';
constexpr auto PUZ_BACKSLASH = '\\';

constexpr auto offset = Position::Directions4;
constexpr auto offset2 = Position::WallsOffset4;

struct puz_laser
{
    vector<Position> m_start_end;
    int m_number;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    map<char, puz_laser> m_letter2laser;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
        if (!walls.contains(p_wall))
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2 + 1)
, m_cells(m_sidelen * m_sidelen, PUZ_SPACE)
{
    set<Position> rng;
    for (int r = 1;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2 - 1];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen - 1) break;
        string_view str_v = strs[r * 2];
        for (int c = 1;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen - 1) break;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        m_areas.push_back({smoves.begin(), smoves.end()});
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }
    
    auto f = [&](int r, int c) {
        Position p(r, c);
        string_view str = strs[r * 2];
        int c2 = c * 2 + (c == m_sidelen - 1 ? 1 : 0);
        char ch1 = str[c2], ch2 = str[c2 + 1];
        if (ch1 == PUZ_SPACE)
            cells(p) = PUZ_EMPTY;
        else {
            cells(p) = ch1;
            auto& [v, n] = m_letter2laser[ch1];
            v.push_back(p);
            n = ch2 - '0';
        }
    };
    for (int i = 0; i < m_sidelen; ++i)
        f(0, i), f(m_sidelen - 1, i), f(i, 0), f(i, m_sidelen - 1);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int move_id);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
{
}

bool puz_state::make_move(int move_id)
{
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
//    for (auto& move_id : move_ids)
//        if (!children.emplace_back(*this).make_move(move_id))
//            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
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
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_MirrorsExtended()
{
    using namespace puzzles::MirrorsExtended;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MirrorsExtended.xml", "Puzzles/MirrorsExtended.txt", solution_format::GOAL_STATE_ONLY);
}
