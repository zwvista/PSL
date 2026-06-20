#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 6/Hedge Maze

    Summary
    Wendy ?

    Description
    1. Fill some of the empty areas with hedges, thus forming a maze.
    2. The maze should be one tile wide. It can branch itself, but not close in a loop.
    3. There should be a path between the two gates. This path should pass on
       all the steps and not on any fountain.
    4. On the board there can't be a 2x2 area all made of hedges or all without hedges (empty).
    5. Tiles with any icon count as empty and cannot be filled with hedges.
*/

namespace puzzles::HedgeMaze{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_HEDGE = 'H';
constexpr auto PUZ_GATE = 'G';
constexpr auto PUZ_STEP = 'S';
constexpr auto PUZ_FOUNTAIN = 'F';

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::WallsOffset4;
constexpr array<Position, 4> offset3 = Position::Square2x2Offset;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    vector<Position> m_gates, m_steps;
    set<int> m_iconless_areas;
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
, m_sidelen(strs.size() / 2)
{
    set<Position> rng;
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
            char ch = str_v[c * 2 + 1];
            m_cells.push_back(ch);
            if (ch == PUZ_GATE)
                m_gates.push_back(p);
            else if (ch == PUZ_STEP)
                m_steps.push_back(p);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        m_areas.emplace_back(smoves.begin(), smoves.end());
        bool iconless = true;
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (cells(p) != PUZ_SPACE)
                iconless = false;
        }
        if (iconless)
            m_iconless_areas.insert(n);
        else
            // 5. Tiles with any icon count as empty and cannot be filled with hedges.
            for (auto& p : smoves)
                if (char& ch = cells(p); ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int n, char ch);
    void make_move2(int n, char ch);
    bool is_interconnected() const;
    bool check_2x2();
    bool check_branch() const;
    bool check_path() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_iconless_areas.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    set<int> m_iconless_areas;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_iconless_areas(g.m_iconless_areas)
{
    check_2x2();
}

// 4. On the board there can't be a 2x2 area all made of hedges or all without hedges (empty).
// 5. Tiles with any icon count as empty and cannot be filled with hedges.
bool puz_state::check_2x2()
{
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            Position p(r, c);
            set<int> hedge_areas, icon_areas, empty_areas, space_areas;
            for (auto& os : offset3) {
                auto p2 = p + os;
                int id = m_game->m_pos2area.at(p2);
                char ch = cells(p2);
                (ch == PUZ_HEDGE ? hedge_areas : !m_game->m_iconless_areas.contains(id) ? icon_areas : ch == PUZ_EMPTY ? empty_areas : space_areas).insert(id);
            }
            if (hedge_areas.empty() || icon_areas.empty() && empty_areas.empty()) {
                if (space_areas.empty())
                    return false;
                if (space_areas.size() == 1) {
                    char ch = hedge_areas.empty() ? PUZ_HEDGE : PUZ_EMPTY;
                    int n = *space_areas.begin();
                    make_move2(n, ch);
                }
            }
        }
    return true;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (int i = 0; i < 4; ++i)
        if (auto p2 = *this + offset[i];
            m_state->is_valid(p2) && m_state->cells(p2) != PUZ_HEDGE)
            children.emplace_back(*this).make_move(p2);
}

inline bool is_maze(char ch) { return ch != PUZ_SPACE && ch != PUZ_HEDGE; }

bool puz_state::is_interconnected() const
{
    int i = boost::find_if(m_cells, is_maze) - m_cells.begin();
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return is_maze(cells(p));
    }) == boost::count_if(m_cells, is_maze);
}

void puz_state::make_move2(int n, char ch)
{
    auto& area = m_game->m_areas[n];
    for (auto& p : area)
        cells(p) = ch;
    m_iconless_areas.erase(n);
    ++m_distance;
}

// 2. The maze should be one tile wide. It can branch itself, but not close in a loop.
bool puz_state::check_branch() const
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (char ch = cells(p); is_maze(ch))
                rng.insert(p);
        }
    while (!rng.empty()) {
        set<Position> moves;
        // https://stackoverflow.com/questions/78166176/how-can-i-write-an-inline-recursive-lambda-in-c
        auto dfs = [&](this const auto &self, const Position& p, int n) {
            if (!moves.insert(p).second)
                return false;
            for (int i = 0; i < 4; ++i) {
                if (i == n) continue;
                auto p2 = p + offset[i];
                if (!rng.contains(p2)) continue;
                if (!self(p2, (i + 2) % 4))
                    return false;
            }
            return true;
        };
        if (!dfs(*rng.begin(), -1))
            return false;
        for (auto& p : moves)
            rng.erase(p);
    }
    return true;
}

// 3. There should be a path between the two gates. This path should pass on
//    all the steps and not on any fountain.
bool puz_state::check_path() const
{
    if (m_game->m_gates.size() != 2)
        return false;
    auto &gate1 = m_game->m_gates[0], &gate2 = m_game->m_gates[1];

    set<Position> moves;
    auto dfs = [&](this const auto &self, const Position& p, int n) {
        if (char ch = cells(p); ch == PUZ_HEDGE || ch == PUZ_FOUNTAIN)
            return false;
        moves.insert(p);
        if (p == gate2)
            return true;
        for (int i = 0; i < 4; ++i) {
            if (i == n) continue;
            auto p2 = p + offset[i];
            if (!is_valid(p2)) continue;
            if (self(p2, (i + 2) % 4))
                return true;
        }
        moves.erase(p);
        return false;
    };
    return dfs(gate1, -1) && boost::algorithm::all_of(m_game->m_steps, [&](const Position& p) {
        return moves.contains(p);
    });
}

bool puz_state::make_move(int n, char ch)
{
    m_distance = 0;
    make_move2(n, ch);
    return check_2x2() && is_interconnected() && check_branch() && (!is_goal_state() || check_path());
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int n = *boost::max_element(m_iconless_areas, [&](int id1, int id2) {
        return m_game->m_areas[id1].size() < m_game->m_areas[id2].size();
    });
    for (char ch : {PUZ_HEDGE, PUZ_EMPTY})
        if (!children.emplace_back(*this).make_move(n, ch))
            children.pop_back();
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

void solve_puz_HedgeMaze()
{
    using namespace puzzles::HedgeMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HedgeMaze.xml", "Puzzles/HedgeMaze.txt", solution_format::GOAL_STATE_ONLY);
}
