#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: pattern puzzle
*/

namespace puzzles::patternpuzzle{

constexpr Position offset[] = {
    {0, 1},
    {1, 1},
    {1, 0},
    {1, -1},
    {0, -1},
    {-1, -1},
    {-1, 0},
    {-1, 1},
};

string dir_strs[] = {
    "r",
    "rd",
    "d",
    "ld",
    "l",
    "lu",
    "u",
    "ru",
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    vector<int> m_dirs;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    int& dir(int r, int c) {return m_dirs[r * cols() + c];}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
    , m_dirs(rows() * cols())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            int& d = dir(r, c);
            for (int i = 0; i < 8; ++i) {
                Position p = Position(r, c) + offset[i];
                if (is_valid(p))
                    d |= 1 << i;
            }
        }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_dirs(g.m_dirs), m_game(&g), m_index(-1) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    int dir(const Position& p) const {return m_dirs.at(p.first * cols() + p.second);}
    int& dir(const Position& p) {return m_dirs[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_dirs < x.m_dirs ||
            m_cells == x.m_cells && m_dirs == x.m_dirs && m_curpos < x.m_curpos;
    }
    bool first_move() const {return m_index == -1;}
    Position next_pos() const {return m_curpos + offset[m_index];}
    bool make_move(const Position& p, int i);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::accumulate(m_cells, 0, arg1 + (arg2 - '0'));}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {
        if (first_move()) return;
        out << m_curpos << " " << dir_strs[m_index] << " " << next_pos() << endl;
    }
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    vector<int> m_dirs;
    Position m_curpos;
    int m_index;
};

bool puz_state::make_move(const Position& p, int i)
{
    if (first_move())
        --cells(p);
    dir(p) ^= 1 << i;

    Position p2 = p + offset[i];
    --cells(p2);

    m_curpos = p;
    m_index = i;

    // pruning
    //for(int r = 0; r < rows(); ++r)
    //    for (int c = 0; c < cols(); ++c) {
    //        Position p(r, c);
    //        if (cells(p) == '0') continue;
    //        bool connected = false;
    //        for (int i = 0; i < 8; ++i)
    //            if ((dir(p) & (1 << i)) && cells(p + offset[i]) != '0') {
    //                connected = true;
    //                break;
    //            }
    //        if (!connected) return false;
    //    }

    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<Position> curs;
    if (first_move())
        for (int r = 0; r < rows(); ++r)
            for (int c = 0; c < cols(); ++c) {
                Position p(r, c);
                if (cells(p) != '0')
                    curs.push_back(p);
            } else
        curs.push_back(next_pos());

    for (const Position& p : curs)
        for (int i = 0; i < 8; ++i)
            if ((dir(p) & (1 << i)) && cells(p + offset[i]) != '0') {
                children.push_back(*this);
                if (!children.back().make_move(p, i))
                    children.pop_back();
            }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << cells({r, c}) << " ";
        println(out);
    }
    return out;
}

}

void solve_puz_patternpuzzle()
{
    using namespace puzzles::patternpuzzle;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/patternpuzzle.xml", "Puzzles/patternpuzzle.txt", solution_format::MOVES_ONLY);
}
