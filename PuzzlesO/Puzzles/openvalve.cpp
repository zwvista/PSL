#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::openvalve{

constexpr auto PUZ_PIPE_L = 'L';        // "┌","┐","└","┘"
constexpr auto PUZ_PIPE_I = 'I';        // "─","│"
constexpr auto PUZ_PIPE_3 = '3';        // "├","┤","┬","┴"
constexpr auto PUZ_PIPE_4 = '4';        // "┼"

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};
const string dirs = "wensewsn";
const string pipes = "LI34";
const int pipe_offset[] = {0, 4, 6, 10, 11};
const string pipe_dirss[] = {
    "se", "sw", "ne", "nw",
    "we", "ns",
    "nsw", "nse", "swe", "nwe",
    "wens"
};
const string pipe_cells[] = {
    "se", "sw", "ne", "nw",
    "we", "ns",
    "-e", "-w", "-n", "-s",
    "--"
};
typedef pair<Position, char> pipe_info;

struct puz_game
{
    string m_id;
    Position m_size;
    string m_pipes, m_start;
    Position m_entrance, m_exit;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    int cell_offset(const Position& p) const {return p.first * cols() + p.second;}
    char pipe(const Position& p) const {return m_pipes[cell_offset(p)];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length() - 2)
    , m_start(rows() * cols() * 3, '.')
{
    for (int r = 0; r < rows(); ++r) {
        const string& s = strs[r];
        if (s[0] == '-')
            m_entrance = Position(r, -1);
        if (s[cols() + 1] == '-')
            m_exit = Position(r, cols());
        m_pipes += s.substr(1, cols());
    }
}

struct puz_state
{
    puz_state(const puz_game& g) 
        : m_game(&g), m_cells(g.m_start), m_connected(false)
        , m_frontier(1, make_pair(m_game->m_entrance, 'e')) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    int cell_offset(const Position& p) const {return m_game->cell_offset(p) * 3;}
    string get_cells(const Position& p) const {return m_cells.substr(cell_offset(p), 3);}
    void set_cells(const Position& p, const string& cell) {
        boost::range::copy(cell, m_cells.begin() + cell_offset(p));
    }
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_connected < x.m_connected ||
            m_cells == x.m_cells && m_connected == x.m_connected && m_frontier < x.m_frontier;
    }
    bool operator==(const puz_state& x) const {
        return m_cells == x.m_cells && m_connected == x.m_connected && m_frontier == x.m_frontier;
    }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    bool can_move(const Position& p, char dir, vector<int>& offset_vec) const;
    void remove_dir(const Position& p, char dir) {
        boost::range::remove_erase(m_frontier, make_pair(p, dir));
    }
    void make_move(const Position& p, char dir, int i);

    //solve_puzzle interface
    bool is_goal_state() const {return m_connected && m_frontier.empty();}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {println(out);}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    bool m_connected;
    list<pipe_info> m_frontier;
};

bool puz_state::can_move(const Position& p, char dir, vector<int>& offset_vec) const
{
    int n = dirs.find(dir);
    Position p2 = p + offset[n];
    if (p2 == m_game->m_exit)
        return true;
    if (!is_valid(p2))
        return false;

    char dir2 = dirs[n + 4];
    if (get_cells(p2) != "...")
        return boost::algorithm::any_of_equal(m_frontier, make_pair(p2, dir2));

    n = pipes.find(m_game->pipe(p2));
    for (int i = pipe_offset[n]; i < pipe_offset[n + 1]; ++i)
        if (pipe_dirss[i].find(dir2) != -1)
            offset_vec.push_back(i);
    return true;
}

void puz_state::make_move(const Position& p, char dir, int i)
{
    remove_dir(p, dir);
    int n = dirs.find(dir);
    Position p2 = p + offset[n];
    char dir2 = dirs[n + 4];
    if (i == -1)
        if (p2 == m_game->m_exit)
            m_connected = true;
        else
            remove_dir(p2, dir2);
    else {
        set_cells(p2, m_game->pipe(p2) + pipe_cells[i]);
        for (char dir3 : pipe_dirss[i])
            if (dir3 != dir2)
                m_frontier.push_back(make_pair(p2, dir3));
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_frontier.empty()) return;
    const pipe_info& pi = m_frontier.front();
    const Position& p = pi.first;
    char dir = pi.second;
    vector<int> offset_vec;
    if (!can_move(p, dir, offset_vec))
        return;
    if (offset_vec.empty())
        offset_vec.push_back(-1);

    for (int i : offset_vec) {
        children.push_back(*this);
        children.back().make_move(p, dir, i);
    }
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int d = 0;
    for (const pipe_info& pi : m_frontier)
        d = max(d, manhattan_distance(pi.first, m_game->m_exit));
    return d;
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        out << (r == m_game->m_entrance.first ? '-' : ' ');
        for (int c = 0; c < cols(); ++c)
            out << get_cells({r, c});
        out << (r == m_game->m_exit.first ? '-' : ' ') << endl;
    }
    return out;
}

}

void solve_puz_openvalve()
{
    using namespace puzzles::openvalve;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/openvalve.xml", "Puzzles/openvalve.txt", solution_format::GOAL_STATE_ONLY);
}
