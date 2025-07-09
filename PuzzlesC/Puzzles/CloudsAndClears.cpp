#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/Clouds and Clears

    Summary
    Holes in the sky

    Description
    1. Paint the clouds according to the numbers.
    2. Each cloud or empty Sky patch contains a single number that is the extension of the region
       itself.
    3. On a region there can be other numbers. These will indicate how many empty (non-cloud) tiles
       around it (diagonal too) including itself.
*/

namespace puzzles::CloudsAndClears{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

struct puz_move
{
    int m_dir; // direction to the next stone
    Position m_to;
    set<Position> m_on_path; // stones on the path
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool operator<(const puz_state& x) const {
        return tie(m_stone2move_ids, m_path) < tie(x.m_stone2move_ids, x.m_path);
    }
    bool make_move(const Position& p, int n);

    //solve_puzzle interface
    // 6. The goal is to pick up every stone.
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_stone2move_ids.size(); }
    unsigned int get_distance(const puz_state& child) const {
        return m_stone2move_ids.empty() ? 2 : 1;
    }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_stone2move_ids;
    vector<Position> m_path; // path from the first stone to the current stone
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
}

bool puz_state::make_move(const Position& p, int n)
{
    return true; // move successful
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto f = [&](const Position& p) {
        for (int n : m_stone2move_ids.at(p))
            if (children.push_back(*this); !children.back().make_move(p, n))
                children.pop_back();
    };
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0; r < sidelen(); ++r) {
    //    for (int c = 0; c < sidelen(); ++c)
    //        if (Position p(r, c); !m_game->m_stones.contains(p))
    //            out << ".. ";
    //        else {
    //            int n = boost::find(m_path, p) - m_path.begin() + 1;
    //            out << format("{:2} ", n);
    //        }
    //    println(out);
    //}
    return out;
}

}

void solve_puz_CloudsAndClears()
{
    using namespace puzzles::CloudsAndClears;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CloudsAndClears.xml", "Puzzles/CloudsAndClears.txt", solution_format::GOAL_STATE_ONLY);
}
