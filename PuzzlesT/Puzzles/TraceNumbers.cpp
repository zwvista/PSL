#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Trace Numbers

    Summary
    Trace Numbers

    Description
    1. On the board there are a few number sets. Those numbers are
       sequences all starting from 1 up to a number N.
    2. You should draw as many lines into the grid as number sets:
       a line starts with the number 1, goes through the numbers in
       order up to the highest, where it ends.
    3. In doing this, you have to pass through all tiles on the board.
       Lines cannot cross.
*/

namespace puzzles::TraceNumbers{

#define PUZ_SPACE        ' '

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, vector<Position>> m_ch2rng;
    string m_start;
    char m_max_ch;
    map<Position, vector<vector<Position>>> m_pos2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p, char ch)
        : m_game(&game), m_char(ch) {
        make_move(p);
    }

    bool is_goal_state() const { return m_game->cells(back()) == m_char + 1; }
    void make_move(const Position& p) { push_back(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    char m_char;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& p = back();
    for (auto& os : offset) {
        auto p2 = p + os;
        if (!m_game->is_valid(p2) || boost::find(*this, p2) != end()) continue;
        char ch2 = m_game->cells(p2);
        if (ch2 == PUZ_SPACE || ch2 == m_char + 1) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_ch2rng[ch].emplace_back(r, c);
    }
    m_max_ch = m_ch2rng.rbegin()->first;

    for (auto&& [ch, rng] : m_ch2rng) {
        if (ch == m_max_ch) break;
        for (auto& p : rng) {
            puz_state2 sstart(*this, p, ch);
            list<list<puz_state2>> spaths;
            puz_solver_bfs<puz_state2, true, false, false>::find_solution(sstart, spaths);
            // save all goal states as permutations
            // A goal state is a line starting from N to N+1
            auto& perms = m_pos2perms[p];
            for (auto& spath : spaths)
                perms.push_back(spath.back());
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    bool check_four_boxes();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    set<Position> m_horz_walls, m_vert_walls;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
//, m_matches(g.m_pos2infoids)
{
    find_matches(true);
}
    
bool puz_state::check_four_boxes()
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            if (m_horz_walls.count({r + 1, c}) == 1 &&
                m_horz_walls.count({r + 1, c + 1}) == 1 &&
                m_vert_walls.count({r, c + 1}) == 1 &&
                m_vert_walls.count({r + 1, c + 1}) == 1)
                return false;
    return true;
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            return false;
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(box_ids[0]), 1;
            }
    }
    return check_four_boxes() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen()) break;
            out << m_game->cells({r, c});
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_TraceNumbers()
{
    using namespace puzzles::TraceNumbers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TraceNumbers.xml", "Puzzles/TraceNumbers.txt", solution_format::GOAL_STATE_ONLY);
}
