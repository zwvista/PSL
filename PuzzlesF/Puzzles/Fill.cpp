#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Fill

    Summary
    One stroke drawing

    Description
    Fill all squares in one stroke, starting from the marked one.
*/

namespace puzzles::Fill{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_AREA = 'O';
constexpr auto PUZ_OBJECT = '@';

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
    string m_cells;
    set<Position> m_area;
    Position m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c)
            switch (Position p(r, c); str[c]) {
            case PUZ_AREA: m_area.insert(p); break;
            case PUZ_OBJECT: m_start = p; break;
            }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g)
        : string(g.m_cells), m_game(&g), m_area(g.m_area), m_p(g.m_start) { }
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * cols() + p.second]; }
    bool make_move(int i, Position p2);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return m_area.size();}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    set<Position> m_area;
    Position m_p;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state& s)
        : m_state(&s) { make_move(*s.m_area.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; m_state->m_area.contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

bool puz_state::make_move(int n, Position p2)
{
    cells(m_p) = dirs[n];
    m_area.erase(m_p = p2);
    cells(m_p) = PUZ_OBJECT;
    if (is_goal_state())
        return true;
    auto smoves = puz_move_generator<puz_state2>::gen_moves({*this});
    return smoves.size() == m_area.size();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i)
        if (auto p2 = m_p + offset[i]; m_area.contains(p2)) {
            children.push_back(*this);
            if (!children.back().make_move(i, p2))
                children.pop_back();
        }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            out << cells({r, c}) << (p == m_game->m_start ? '*' : ' ');
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Fill()
{
    using namespace puzzles::Fill;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Fill.xml", "Puzzles/Fill.txt", solution_format::GOAL_STATE_ONLY);
}
