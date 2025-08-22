#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Numeric Paranoia
*/

namespace puzzles::numeric_paranoia{

constexpr array<Position, 4> offset = Position::Directions4;
constexpr string_view dirs = "^>v<";

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    vector<Position> m_nums;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0, n = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c, ++n) {
            char& ch = m_cells[n];
            if (ch != ' ') {
                int n = ch - 'A';
                if (n >= m_nums.size())
                    m_nums.resize(n + 1);
                m_nums[n] = Position(r, c);
            }
            if (ch == 'A')
                ch = '@';
        }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_game(&g), m_head(g.m_nums[0])
        , m_next(1), m_move(0) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool is_unvisited(char ch) const {return ch == ' ' || isupper(ch);}
    int num_unvisited() const {
        int n = 0;
        for (size_t i = 0; i < m_cells.length(); ++i)
            if (is_unvisited(m_cells[i]))
                ++n;
        return n;
    }
    bool make_move(int i);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return num_unvisited();}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    Position m_head;
    int m_next;
    char m_move;
};

bool puz_state::make_move(int i)
{
    m_head += offset[i];
    if (!is_valid(m_head)) return false;
    char& ch = cells(m_head);
    if (!is_unvisited(ch)) return false;
    if (isupper(ch)) {
        if (ch - 'A' != m_next)
            return false;
        if (m_next == m_game->m_nums.size() - 1 && num_unvisited() != 1)
            return false;
        ++m_next;
    }

    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            if (p == m_head || !is_unvisited(cells(p))) continue;
            bool reachable = false;
            for (int i = 0; i < 4; ++i) {
                Position p2 = p + offset[i];
                if (!is_valid(p2)) continue;
                if (p2 == m_head || is_unvisited(cells(p2))) {
                    reachable = true;
                    break;
                }
            }
            if (!reachable)
                return false;
        }

    ch = m_move = dirs[i];
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i)
        if (children.push_back(*this); !children.back().make_move(i))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_numeric_paranoia()
{
    using namespace puzzles::numeric_paranoia;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/numeric_paranoia.xml", "Puzzles/numeric_paranoia.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
