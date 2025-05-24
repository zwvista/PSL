#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::twinballs{

enum EDir {mvLeft, mvRight, mvUp, mvDown};

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    Position m_size;
    array<Position, 2> m_balls;
    array<Position, 2> m_goals;
    set<Position> m_horz_wall;
    set<Position> m_vert_wall;
    string m_cells;
    string m_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_horz_wall(const Position& p) const {return m_horz_wall.count(p) != 0;}
    bool is_vert_wall(const Position& p) const {return m_vert_wall.count(p) != 0;}
    bool is_hole(const Position& p) const {return cells(p) == '#';}
    bool is_goal(const Position& p) const {return p == m_goals[0] || p == m_goals[1];}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() / 2 + 2, strs[0].length() / 2 + 2))
{
    m_cells = string(rows() * cols(), ' ');
    fill(m_cells.begin(), m_cells.begin() + cols(), '#');
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), '#');

    int n = cols();
    int ball_count = 0, goal_count = 0;
    for (int r = 1; ; ++r, n+= cols()) {
        const string& hstr = strs.at(2 * r - 2);
        for (size_t i = 0; i < hstr.length(); i++)
            if (hstr[i] == '-')
                m_horz_wall.insert(Position(r, i / 2 + 1));

        if (r == rows() - 1) break;

        m_cells[n] = m_cells[n + cols() - 1] = '#';

        const string& vstr = strs.at(2 * r - 1);
        for (size_t i = 0; i < vstr.length(); i++) {
            Position p(r, i / 2 + 1);
            switch(vstr[i]) {
            case '|': m_vert_wall.insert(p); break;
            case '@': m_balls[ball_count++] = p; break;
            case '.': m_goals[goal_count++] = p; break;
            case '#': m_cells[n + p.second] = '#'; break;
            }
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g)
        : m_game(&g), m_balls(g.m_balls), m_move(0) {}
    bool operator<(const puz_state& x) const {return m_balls < x.m_balls;}
    bool operator==(const puz_state& x) const {return m_balls == x.m_balls;}

    // solve_puzzle interface
    bool is_goal_state() const {return m_balls == m_game->m_goals;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    bool is_ball(const Position& p) const {return p == m_balls[0] || p == m_balls[1];}
    bool make_move(EDir dir);
    const puz_game* m_game = nullptr;
    array<Position, 2> m_balls;
    char m_move;
};

bool puz_state::make_move(EDir dir)
{
    static string_view moves = "lrud";
    bool moved = false;
    for (Position& ball : m_balls) {
        Position p2 = ball + offset[dir];
        bool blocked = 
            dir == mvLeft && m_game->is_vert_wall(ball) ||
            dir == mvRight && m_game->is_vert_wall(p2) ||
            dir == mvUp && m_game->is_horz_wall(ball) ||
            dir == mvDown && m_game->is_horz_wall(p2);
        if (!blocked) {
            if (m_game->is_hole(p2))
                return false;
            ball = p2;
            moved = true;
        }
    }
    if (!moved || m_balls[0] == m_balls[1])
        return false;
    sort(m_balls.begin(), m_balls.end());
    m_move = moves[dir];
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++) {
        children.push_back(*this);
        if (!children.back().make_move(EDir(i)))
            children.pop_back();
    }
}

unsigned int puz_state::get_heuristic() const
{
    static array<int, 4> a;
    for (int i = 0; i < 4; ++i)
        a[i] = manhattan_distance(m_balls[i / 2], m_game->m_goals[i % 2]);
    boost::nth_element(a, a.begin() + 1);
    //boost::sort(a);
    return a[1];
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 1; ; ++r) {
        // draw horz wall
        for (int c = 1; c < m_game->cols() - 1; ++c) {
            Position pos(r, c);
            out << (m_game->is_horz_wall(pos) ? " -" : "  ");
        }
        out << endl;
        if (r == m_game->rows() - 1) break;
        for (int c = 1; ; ++c) {
            Position pos(r, c);
            // draw vert wall
            out << (m_game->is_vert_wall(pos) ? "|" : " ");
            if (c == m_game->cols() - 1) break;
            // draw balls and goals
            out << (is_ball(pos) ? '@' : 
                m_game->is_goal(pos) ? '.' : m_game->cells(pos));
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_twinballs()
{
    using namespace puzzles::twinballs;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/twinballs.xml", "Puzzles/twinballs_a.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>
        ("Puzzles/twinballs.xml", "Puzzles/twinballs_ida.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
