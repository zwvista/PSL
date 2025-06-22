#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 4/Assembly Instructions

    Summary
    New screw legs 'A' to seat 'C' using bolts 'J'...

    Description
    1. Divide the board so that every letter corresponds to a 'part' which
       has the same shape and orientation everywhere it is found.
    2. So for example if letter 'A' is a 2x3 rectangle, every 'A' on the board
       will correspond to a 2x3 rectangle and 'A' will appear in the same position
       in the rectangle itself.
    3. If letter 'B' has an L shape with the letter on the top left, every 'B'
       will have an L shape with the letter on the top left, etc.
*/

namespace puzzles::AssemblyInstructions{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

// top-left and bottom-right
using puz_area = vector<vector<Position>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2letter;
    map<char, vector<Position>> m_letter2rng;
    vector<puz_area> m_areas;
    map<Position, vector<int>> m_pos2area_ids;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<vector<Position>>
{
    puz_state2(const puz_game& game, const vector<Position>& rng)
        : vector<vector<Position>>(rng.size()), m_game(&game) { make_move(rng); }

    bool is_goal_state() const { return true; }
    void make_move(const vector<Position>& rng) {
        for (int i = 0; i < rng.size(); ++i)
            (*this)[i].push_back(rng[i]);
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    int sz = front().size();
    for (int j = 0; j < sz; ++j)
        for (auto& os : offset) {
            vector<Position> rng(sz);
            for (int i = 0; i < size(); ++i)
                // Areas extend horizontally or vertically
                rng.push_back((*this)[i][j] + os);
            if (boost::algorithm::all_of(rng, [&](const Position& p) {
                // An adjacent tile can be occupied by the area
                // if it is a space tile and has not been occupied by the area
                return m_game->cells(p) == PUZ_SPACE ||
                    boost::algorithm::none_of(*this, [&](const vector<Position>& rng2) {
                        return boost::algorithm::none_of_equal(rng2, p);
                    });
            })) {
                children.push_back(*this);
                children.back().make_move(rng);
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char name = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                m_pos2letter[p] = ch;
                m_letter2rng[ch].push_back(p);
                m_start.push_back(name++);
            }
            m_start.push_back(PUZ_BOUNDARY);
        }
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [letter, rng] : m_letter2rng) {
        puz_state2 sstart(*this, rng);
        list<list<puz_state2>> spaths;
        // Areas can have any form.
        puz_solver_bfs<puz_state2, false, false, false>::find_solution(sstart, spaths);
        // save all goal states as permutations
        // A goal state is an area formed from the letter(s)
        for (auto& spath : spaths) {
            int n = m_areas.size();
            puz_area area(spath.back().begin(), spath.back().end());
            m_areas.push_back(area);
            for (auto& rng : area)
                for (auto& p : rng)
                    m_pos2area_ids[p].push_back(n);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
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
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2area_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, box_ids] : m_matches) {
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
    return 2;
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
    auto& [p, box_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : box_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2letter.find(p); it == m_game->m_pos2letter.end())
                out << ".";
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_AssemblyInstructions()
{
    using namespace puzzles::AssemblyInstructions;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/AssemblyInstructions.xml", "Puzzles/AssemblyInstructions.txt", solution_format::GOAL_STATE_ONLY);
}
