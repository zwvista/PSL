#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"
#include "HiddenPath.h"


namespace puzzles::HiddenPath{

struct puz_game    
{
    string m_id;
    int m_sidelen;
    vector<int> m_nums, m_dirs;
    map<int, Position> m_num2pos;
    Position m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen * 3; c += 3) {
            Position p(r, c / 3);
            auto s = str.substr(c, 2);
            int n = s == "  " ? PUZ_UNKNOWN : stoi(string(s));
            if (n == 1)
                m_start = p;
            m_nums.push_back(n);
            m_dirs.push_back(str[c + 2] - '0');
            if (n != PUZ_UNKNOWN)
                m_num2pos[n] = p;
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g) 
        : m_cells(g.m_nums), m_p(g.m_start)
        , m_game(&g) {}
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int index(const Position& p) const { return p.first * sidelen() + p.second; }
    int dirs(const Position& p) const { return m_game->m_dirs[index(p)]; }
    int cells(const Position& p) const { return m_cells[index(p)]; }
    int& cells(const Position& p) { return m_cells[index(p)]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_p) < tie(x.m_cells, x.m_p);
    }
    void make_move(const Position& p, int n) { cells(m_p = p) = n; }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_cells.size() - cells(m_p); }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    // the current position
    Position m_p;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    int n = cells(m_p) + 1;
    auto it = m_game->m_num2pos.find(n);
    bool found = it != m_game->m_num2pos.end();
    auto& os = offset[dirs(m_p)];
    for (auto p = m_p + os; is_valid(p); p += os)
        if (found && p == it->second || !found && cells(p) == PUZ_UNKNOWN) {
            children.emplace_back(*this).make_move(p, n);
            if (found) break;
        }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << format("{:02}", cells({r, c})) << " ";
        println(out);
    }
    return out;
}

}

void solve_puz_HiddenPath()
{
    using namespace puzzles::HiddenPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HiddenPath/HiddenPath.xml", "Puzzles/HiddenPath/HiddenPath.txt", solution_format::GOAL_STATE_ONLY);
}

void solve_puz_HiddenPathTest()
{
    using namespace puzzles::HiddenPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, false, true>>(
        "../Test.xml", "../Test.txt", solution_format::GOAL_STATE_ONLY);
}
