#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Magic Square -Swap Puzzle-
*/

namespace puzzles::magic_square{

struct puz_game
{
    string m_id;
    Position m_size;
    vector<int> m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    int sidelen() const {return m_size.first;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs.size())
    , m_cells(sidelen() * sidelen())
{
    for (int r = 0, n = 0; r < sidelen(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < sidelen(); ++c)
            m_cells[n++] = stoi(string(str.substr(c * 2, 2)));
    }
}

struct puz_move
{
    Position m_p1, m_p2;
    puz_move(const Position& p1, const Position& p2)
        : m_p1(p1 + Position(1, 1)), m_p2(p2 + Position(1, 1)) {}
};

ostream & operator<<(ostream &out, const puz_move &mi)
{
    out << format("move: {} <=> {}\n", mi.m_p1, mi.m_p2);
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_game(&g) {}
    int sidelen() const {return m_game->sidelen();}
    int cells(int r, int c) const {return m_cells[r * sidelen() + c];}
    int& cells(int r, int c) {return m_cells[r * sidelen() + c];}
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    void make_move(int r1, int c1, int r2, int c2) {
        std::swap(cells(r1, c1), cells(r2, c2));
        m_move = puz_move(Position(r1, c1), Position(r2, c2));
    }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    boost::optional<puz_move> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    int rcmax = sidelen() - 1;
    for (int r1 = 0; r1 < rcmax; ++r1)
        for (int c1 = 0; c1 < rcmax; ++c1)
            for (int r2 = r1; r2 < rcmax; ++ r2)
                for (int c2 = r1 == r2 ? c1 + 1 : 0; c2 < rcmax; ++c2) {
                    children.push_back(*this);
                    children.back().make_move(r1, c1, r2, c2);
                }
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int d = 0;
    int rcmax = sidelen() - 1;
    for (int r = 0; r < rcmax; ++r) {
        int sum = 0;
        for (int c = 0; c < rcmax; ++c)
            sum += cells(r, c);
        if (sum != cells(r, rcmax))
            d++;
    }
    for (int c = 0; c < rcmax; ++c) {
        int sum = 0;
        for (int r = 0; r < rcmax; ++r)
            sum += cells(r, c);
        if (sum != cells(rcmax, c))
            d++;
    }
    {
        int sum = 0;
        for (int i = 0; i < rcmax; ++i)
            sum += cells(i, i);
        if (sum != cells(rcmax, rcmax))
            d++;
    }
    return (d + 3) / 4;
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << format("{:2}", cells(r, c));
        println(out);
    }
    return out;
}

}

void solve_puz_magic_square()
{
    using namespace puzzles::magic_square;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/magic_square.xml", "Puzzles/magic_square.txt");
}
