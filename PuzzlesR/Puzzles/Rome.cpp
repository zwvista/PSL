#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 2/Rome

    Summary
    All roads lead to ...

    Description
    1. All the roads lead to Rome.
    2. Hence you should fill the remaining spaces with arrows and in the
       end, starting at any tile and following the arrows, you should get
       at the Rome icon.
    3. Arrows in an area should all be different, i.e. there can't be two
       similar arrows in an area.
*/

namespace puzzles::Rome{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ROME = 'R';

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::WallsOffset4;

constexpr string_view tool_dirs = "^>v<";

using puz_cell = vector<char>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    vector<Position> m_romes;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_horz_walls, * m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
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
            if (ch != PUZ_ROME)
                rng.insert(p);
            else
                m_romes.push_back(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_areas.emplace_back(smoves.begin(), smoves.end());
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    const puz_cell& cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    puz_cell& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, char ch);
    bool check_lead_to_rome() const;
    bool check_arrows();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return sidelen() * sidelen() - m_finished.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_cell> m_cells;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& cl = cells(p);
            if (char ch = g.cells(p); ch != PUZ_SPACE)
                cl.push_back(ch);
            else
                for (int i = 0; i < 4; ++i)
                    if (auto p2 = p + offset[i]; is_valid(p2))
                        cl.push_back(tool_dirs[i]);
        }
    m_finished.insert_range(m_game->m_romes);
    check_arrows();
}

bool puz_state::check_arrows()
{
    for (;;) {
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                if (m_finished.contains(p)) continue;
                auto& cl = cells(p);
                if (cl.size() != 1) continue;
                newly_finished.insert(p);
            }
        
        if (newly_finished.empty())
            return true;

        auto f = [&](const Position& p, char ch) {
            auto& cl = cells(p);
            if (cl.size() == 1) return true;
            boost::remove_erase(cl, ch);
            // 3. Arrows in an area should all be different, i.e. there can't be two
            //    similar arrows in an area.
            set<char> chars;
            int n = 0;
            auto& area = m_game->m_areas[m_game->m_pos2area.at(p)];
            for (auto& p2 : area)
                if (auto& cl2 = cells(p2); cl2.size() == 1)
                    chars.insert(cl2[0]), n++;
            return chars.size() == n;
        };
        for (auto& p : newly_finished) {
            char ch = cells(p)[0];
            auto& area = m_game->m_areas[m_game->m_pos2area.at(p)];
            for (auto& p2 : area)
                if (p2 != p && !f(p2, ch))
                    return false;
            if (int d = tool_dirs.find(ch);
                !f(p + offset[d], tool_dirs[(d + 2) % 4]))
                return false;
        }
        m_finished.insert_range(newly_finished);
        m_distance += newly_finished.size();
    }
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
        if (auto p2 = *this + offset[i]; m_state->is_valid(p2))
            if (auto& cl = m_state->cells(p2);
                boost::algorithm::any_of_equal(cl, tool_dirs[(i + 2) % 4]))
                children.emplace_back(*this).make_move(p2);
}

// 1. All the roads lead to Rome.
// 2. Hence you should fill the remaining spaces with arrows and in the
//    end, starting at any tile and following the arrows, you should get
//    at the Rome icon.
bool puz_state::check_lead_to_rome() const
{
    set<Position> rng;
    for (auto& p : m_game->m_romes) {
        auto smoves = puz_move_generator<puz_state3>::gen_moves({this, p});
        rng.insert_range(smoves);
    }
    return rng.size() == sidelen() * sidelen();
}

bool puz_state::make_move(int i, char ch)
{
    m_distance = 1;
    m_cells[i] = {ch};
    return check_arrows() && check_lead_to_rome();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int i = boost::min_element(m_cells, [](const puz_cell& cl1, const puz_cell& cl2) {
        auto f = [](const puz_cell& cl) {
            int sz = cl.size();
            return sz == 1 ? 100 : sz;
        };
        return f(cl1) < f(cl2);
    }) - m_cells.begin();
    auto& cl = m_cells[i];
    for (char ch : cl)
        if (!children.emplace_back(*this).make_move(i, ch))
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
            out << cells(p)[0];
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Rome()
{
    using namespace puzzles::Rome;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Rome.xml", "Puzzles/Rome.txt", solution_format::GOAL_STATE_ONLY);
}
