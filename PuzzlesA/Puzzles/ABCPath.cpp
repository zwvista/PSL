#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    https://www.brainbashers.com/showabcpath.asp
    ABC Path
 
    Description
    1.  Enter every letter from A to Y into the grid.
    2.  Each letter is next to the previous letter either horizontally, vertically or diagonally.
    3.  The clues around the edge tell you which row, column or diagonal each letter is in.
*/

namespace puzzles::ABCPath{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2ch;
    map<char, Position> m_ch2pos;
    Position m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 2)
{
    for (int r = 0; r < m_sidelen + 2; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen + 2; ++c) {
            Position p(r, c);
            char ch = str[c];
            if (r == 0 || c == 0 || r == m_sidelen + 1 || c == m_sidelen + 1)
                m_ch2pos[ch] = p, m_pos2ch[p] = ch;
            else if (ch == 'A')
                m_start = Position(r - 1, c - 1);
        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const {return (*this)[p.first * sidelen() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * sidelen() + p.second];}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool make_move(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    char m_ch;
    Position m_p;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g), m_p(g.m_start), m_ch('A')
{
    cells(m_p) = m_ch;
}

bool puz_state::make_move(const Position& p)
{
    cells(m_p = p) = ++m_ch;
    auto it = m_game->m_ch2pos.find(m_ch);
    if (it == m_game->m_ch2pos.end()) return true;
    auto p2 = it->second;
    return (p2 == Position(0, 0) || p2 == Position(sidelen() + 1, sidelen() + 1)) && p.first == p.second ||
        (p2 == Position(0, sidelen() + 1) || p2 == Position(sidelen() + 1, 0)) && p.first + p.second == sidelen() - 1 ||
        (p2.first == 0 || p2.first == sidelen() + 1) && p.second == p2.second - 1 ||
        (p2.second == 0 || p2.second == sidelen() + 1) && p.first == p2.first - 1;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (auto& os : offset) {
        auto p = m_p + os;
        if (!is_valid(p) || cells(p) != PUZ_SPACE) continue;
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen() + 2; ++r) {
        for (int c = 0; c < sidelen() + 2; ++c)
            if (r == 0 || c == 0 || r == sidelen() + 1 || c == sidelen() + 1)
                out << m_game->m_pos2ch.at(Position(r, c)) << ' ';
            else
                out << cells(Position(r - 1, c - 1)) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_ABCPath()
{
    using namespace puzzles::ABCPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ABCPath.xml", "Puzzles/ABCPath.txt", solution_format::GOAL_STATE_ONLY);
}
