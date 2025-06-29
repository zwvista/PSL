#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::escapology{

constexpr auto PUZ_STONE = '#';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BALL = '@';
constexpr auto PUZ_GOAL = '.';
constexpr auto PUZ_BLUE = '!';

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    Position m_size;
    set<Position> m_balls;
    set<Position> m_goals;
    string m_cells;
    string m_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_goal(const Position& p) const {return m_goals.contains(p);}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() + 2, strs[0].length() + 2))
{
    m_cells = string(rows() * cols(), PUZ_SPACE);
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_STONE);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_STONE);

    int n = cols();
    for (int r = 1; r < rows() - 1; ++r, n += cols()) {
        m_cells[n] = m_cells[n + cols() - 1] = PUZ_STONE;
        string_view vstr = strs.at(r - 1);
        for (int c = 1; c < cols() - 1; ++c) {
            Position p(r, c);
            switch(char ch = vstr[c - 1]) {
            case PUZ_BALL:
                m_balls.insert(p); break;
            case PUZ_GOAL:
                m_goals.insert(p); break;
            default: 
                m_cells[n + c] = ch; break;
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_game(&g), m_balls(g.m_balls), m_move(0) {}
    bool operator<(const puz_state& x) const {return m_balls < x.m_balls;}
    char cells(const Position& p) const {return m_game->cells(p);}

    // solve_puzzle interface
    bool is_goal_state() const {return m_balls == m_game->m_goals;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    bool is_ball(const Position& p) const {return m_balls.contains(p);}
    bool make_move(int i);
    const puz_game* m_game = nullptr;
    set<Position> m_balls;
    char m_move;
};

bool puz_state::make_move(int i)
{
    string dirs = "lrud";
    char dir = dirs[i];
    bool can_move = false;
    Position os = offset[i];

    vector<Position> next;

    while (!m_balls.empty()) {
        Position b = *m_balls.begin();
        int n = 1;

        while (m_balls.contains(b - os))
            b -= os;

        for (;;n++) {
            m_balls.erase(b);
            Position p1 = b - os, p2 = b + os;
            char ch1 = cells(p1), ch2 = cells(p2), chb = cells(b);
            if (ch1 == PUZ_BLUE || ch2 == PUZ_BLUE || ch2 == PUZ_STONE ||
                dirs.find(chb) != -1 && chb != dir ||
                dirs.find(ch2) != -1 && ch2 != dir)
                break;
            if (!m_balls.contains(b = p2)) {
                can_move = true;
                break;
            }
        }

        for (int i = 0; i < n; i++, b -= os)
            next.push_back(b);
    }

    if (!can_move) return false;

    m_balls.insert(next.begin(), next.end());
    m_move = dir;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++)
        if (children.push_back(*this); !children.back().make_move(i))
            children.pop_back();
}

unsigned int puz_state::get_heuristic() const
{
    vector<Position> balls(m_balls.begin(), m_balls.end());
    vector<Position> goals(m_game->m_goals.begin(), m_game->m_goals.end());
    int n = m_balls.size();
    vector<int> a(n * n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            a[i * n + j] = manhattan_distance(balls[i], goals[j]);
    boost::nth_element(a, a.begin() + n - 1);
    return a[n - 1];
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 1; ; ++r) {
        for (int c = 1; ; ++c) {
            Position pos(r, c);
            out << (is_ball(pos) ? '@' : 
                m_game->is_goal(pos) ? '.' : m_game->cells(pos));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_escapology()
{
    using namespace puzzles::escapology;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/escapology.xml", "Puzzles/escapology.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
