#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::hrd{

enum EBrickType {bt1X1, bt2X1, bt1X2, bt2X2};

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

typedef map<Position, EBrickType> brick_map;
typedef pair<const Position, EBrickType> brick_pair;

typedef vector<Position> brick_info;

struct puz_game
{
    array<brick_info, 4> m_bricks_info;
    Position m_size;
    brick_map m_brick_map;
    vector<char> m_cells;
    Position m_caocao, m_goal;
    string m_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
    , m_goal(rows() - 3, 2)
{
    for (int r = 0; r < rows(); ++r) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            m_cells.push_back(str[c]);
            switch(str[c]) {
            case 'A':
                m_brick_map[p] = bt1X1;
                break;
            case 'B':
                m_brick_map[p] = bt2X1;
                break;
            case 'C':
                m_brick_map[p] = bt1X2;
                break;
            case 'D':
                m_brick_map[m_caocao = p] = bt2X2;
                break;
            }
        }
    }

    Position pos[] = {Position(0, 0), Position(1, 0), Position(0, 1), Position(1, 1)};
    m_bricks_info[0].push_back(pos[0]);
    m_bricks_info[1].assign(pos, pos + 2);
    m_bricks_info[2].push_back(pos[0]);
    m_bricks_info[2].push_back(pos[2]);
    m_bricks_info[3].assign(pos, pos + 4);
}

typedef pair<Position, int> puz_step;

ostream& operator<<(ostream& out, const puz_step& a)
{
    const string_view dirs = "LRUD";
    out << boost::format("(%1%,%2%)%3%") % a.first.first % a.first.second % dirs[a.second];
    return out;
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g)
        : m_game(&g), m_cells(g.m_cells), m_caocao(g.m_caocao),
        m_brick_map(g.m_brick_map) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(int r, int c) const {return m_cells[r * cols() + c];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_move < x.m_move;
    }
    bool operator==(const puz_state& x) const {
        return m_cells == x.m_cells && m_move == x.m_move;
    }

    // solve_puzzle interface
    bool is_goal_state() const {return m_caocao == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_caocao, m_game->m_goal);}
    unsigned int get_distance(const puz_state& child) const{
        Position lastto = m_move ? m_move->first + offset[m_move->second] : Position(0, 0);
        unsigned int d = 1;
        if (lastto != child.m_move->first)
            d += 1 << 16;
        return d;
    }
    //unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    bool make_move(const Position& pos, EBrickType bt, const brick_info& bi, int i);
    const puz_game* m_game = nullptr;
    vector<char> m_cells;
    brick_map m_brick_map;
    Position m_caocao;
    boost::optional<puz_step> m_move;
};

bool puz_state::make_move(const Position& p, EBrickType bt, const brick_info& bi, int i)
{
    vector<char> vch;
    for (const Position& u : bi) {
        char& ch = cells(p + u);
        vch.push_back(ch);
        ch = ' ';
    }

    Position p2 = p + offset[i];
    for (const Position& u : bi)
        if (cells(p2 + u) != ' ')
            return false;

    for (size_t j = 0; j < vch.size(); ++j)
        cells(p2 + bi[j]) = vch[j];
    m_brick_map.erase(p);
    m_move = puz_step(p, i);
    m_brick_map[p2] = bt;
    if (m_caocao == p)
        m_caocao = p2;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    const array<brick_info, 4>& bis = m_game->m_bricks_info;
    for (const brick_pair& bp : m_brick_map) {
        const Position& p = bp.first;
        EBrickType bt = bp.second;
        const brick_info& bi = bis[bt];
        for (int i = 0; i < 4; ++i) {
            children.push_back(*this);
            if (!children.back().make_move(p, bt, bi, i))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells(r, c);
        out << endl;
    }
    return out;
}

}

void solve_puz_hrd()
{
    using namespace puzzles::hrd;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/hrd.xml", "Puzzles/hrd.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/hrd.xml", "Puzzles/hrd_ida.txt");
}
