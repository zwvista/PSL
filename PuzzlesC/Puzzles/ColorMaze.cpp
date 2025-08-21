#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: ColorMaze

    Summary
    Water Sort puzzle
      
    Description
    1. Now try to pour the water in different colors and sort the water in the same color into the same bottles.
    2. Tap a bottle first, then tap another bottle, and pour water from the first bottle to the second one.
    3. You can pour when two bottles have the same water color on the top, and there's enough space for the second bottle to be poured.
    4. Each bottle could only hold a certain amount of water. If it's full, no more could be poured.
*/

namespace puzzles::ColorMaze{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_PAINTED = '.';
constexpr auto PUZ_BALL = 'O';
constexpr auto PUZ_BLOCK = '#';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const string_view dirs = "^>v<";

struct puz_game
{
    string m_id;
    Position m_size;
    Position m_ball;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

struct puz_move : pair<int, int> { using pair::pair; };

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() + 2, strs[0].length() + 2))
{
    m_cells.append(cols(), PUZ_BLOCK);
    for (int r = 1; r < rows() - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK);
        for (int c = 1; c < cols() - 1; ++c) {
            switch (Position p(r, c); char ch = str[c - 1]) {
            case PUZ_SPACE:
            case PUZ_BLOCK:
                m_cells.push_back(ch);
                break;
            case PUZ_BALL:
                m_cells.push_back(PUZ_PAINTED);
                m_ball = p;
                break;
            }
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(cols(), PUZ_BLOCK);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_ball) < tie(x.m_cells, m_ball); 
    }
    void make_move(int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const { if (m_move) out << *m_move; }
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    Position m_ball;
    string m_cells;
    optional<char> m_move;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_ball(g.m_ball)
{
}

void puz_state::make_move(int n)
{
    auto& os = offset[n];
    for (auto p = m_ball + os;; p += os) {
        char& ch = cells(p);
        if (ch == PUZ_SPACE)
            ++m_distance, ch = PUZ_PAINTED, m_ball = p;
        else if (ch == PUZ_PAINTED)
            m_ball = p;
        else
            break;
    }
    m_move = dirs[n];
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++)
        if (auto p = m_ball + offset[i]; cells(p) != PUZ_BLOCK) {
            children.push_back(*this);
            children.back().make_move(i);
        }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    println(out);
    return out;
}

}

void solve_puz_ColorMaze()
{
    using namespace puzzles::ColorMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ColorMaze.xml", "Puzzles/ColorMaze.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
