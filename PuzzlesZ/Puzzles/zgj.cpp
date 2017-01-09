#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace zgj{

#define PUZ_WALL        '#'
#define PUZ_MONKEY        '@'
#define PUZ_STONE        'S'
#define PUZ_TRUNK        'T'
#define PUZ_MUD            'M'
#define PUZ_ICE            'I'
#define PUZ_GOAL        '.'
#define PUZ_SPACE        ' '

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
    string m_cells;
    Position m_monkey, m_goal;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_cells = string(rows() * cols(), ' ');
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_WALL);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_WALL);

    int n = cols();
    for(int r = 1; r < rows() - 1; ++r, n += cols()){
        const string& str = strs[r - 1];
        m_cells[n] = m_cells[n + cols() - 1] = PUZ_WALL;
        for(int c = 1; c < cols() - 1; ++c){
            char ch = str[c - 1];
            switch(ch){
            case PUZ_MONKEY:
                m_monkey = Position(r, c);
                ch = PUZ_SPACE;
                break;
            case PUZ_GOAL:
                m_goal = Position(r, c);
                ch = PUZ_SPACE;
                break;
            }
            m_cells[n + c] = ch;
        }
    }
}

struct puz_state_base
{
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}

    const puz_game* m_game;
    Position m_monkey;
    string m_move;
};

struct puz_state2;

struct puz_state : puz_state_base
{
    puz_state() {}
    puz_state(const puz_game& g) : m_cells(g.m_cells){
        m_game = &g, m_monkey = g.m_monkey;
    }
    puz_state(const puz_state2& x2);
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_monkey < x.m_monkey ||
            m_cells == x.m_cells && m_monkey == x.m_monkey && m_move < x.m_move;
    }
    void make_move(const Position& p1, const Position& p2, char dir);

    //solve_puzzle interface
    bool is_goal_state() const {return m_monkey == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_monkey, m_game->m_goal);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(!m_move.empty()) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    string m_cells;
};

struct puz_state2 : puz_state_base
{
    puz_state2(const puz_state& s) : m_cells(&s.m_cells){
        m_game = s.m_game, m_monkey = s.m_monkey;
    }
    char cells(const Position& p) const {return (*m_cells)[p.first * cols() + p.second];}
    bool operator<(const puz_state2& x) const {
        return m_monkey < x.m_monkey;
    }
    void make_move(const Position& p, char dir){
        m_monkey = p, m_move += dir;
    }
    void gen_children(list<puz_state2>& children) const;

    const string* m_cells;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    static char* dirs = "lrud";
    for(int i = 0; i < 4; ++i){
        Position p = m_monkey + offset[i];
        if(cells(p) == PUZ_SPACE){
            children.push_back(*this);
            children.back().make_move(p, dirs[i]);
        }
    }
}

puz_state::puz_state(const puz_state2& x2)
    : m_cells(*x2.m_cells)
{
    m_game = x2.m_game, m_monkey = x2.m_monkey, m_move = x2.m_move;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    static char* dirs = "LRUD";
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    for(const puz_state2& s : smoves)
        if(s.m_monkey == m_game->m_goal)
            children.push_back(s);
        else
            for(int i = 0; i < 4; ++i){
                Position p1 = s.m_monkey + offset[i];
                char ch = cells(p1);
                if(ch != PUZ_TRUNK && ch != PUZ_MUD && ch != PUZ_ICE) continue;
                Position p2 = p1 + offset[i];
                ch = cells(p2);
                if(ch != PUZ_SPACE) continue;
                children.push_back(s);
                children.back().make_move(p1, p2, dirs[i]);
            }
}

void puz_state::make_move(const Position& p1, const Position& p2, char dir)
{
    char& ch = cells(p2) = cells(p1);
    cells(p1) = PUZ_SPACE;

    vector<Position> vp;
    for(int i = 0; i < 4; ++i){
        Position p = p2 + offset[i];
        char ch2 = cells(p);
        if(ch2 == PUZ_STONE){
            ch = PUZ_STONE;
            break;
        }
        if(ch2 == ch)
            vp.push_back(p);
    }
    if(ch != PUZ_STONE && !vp.empty()){
        ch = PUZ_SPACE;
        for(const Position& p : vp)
            cells(p) = PUZ_SPACE;
    }

    m_monkey = p1;
    m_move += dir;
}

ostream& puz_state::dump(ostream& out) const
{
    if(!m_move.empty())
        out << "move: " << m_move << endl;
    for(int r = 0; r < rows(); ++r) {
        for(int c = 0; c < cols(); ++c){
            Position p(r, c);
            char ch = cells({r, c});
            out << (p == m_monkey ? PUZ_MONKEY :
                ch == PUZ_SPACE && p == m_game->m_goal ? PUZ_GOAL :
                ch);
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_zgj()
{
    using namespace puzzles::zgj;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/zgj.xml", "Puzzles/zgj.txt");
}
