#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 6/Plug it in

    Summary
    Give them light

    Description
    1. Connect each battery with a lightbulb by a horizontal or vertical cable.
    2. Cables are not allowed to cross other cables.
*/

namespace puzzles::PlugItIn{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_LIGHTBULB = 'L';
constexpr auto PUZ_BATTERY = 'B';
constexpr auto PUZ_HORZ = '-';
constexpr auto PUZ_VERT = '|';
constexpr auto PUZ_BOUNDARY = 'X';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, vector<int>> m_pos2dirs;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; ; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            char ch = str[c - 1];
            m_cells.push_back(ch);
            if (ch != PUZ_SPACE)
                m_pos2dirs[{r * 2 - 1, c * 2 - 1}];
            if (c == m_sidelen / 2) break;
            m_cells.push_back(PUZ_SPACE);
        }
        m_cells.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_cells.push_back(PUZ_BOUNDARY);
        m_cells.append(m_sidelen - 2, PUZ_SPACE);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    
    for (auto& [p, dirs] : m_pos2dirs) {
        char ch = cells(p);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            for (auto p2 = p + os;; p2 += os) {
                char ch2 = cells(p2);
                if (ch2 == PUZ_SPACE) continue;
                if (ch2 != PUZ_BOUNDARY && ch2 != ch)
                    dirs.push_back(i);
                break;
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the island
    // value.elem: the numbers of the bridges connected to the island
    //             in all four directions
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    m_matches = g.m_pos2dirs;
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, dirs] : m_matches) {
        boost::remove_erase_if(dirs, [&](int i){
            auto& os = offset[i];
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE) continue;
                if (ch == PUZ_HORZ || ch == PUZ_VERT)
                    return true;
                break;
            }
            return false;
        });

        if (!init)
            switch(dirs.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, dirs.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    bool is_horz = n % 2 == 1;
    auto& os = offset[n];
    auto p2 = p + os;
    for (; ; p2 += os) {
        char& ch = cells(p2);
        if (ch != PUZ_SPACE)
            break;
        ch = is_horz ? PUZ_HORZ : PUZ_VERT;
    }

    m_distance += 2;
    m_matches.erase(p);
    m_matches.erase(p2);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, dirs] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : dirs)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_PlugItIn()
{
    using namespace puzzles::PlugItIn;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PlugItIn.xml", "Puzzles/PlugItIn.txt", solution_format::GOAL_STATE_ONLY);
}
