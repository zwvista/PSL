#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::lightsout{

#define PUZ_OFF        '0'
#define PUZ_NONE    '#'

enum EWrapAround {waNone, waLeftRight, waUpDown, waBoth};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    char m_on;
    vector<Position> m_offset;
    EWrapAround m_wraparound;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    static const string movetype_orthogonal = "010111010";
    static const string movetype_diagonal = "101010101";
    static const string movetype_knight = "0101010001001001000101010";
    static const string movetype_hexagonal = "0000001010101010101000000";
    string movetype = level.attribute("movetype").as_string(movetype_orthogonal.c_str());
    if (movetype == "+")
        movetype = movetype_orthogonal;
    else if (movetype == "*")
        movetype = movetype_diagonal;
    else if (movetype == "k")
        movetype = movetype_knight;
    else if (movetype == "h")
        movetype = movetype_hexagonal;
    int delta = movetype.length() == 25 ? 2 : 1;
    for (int r = -delta, i = 0; r <= delta; ++r)
        for (int c = -delta; c <= delta; ++c, ++i)
            if (movetype[i] == '1')
                m_offset.push_back(Position(r, c));

    string wraparound = level.attribute("wraparound").as_string("none");
    m_wraparound = 
        wraparound == "both" ? waBoth :
        wraparound == "left-right" ? waLeftRight :
        wraparound == "up-down" ? waUpDown :
        waNone;

    m_start = boost::accumulate(strs, string());
    m_on = string(level.attribute("on").as_string("1"))[0];
}

struct puz_step
{
    Position m_p;
    puz_step(const Position& p)
        : m_p(p + Position(1, 1)) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << "click " << mi.m_p << endl;;
    return out;
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    bool is_valid(Position& p) const;
    void click(const Position& p);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        // not an admissible heuristic
        int n = 0;
        for (size_t i = 0; i < length(); ++i) {
            char ch = at(i);
            if (ch != PUZ_NONE)
                n += at(i) - PUZ_OFF;
        }
        return n;
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    boost::optional<puz_step> m_move;
};

bool puz_state::is_valid(Position& p) const
{
    EWrapAround wa = m_game->m_wraparound;
    if ((wa == waNone || wa == waLeftRight) && (p.first < 0 || p.first >= rows()) ||
        (wa == waNone || wa == waUpDown) && (p.second < 0 || p.second >= cols()))
        return false;
    if (wa != waNone) {
        p.first = (p.first + rows()) % rows();
        p.second = (p.second + cols()) % cols();
    }
    return true;
}

void puz_state::click(const Position& p)
{
    for (const Position& os : m_game->m_offset) {
        Position p2 = p + os;
        if (is_valid(p2) && cells(p2) != PUZ_NONE)
            // with "dim" state
            cells(p2) = cells(p2) == PUZ_OFF ? m_game->m_on : cells(p2) - 1;
    }
    m_move = puz_step(p);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_NONE) {
                children.push_back(*this);
                children.back().click(p);
            }
        }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells({r, c}) << " ";
        out << endl;
    }
    return out;
}

}

void solve_puz_lightsout()
{
    using namespace puzzles::lightsout;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/lightsout_classic.xml", "Puzzles/lightsout_classic.txt", solution_format::MOVES_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/lightsout_2000.xml", "Puzzles/lightsout_2000.txt", solution_format::MOVES_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/lightsout_variants.xml", "Puzzles/lightsout_variants.txt", solution_format::MOVES_ONLY);
}
