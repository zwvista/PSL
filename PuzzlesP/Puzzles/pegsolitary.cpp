#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    pegsolitary
*/

namespace puzzles::pegsolitary{

constexpr auto PUZ_PEG = '$';
constexpr auto PUZ_HOLE = ' ';
constexpr auto PUZ_NONE = '#';

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
    {-1, -1},
    {1, 1},
    {-1, 1},
    {1, -1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_cells = string(rows() * cols(), ' ');
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_NONE);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_NONE);

    int n = cols();
    for (int r = 1; r < rows() - 1; ++r, n += cols()) {
        string_view str = strs[r - 1];
        m_cells[n] = m_cells[n + cols() - 1] = PUZ_NONE;
        for (int c = 1; c < cols() - 1; ++c)
            m_cells[n + c] = str[c - 1];
    }
}

struct puz_step
{
    Position m_p1, m_p2;
    puz_step(const Position& p1, const Position& p2)
        : m_p1(p1), m_p2(p2) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: {} => {}\n", mi.m_p1, mi.m_p2);
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    void make_move(const Position& p1, const Position& p2, const Position& p3) {
        cells(p1) = cells(p2) = PUZ_HOLE;
        cells(p3) = PUZ_PEG;
        m_move = puz_step(p1, p3);
    }

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 1;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        int n = 0;
        for (size_t i = 0; i < m_cells.length(); i++)
            if (m_cells[i] == PUZ_PEG)
                ++n;
        return n;
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    boost::optional<puz_step> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int r = 1; r < rows() - 1; ++r)
        for (int c = 1; c < cols() - 1; ++c) {
            Position p1(r, c);
            if (cells(p1) != PUZ_PEG) continue;
            for (int i = 0; i < 8; ++i) {
                Position p2 = p1 + offset[i];
                Position p3 = p2 + offset[i];
                if (cells(p2) == PUZ_PEG && cells(p3) == PUZ_HOLE) {
                        children.push_back(*this);
                        children.back().make_move(p1, p2, p3);
                }
            }
        }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 1; r < rows() -1 ; ++r) {
        for (int c = 0; c < cols() - 1; ++c)
            out << cells({r, c}) << " ";
        println(out);
    }
    return out;
}

}

void solve_puz_pegsolitary()
{
    using namespace puzzles::pegsolitary;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/pegsolitary.xml", "Puzzles/pegsolitary.txt");
}
