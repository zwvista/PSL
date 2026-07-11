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
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_FRONT_SLASH = '/';
constexpr auto PUZ_BACK_SLASH = '\\';

constexpr auto offset = Position::Directions4;
constexpr auto offset2 = Position::WallsOffset4;

using puz_laser_dot = pair<Position, int>;
constexpr puz_laser_dot mirror_dots[][4] = {
    // front slash '/'
    { {{0, 1}, 1}, {{-1, 0}, 0}, {{0, -1}, 3}, {{1, 0}, 2} },
    // back slash  '\'
    { {{0, -1}, 3}, {{1, 0}, 2}, {{0, 1}, 1}, {{-1, 0}, 0} },
};

struct puz_laser
{
    vector<puz_laser_dot> m_start_end;
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
    map<puz_laser_dot, puz_laser_dot> m_dot2dot;
    vector<char> m_laser_turns;
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
    
    auto f = [&](int r, int c, int i) {
        Position p(r, c);
        string_view str = strs[r * 2];
        int c2 = c * 2 + (c == m_sidelen - 1 ? 1 : 0);
        char ch1 = str[c2], ch2 = str[c2 + 1];
        if (ch1 == PUZ_SPACE)
            cells(p) = PUZ_BOUNDARY;
        else {
            cells(p) = ch1;
            m_dot2dot[{p, i}] = {p + offset[i], i};
            auto& [v, n] = m_letter2laser[ch1];
            v.emplace_back(p, i);
            n = ch2 - '0';
        }
    };
    for (int i = 0; i < m_sidelen; ++i)
        f(0, i, 2), f(m_sidelen - 1, i, 0), f(i, 0, 1), f(i, m_sidelen - 1, 3);

    for (int r = 1; r < m_sidelen - 1; ++r)
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            for (int i = 0; i < 4; ++i)
                m_dot2dot[{p, i}] = {p + offset[i], i};
        }

    for (auto& [letter, laser] : m_letter2laser) {
        m_laser_turns.push_back(letter);
        boost::sort(laser.m_start_end);
    }
    boost::sort(m_laser_turns, [&](char letter1, char letter2) {
        auto g = [&](char letter) {
            return pair(m_letter2laser.at(letter).m_number, letter);
        };
        return g(letter1) < g(letter2);
    });
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_letter2num, m_dots) < tie(x.m_cells, x.m_letter2num, x.m_dots);
    }
    void make_move_laser();
    void make_move_mirror(int n, char ch2);
    bool make_move_end();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) +
        boost::accumulate(m_letter2num, 0, [&](int acc, const pair<const char, int>& kv) {
            return acc + kv.second * sidelen();
        }) + (m_dots.empty() ? sidelen() : sidelen() - m_dots.size());
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<puz_laser_dot, puz_laser_dot> m_dot2dot;
    map<char, int> m_letter2num;
    vector<puz_laser_dot> m_dots;
    char m_current_letter;
    puz_laser_dot m_current_dot;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_dot2dot(g.m_dot2dot)
, m_current_letter(g.m_laser_turns[0])
, m_current_dot(g.m_letter2laser.at(m_current_letter).m_start_end[0])
{
    for (auto& [letter, laser] : g.m_letter2laser)
        m_letter2num[letter] = laser.m_number;
}

inline bool is_slash_char(char ch) { return ch == PUZ_FRONT_SLASH || ch == PUZ_BACK_SLASH; }
inline bool is_end_char(char ch) { return isupper(ch) || ch == PUZ_BOUNDARY; }

void puz_state::make_move_laser()
{
    auto h = get_heuristic();
    auto dt = m_current_dot;
    for (;;) {
        dt = m_dot2dot.at(dt);
        char ch = cells(dt.first);
        if (ch == PUZ_SPACE || is_slash_char(ch) || is_end_char(ch))
            m_dots.push_back(dt);
        if (ch != PUZ_SPACE) {
            m_distance = h - get_heuristic();
            break;
        }
    }
}

void puz_state::make_move_mirror(int n, char ch2)
{
    auto h = get_heuristic();
    for (int i = 0; i < n; ++i)
        cells(m_dots[i].first) = PUZ_EMPTY;
    auto& p = m_dots[n].first;
    if (char& ch = cells(p); ch == PUZ_SPACE) {
        auto& area = m_game->m_areas[m_game->m_pos2area.at(p)];
        for (auto& p2 : area)
            cells(p2) = PUZ_EMPTY;

        for (auto it = m_dot2dot.begin(); it != m_dot2dot.end();)
            if (it->first.first == p)
                it = m_dot2dot.erase(it);
            else
                it++;
        auto& dots = mirror_dots[(ch = ch2) == PUZ_FRONT_SLASH ? 0 : 1];
        for (int i = 0; i < 4; ++i) {
            auto& [dp, d] = dots[i];
            m_dot2dot[{p, i}] = {p + dp, d};
        }
    }
    m_current_dot.first = p;
    m_letter2num.at(m_current_letter)--;
    m_dots.clear();
    m_distance = h - get_heuristic();
}

bool puz_state::make_move_end()
{
    auto h = get_heuristic();
    auto& p = m_dots.back().first;
    if (p != m_game->m_letter2laser.at(m_current_letter).m_start_end[1].first)
        return false;
    m_letter2num.erase(m_current_letter);
    if (!m_letter2num.empty()) {
        m_current_letter = *next(boost::find(m_game->m_laser_turns, m_current_letter));
        m_current_dot = m_game->m_letter2laser.at(m_current_letter).m_start_end[0];
    }
    m_dots.clear();
    m_distance = h - get_heuristic();
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_dots.empty())
        children.emplace_back(*this).make_move_laser();
    else {
        int n = m_letter2num.at(m_current_letter);
        if (n == 0) {
            if (!children.emplace_back(*this).make_move_end())
                children.pop_back();
        } else
            for (int i = 0; i < m_dots.size(); ++i) {
                char ch = cells(m_dots[i].first);
                if (is_end_char(ch))
                    break;
                auto v = is_slash_char(ch) ? vector{ch} : vector{PUZ_FRONT_SLASH, PUZ_BACK_SLASH};
                for (char ch2 : v)
                    children.emplace_back(*this).make_move_mirror(i, ch2);
            }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << ch;
            if (ch == PUZ_EMPTY)
                out << PUZ_EMPTY;
            else
                out << m_game->m_letter2laser.at(ch).m_number;
        }
    };
    f(0);
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
        }
        println(out);
    }
    f(sidelen() - 1);
    return out;
}

}

void solve_puz_MirrorsExtended()
{
    using namespace puzzles::MirrorsExtended;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MirrorsExtended.xml", "Puzzles/MirrorsExtended.txt", solution_format::GOAL_STATE_ONLY);
}
