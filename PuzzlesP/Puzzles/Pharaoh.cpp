#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: PharaohÅfs Puzzle
*/

namespace puzzles::Pharaoh{

enum EBrickDir {mvLeft, mvRight, mvUp, mvDown};
enum EBrickType {btRed, btBlue, btYellow};

constexpr array<Position, 4> offset = Position::Directions4;

using brick_map = map<Position, EBrickType>;
using brick_pair = pair<const Position, EBrickType>;

struct brick_info
{
    vector<Position> units;
    vector<EBrickDir> dirs;
    template<class I, class J>
    brick_info(I beg1, I end1, J beg2, J end2) :
    units(beg1, end1), dirs(beg2, end2) {}
};

struct puz_game
{
    vector<brick_info> brick_infos;
    Position m_size;
    brick_map m_brick_map;
    vector<char> m_cells;
    string m_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    Position pos[] = {Position(0, 0), Position(0, 1), Position(0, 0), Position(1, 0)};
    EBrickDir dirs[] = {mvLeft, mvRight, mvUp, mvDown};
    brick_info bi1(pos, pos + 2, dirs, dirs + 2);
    brick_info bi2(pos + 2, pos + 4, dirs + 2, dirs + 4);
    brick_infos.push_back(bi1);
    brick_infos.push_back(bi1);
    brick_infos.push_back(bi2);

    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            m_cells.push_back(str[c]);
            switch(str[c]) {
            case '=':
                m_brick_map[p] = btRed;
                break;
            case '-':
                m_brick_map[p] = btBlue;
                break;
            case '|':
                m_brick_map[p] = btYellow;
                break;
            }
        }
    }
}

using puz_move = pair<Position, EBrickDir>;

ostream& operator<<(ostream& out, const puz_move& mi)
{
    const string_view dirs = "LRUD";
    out << format("move: ({},{}) {}\n", mi.first.first, mi.first.second, dirs[mi.second]);
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_game(&g), m_cells(g.m_cells), m_brick_map(g.m_brick_map) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_move < x.m_move;
    }
    bool operator==(const puz_state& x) const {
        return m_cells == x.m_cells && m_move == x.m_move;
    }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return myabs(cols() - 3 - red_pos().second);}
    unsigned int get_distance(const puz_state& child) const{
        Position lastto = m_move ? m_move->first + offset[m_move->second] : Position(0, 0);
        //unsigned int d = 1;
        //if(lastto != x.m_move->first)
        //    d += 1 << 16;
        unsigned int d = 1 << 16;
        if (lastto != child.m_move->first)
            d += 1;
        return d;
    }
    //unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    Position red_pos() const;
    bool make_move(const Position& pos, EBrickType bt, const vector<Position>& units, EBrickDir dir);
    const puz_game* m_game;
    vector<char> m_cells;
    brick_map m_brick_map;
    boost::optional<puz_move> m_move;
};

bool puz_state::make_move(const Position& pos, EBrickType bt, const vector<Position>& units, EBrickDir dir)
{
    bool can_move = true;
    vector<char> vch;
    for (const Position& p : units)
        vch.push_back(cells(pos + p));
    for (const Position& p : units)
        cells(pos + p) = ' ';
    Position pos2 = pos + offset[dir];
    for (const Position& p : units)
        if (cells(pos2 + p) != ' ') {
            pos2 = pos;
            can_move = false;
            break;
        }
    for (size_t i = 0; i < vch.size(); i++)
        cells(pos2 + units[i]) = vch[i];
    if (can_move) {
        m_brick_map.erase(pos);
        m_move = puz_move(pos, dir);
        m_brick_map[pos2] = bt;
    }
    return can_move;
}

Position puz_state::red_pos() const
{
    for (const brick_pair& bp : m_brick_map)
        if (bp.second == btRed)
            return bp.first;
    return Position();
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int i = 0, n = 0; i < rows(); i++) {
        for (int j = 0; j < cols(); j++, n++)
            out << m_cells[n];
        println(out);
    }
    return out;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    const vector<brick_info>& bis = m_game->brick_infos;
    puz_state p = *this;
    for (const brick_pair& bp : m_brick_map) {
        const Position& pos = bp.first;
        EBrickType bt = bp.second;
        const brick_info& bi = bis[bt];
        for (EBrickDir dir : bi.dirs)
            if (p.make_move(pos, bt, bi.units, dir)) {
                children.push_back(p);
                p = *this;
            }
    }
}

}

void solve_puz_Pharaoh()
{
    using namespace puzzles::Pharaoh;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Pharaoh.xml", "Puzzles/Pharaoh_a.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/Pharaoh.xml", "Puzzles/Pharaoh_ida.txt");
}
