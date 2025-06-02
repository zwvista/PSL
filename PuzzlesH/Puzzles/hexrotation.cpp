#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::hexrotation{

constexpr auto PUZ_NOENTRY = '#';

/*
    iOS Game: Hex Rotation
*/

constexpr Position offset[] = {
    {0, 2},
    {-1, 1},
    {-1, -1},
    {0, -2},
    {1, -1},
    {1, 1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start, m_goal;
    vector<Position> m_clickable;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2, strs[0].length())
{
    m_start = accumulate(strs.begin(), strs.begin() + rows(), string());
    m_goal = accumulate(strs.begin() + rows(), strs.end(), string());
    for (int r = 0, n = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c, ++n) {
            bool clickable = true;
            Position p(r, c);
            for (int i = 0; i < 6; ++i) {
                Position p2 = p + offset[i];
                if (m_start[n] == PUZ_NOENTRY ||
                    p2.first < 0 || p2.first >= rows() ||
                    p2.second < 0 || p2.second >= cols()) {
                    clickable = false;
                    break;
                }
            }
            if (clickable)
                m_clickable.push_back(p);
        }
}

struct puz_step : Position
{
    puz_step(const Position& p)
        : Position(p + Position(1, 1)) {}
};

struct puz_state : string
{
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    void click(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const {return *this == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 6;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    boost::optional<puz_step> m_move;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    for (const Position& p : m_game->m_clickable) {
        children.push_back(*this);
        children.back().click(p);
    }
}

void puz_state::click(const Position& p)
{
    static vector<char> v(6);
    for (int i = 0; i < 6; ++i)
        v[i] = cells(p + offset[i]);
    rotate(v.begin(), v.end() - 1, v.end());
    for (int i = 0; i < 6; ++i)
        cells(p + offset[i]) = v[i];
    m_move = puz_step(p);
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int md = 0;
    map<char, Position> m1, m2;
    for (size_t i = 0; i < length(); ++i) {
        char ch = at(i);
        if (ch != PUZ_NOENTRY)
            m1[ch] += Position(i / cols(), i % cols());
    }
    for (size_t i = 0; i < length(); ++i) {
        char ch = m_game->m_goal[i];
        if (ch != PUZ_NOENTRY)
            m2[ch] += Position(i / cols(), i % cols());
    }
    using pair_type = pair<char, Position>;
    for (const pair_type& pr : m1) {
        const Position& p1 = pr.second;
        const Position& p2 = m2.at(pr.first);
        md += manhattan_distance(p1, p2);
    }
    return md / 2;
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "click: " << *m_move << endl;
    println(out);
    return out;
}

}

void solve_puz_hexrotation()
{
    using namespace puzzles::hexrotation;
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
    //    "Puzzles/hexrotation.xml", "Puzzles/hexrotation.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/hexrotation.xml", "Puzzles/hexrotation_ida.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
