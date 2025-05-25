#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::zafiro{

#define PUZ_FIXED        '#'
#define PUZ_GOAL        '.'
#define PUZ_ZAFIRO        '$'
#define PUZ_GLASS        '!'
#define PUZ_COLORED        'o'
#define PUZ_SPACE        ' '

const Position offset[] = {
    {1, 0},
    {0, 1},
    {-1, 0},
    {0, -1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    Position m_zafiro, m_portal;
    set<Position> m_glasses;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_start = string(rows() * cols(), PUZ_SPACE);
    fill(m_start.begin(), m_start.begin() + cols(), PUZ_FIXED);
    fill(m_start.rbegin(), m_start.rbegin() + cols(), PUZ_FIXED);

    for (int r = 1, n = cols(); r < rows() - 1; ++r) {
        const string& str = strs[r - 1];
        m_start[n++] = PUZ_FIXED;
        for (int c = 1; c < cols() - 1; ++c) {
            char ch = str[c - 1];
            Position p(r, c);
            switch(ch) {
            case PUZ_ZAFIRO:
                m_zafiro = p;
                break;
            case PUZ_GOAL:
                m_portal = p;
                ch = PUZ_SPACE;
                break;
            case PUZ_GLASS:
                m_glasses.insert(p);
                break;
            }
            m_start[n++] = ch;
        }
        m_start[n++] = PUZ_FIXED;
    }
}

struct puz_step
{
    char m_dir;
    Position m_p;
    bool m_is_click;

    puz_step(char dir) : m_is_click(false), m_dir(dir) {}
    puz_step(const Position& p) : m_is_click(true), m_p(p) {}
};

ostream& operator<<(ostream& out, const puz_step& act)
{
    if (act.m_is_click)
        out << act.m_p;
    else
        out << act.m_dir;
    return out;
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g)
        : string(g.m_start), m_game(&g), m_zafiro(g.m_zafiro)
        , m_grav(0) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return (*this)[p.first * cols() + p.second];}
    char& cells(const Position& p) {return (*this)[p.first * cols() + p.second];}
    bool make_move(int i);
    void click(const Position& p);

    //solve_puzzle interface
    bool is_goal_state() const {return m_zafiro == m_game->m_portal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    Position m_zafiro;
    int m_grav;
    boost::optional<puz_step> m_move;
};

bool puz_state::make_move(int i)
{
    static string_view dirs = "drul";
    int j = i == 4 ? m_grav :(i + m_grav) % 4;
    Position os = offset[j];
    bool moved = false;
    int rs = j % 2 == 0 ? rows() : cols();
    int cs = j % 2 == 0 ? cols() : rows();
    for (int r = rs - 2; r >= 1; --r) {
        for (int c = 1; c < cs - 1; ++c) {
            Position p =
                j == 1 ? Position(cs - 1 - c, r) :
                j == 2 ? Position(rs - 1 - r, cs - 1 - c) :
                j == 3 ? Position(c, rs - 1 - r) :
                Position(r, c);
            char ch = cells(p);
            if (ch != PUZ_ZAFIRO && ch != PUZ_COLORED)
                continue;
            Position p2 = p;
            while (cells(p2 + os) == PUZ_SPACE)
                p2 += os;
            if (p2 != p) {
                moved = true;
                cells(p) = PUZ_SPACE;
                cells(p2) = ch;
                if (ch == PUZ_ZAFIRO)
                    m_zafiro = p2;
            }
        }
    }
    if (moved && i != 4) {
        m_move = puz_step(dirs[i]);
        m_grav = j;
    }
    return moved;
}

void puz_state::click(const Position& p)
{
    cells(p) = PUZ_SPACE;
    Position p2 =
        m_grav == 1 ? Position(p.second, rows() - 1 - p.first) :
        m_grav == 2 ? Position(rows() - 1 - p.first, cols() - 1 - p.second) :
        m_grav == 3 ? Position(cols() - 1 - p.second, p.first) :
        p;
    m_move = puz_step(p2);
    make_move(4);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i) {
        children.push_back(*this);
        if (!children.back().make_move(i))
            children.pop_back();
    }
    for (const Position& p : m_game->m_glasses) {
        if (cells(p) != PUZ_GLASS) continue;
        children.push_back(*this);
        children.back().click(p);
    }
}

unsigned int puz_state::get_heuristic() const
{
    int ir, ic, jr, jc;
    boost::tie(ir, ic) = m_zafiro;
    boost::tie(jr, jc) = m_game->m_portal;
    if (ir == jr && ic == jc) return 0;
    if (ir != jr && ic != jc) return 2;
    //int num_glass = 0;
    //if(ir == jr) {
    //    int d = ic < jc ? 1 : -1;
    //    if (cells({jr, jc + d}) != PUZ_SPACE) return 3;
    //    for (int c = ic + d; c != jc; c += d)
    //        switch(cells({ir, c})) {
    //        case PUZ_FIXED:
    //            return 3;
    //        case PUZ_GLASS:
    //            ++num_glass;
    //            break;
    //    }
    //}
    //else {
    //    int d = ir < jr ? 1 : -1;
    //    if (cells({jr + d, jc}) != PUZ_SPACE) return 3;
    //    for (int r = ir + d; r != jr; r += d)
    //        switch(cells({r, ic})) {
    //        case PUZ_FIXED:
    //            return 3;
    //        case PUZ_GLASS:
    //            ++num_glass;
    //            break;
    //    }
    //}
    //return std::min(num_glass + 1, 3);
    return 1;
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    for (int r = 1; r < rows() - 1; ++r) {
        for (int c = 1; c < cols() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_zafiro()
{
    using namespace puzzles::zafiro;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/zafiro.xml", "Puzzles/zafiro.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
