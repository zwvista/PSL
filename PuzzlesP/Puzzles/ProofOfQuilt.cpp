#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Proof of Quilt

    Summary
    Quilt the board, following the hints

    Description
     1. The goal is to place triangles in some cells in the end generating a pattern
        similar to a Quilt.
     2. The numbered tiles tell you how many triangles share an edge with it,
        horizontally and vertically
     3, For example, if a tile says 4, it has triangles all around it.
     4. If a tile says 1, it has only one triangle somewhere.
     5. Some tiles will remain blank and will form, along with the triangles, rectangles
        and squares.
     6. These can be tilted by 45 degrees.
     7. Some other tiles are filled but contain no number. These and the hints are
        the only tiles that can be completely filled.
     8. Rectangles or squares can't touch orthogonally, but can touch diagonally
*/

namespace puzzles::ProofOfQuilt{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_WHITE = 'W';
constexpr auto PUZ_FILLED = 15;
constexpr auto PUZ_UPPER_LEFT = 9;
constexpr auto PUZ_UPPER_RIGHT = 3;
constexpr auto PUZ_LOWER_LEFT = 12;
constexpr auto PUZ_LOWER_RIGHT = 6;
constexpr auto PUZ_UNKNOWN = -1;
    
constexpr auto offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    map<Position, int> m_pos2num;

    puz_game(const vector<string>& strs, const xml_node& level);
    int cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_FILLED);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        m_cells.push_back(PUZ_FILLED);
        string_view str = strs[r - 1];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_UNKNOWN);
            else {
                m_cells.push_back(PUZ_FILLED);
                if (ch != PUZ_WHITE)
                    m_pos2num[{r, c}] = ch - '0';
            }
        m_cells.push_back(PUZ_FILLED);
    }
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_FILLED);

    // Find all tilted quilts
    // A tilted quilt has a circumscribed square
    for (int i = 2; i <= m_sidelen; ++i) {
        for (int r = 1; r <= m_sidelen - i + 1; ++r) {
            for (int c = 1; c <= m_sidelen - i + 1; ++c) {
                for (int j = 1; j <= i - 1; ++j) {
                    int k = i - j;
                    for (int dr = 0; dr < i; ++dr)
                        for (int dc = 0; dc < i; ++dc) {

                        }
                }
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells;
    }
    bool make_move(int n);
    bool check_cuts();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_cuts_needed; }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    vector<int> m_move_ids;
    int m_cuts_needed;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
}

bool puz_state::make_move(int n)
{
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int n : m_move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << format("{:<2}", (ch == PUZ_SPACE ? '.' : ch));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ProofOfQuilt()
{
    using namespace puzzles::ProofOfQuilt;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ProofOfQuilt.xml", "Puzzles/ProofOfQuilt.txt", solution_format::GOAL_STATE_ONLY);
}
