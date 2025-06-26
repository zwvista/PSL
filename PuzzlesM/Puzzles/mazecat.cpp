#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::mazecat{

constexpr auto PUZ_MALE = 'M';
constexpr auto PUZ_FEMALE = 'F';
constexpr auto PUZ_WALL = '#';
constexpr auto PUZ_THORN = '!';
constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    string m_id;
    Position m_size;
    array<Position, 2> m_cats;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() + 2, strs[0].length() + 2))
{
    m_cells = string(rows() * cols(), PUZ_SPACE);
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_WALL);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_WALL);

    for (int r = 1, n = cols(); r < rows() - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells[n++] = PUZ_WALL;
        for (int c = 1; c < cols() - 1; ++c) {
            char ch = str[c - 1];
            switch(ch) {
            case PUZ_MALE:
                m_cats[0] = Position(r, c);
                ch = PUZ_SPACE;
                break;
            case PUZ_FEMALE:
                m_cats[1] = Position(r, c);
                ch = PUZ_SPACE;
                break;
            }
            m_cells[n++] = ch;
        }
        m_cells[n++] = PUZ_WALL;
    }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_game(&g), m_cats(g.m_cats) {}
    char cells(const Position& p) const {return m_game->cells(p);}
    bool operator<(const puz_state& x) const {
        return m_cats < x.m_cats ||
            m_cats == x.m_cats && m_move < x.m_move;
    }
    bool make_move(int n, int dir, int step);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 1;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_cats[0], m_cats[1]);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(!m_move.empty()) out << m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    array<Position, 2> m_cats;
    string m_move;
};

bool puz_state::make_move(int n, int dir, int step)
{
    static string_view moves = "lrud";
    const Position& os = offset[dir];
    Position p = m_cats[n] + os;
    if (p == m_cats[1 - n] || cells(p) != PUZ_SPACE)
        return false;
    m_cats[n] = p;
     p = m_cats[1 - n] - os;
    if (cells(p) == PUZ_THORN) 
        return false;
    if (cells(p) == PUZ_SPACE && p != m_cats[n])
        m_cats[1 - n] = p;
    m_move = format("{}{}{}", n == 0 ? PUZ_MALE : PUZ_FEMALE, moves[dir], step);
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int n = 0; n < 2; ++n)
        for (int dir = 0; dir < 4; ++dir)
            for (int step = 1; ; ++step) {
                children.push_back(step == 1 ? *this : children.back());
                if (!children.back().make_move(n, dir, step)) {
                    children.pop_back();
                    break;
                }
            }
}

ostream& puz_state::dump(ostream& out) const
{
    if (!m_move.empty())
        out << "move: " << m_move << endl;
    for (int r = 1; r < m_game->rows() - 1; ++r) {
        for (int c = 1; c < m_game->cols() - 1; ++c) {
            Position p(r, c);
            out << (p == m_cats[0] ? PUZ_MALE:
                p == m_cats[1] ? PUZ_FEMALE : cells(p));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_mazecat()
{
    using namespace puzzles::mazecat;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/mazecat.xml", "Puzzles/mazecat.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
