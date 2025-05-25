#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::icedin{

#define PUZ_GROUND        '!'
#define PUZ_SPACE        ' '
#define PUZ_STONE        '$'
#define PUZ_BLOCK        'b'
#define PUZ_SWITCH        '.'
#define PUZ_BLOCK_ON_SWITCH        'B'
#define PUZ_ICE3        '3'
#define PUZ_ICE2        '2'
#define PUZ_ICE1        '1'
#define PUZ_HOLE        'X'
#define PUZ_BLOCK_ON_ICE2        'd'
#define PUZ_BLOCK_ON_ICE1        'c'

enum EDir {mvLeft, mvRight, mvUp, mvDown};

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    vector<Position> m_blocks, m_switches;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_switch(const Position& p) const {
        return boost::algorithm::any_of_equal(m_switches, p);
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_start.resize(rows() * cols());
    for (int r = 0, n = 0; r < rows(); ++r) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); ++c, ++n) {
            Position p(r, c);
            switch(char ch = str[c]) {
            case PUZ_BLOCK:
                m_start[n] = ch;
                m_blocks.push_back(p);
                break;
            case PUZ_SWITCH:
                m_start[n] = PUZ_SPACE;
                m_switches.push_back(p);
                break;
            default:
                m_start[n] = ch;
                break;
            }
        }
    }
}

struct puz_step
{
    Position m_p;
    char m_dir;
    puz_step(const Position& p, char dir)
        : m_p(p), m_dir(dir) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: {} {}\n", mi.m_p, mi.m_dir);
    return out;
}

class puz_state : public string
{
public:
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g), m_blocks(g.m_blocks) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    bool make_move(size_t n, EDir dir);
private:
    unsigned int slide_distance(int r1, int c1, int r2, int c2) const;
    unsigned int slide_distance2(int i, int j1, int j2, bool i_is_r) const;
public:
    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<Position> m_blocks;
    boost::optional<puz_step> m_move;
};

bool puz_state::make_move(size_t n, EDir dir)
{
    static string_view moves = "lrud";
    Position& p = m_blocks[n];
    Position os = offset[dir];
    char& ch_push = cells(p - os);
    if (ch_push != PUZ_GROUND && ch_push != PUZ_SPACE &&
        ch_push != PUZ_ICE3 && ch_push != PUZ_ICE2)
        return false;
    Position p2 = p;
    for (;;) {
        char& ch = cells(p2 + os);
        if (ch == PUZ_SPACE)
            p2 += os;
        else if (ch == PUZ_ICE3 || ch == PUZ_ICE2)
            ch--, p2 += os;
        else if (ch == PUZ_ICE1 || ch == PUZ_HOLE) {
            ch = PUZ_HOLE, p2 += os;
            break;
        } else
            break;
    }
    if (p2 == p)
        return false;
    if (ch_push == PUZ_ICE3 || ch_push == PUZ_ICE2)
        ch_push--;
    m_move = puz_step(p - os, moves[dir]);
    char& chSrc = cells(p);
    chSrc = chSrc == PUZ_BLOCK ? PUZ_SPACE : chSrc - PUZ_BLOCK + '0';
    char& chDest = cells(p = p2);
    if (chDest == PUZ_SPACE)
        chDest = PUZ_BLOCK;
    else if (chDest == PUZ_ICE2 || chDest == PUZ_ICE1)
        chDest += PUZ_BLOCK - '0';
    else {
        //chDest == PUZ_HOLE
        m_blocks.erase(m_blocks.begin() + n);
        if (m_blocks.size() < m_game->m_switches.size())
            return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i)
        for (size_t n = 0; n < m_blocks.size(); ++n) {
            children.push_back(*this);
            if (!children.back().make_move(n, EDir(i)))
                children.pop_back();
        }
}

unsigned int puz_state::slide_distance2(int i, int j1, int j2, bool i_is_r) const
{
    unsigned int d = 2;
    int j_push = j1 < j2 ? j1 - 1 : j1 + 1;
    switch(cells(i_is_r ? Position(i, j_push) : Position(j_push, i)))
    {
    case PUZ_BLOCK:
    case PUZ_BLOCK_ON_ICE1:
    case PUZ_BLOCK_ON_ICE2:
    case PUZ_ICE1:
    case PUZ_HOLE:
    case PUZ_STONE:
        return 3;
    }
    int j_stop = j1 < j2 ? j2 + 1 : j2 - 1;
    switch(cells(i_is_r ? Position(i, j_stop) : Position(j_stop, i)))
    {
    case PUZ_BLOCK:
    case PUZ_BLOCK_ON_ICE1:
    case PUZ_BLOCK_ON_ICE2:
    case PUZ_GROUND:
    case PUZ_STONE:
        d = 1;
        break;
    }
    int jmin = min(j1, j2), jmax = max(j1, j2);
    for (int j = jmin + 1; j < jmax; ++j) {
        switch(cells(i_is_r ? Position(i, j) : Position(j, i))) {
        case PUZ_BLOCK:
        case PUZ_BLOCK_ON_ICE1:
        case PUZ_BLOCK_ON_ICE2:
            d = 2;
            break;
        case PUZ_HOLE:
        case PUZ_ICE1:
        case PUZ_GROUND:
        case PUZ_STONE:
            return 3;
        }
    }
    return d;
}

unsigned int puz_state::slide_distance(int r1, int c1, int r2, int c2) const
{
    return 
        r1 == r2 && c1 == c2 ? 0 :
        r1 != r2 && c1 != c2 ? 2 :
        r1 == r2 ? slide_distance2(r1, c1, c2, true) :
        slide_distance2(c1, r1, r2, false);
//        1;
}

unsigned int puz_state::get_heuristic() const
{
    int r1, c1, r2, c2;
    unsigned int d_sum = 0;
    for (const Position& p2 : m_game->m_switches) {
        boost::tie(r2, c2) = p2;
        unsigned int d_min = 3;
        for (const Position& p1 : m_blocks) {
            boost::tie(r1, c1) = p1;
            unsigned int d = slide_distance(r1, c1, r2, c2);
            if (d_min > d)
                d_min = d;
        }
        d_sum += d_min;
    }
    return d_sum;
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (
                !m_game->is_switch(p) ? ch :
                ch == PUZ_BLOCK ? PUZ_BLOCK_ON_SWITCH :
                PUZ_SWITCH
            );
        }
        println(out);
    }
    return out;
}

}

void solve_puz_icedin()
{
    using namespace puzzles::icedin;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/icedin.xml", "Puzzles/icedin.txt");
}
