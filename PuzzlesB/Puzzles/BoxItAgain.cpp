#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 15/Box It Again

    Summary
    Harder Boxes

    Description
    1. Just like Box It Up, you have to divide the Board in Boxes (Rectangles).
    2. Each Box must contain one number and the number represents the area of
       that Box.
    3. However this time, some tiles can be left unboxed, the board isn't 
       entirely covered by boxes.
*/

namespace puzzles::BoxItAgain{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

// top-left and bottom-right
using puz_box = pair<Position, Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_box> m_boxes;
    map<Position, vector<int>> m_pos2boxids;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            if (ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
        }
    }

    for (int r1 = 0; r1 < m_sidelen; ++r1)
        for (int c1 = 0; c1 < m_sidelen; ++c1)
            for (int h = 1; h <= m_sidelen - r1; ++h)
                for (int w = 1; w <= m_sidelen - c1; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r1, c1), br = tl + box_sz;
                    auto& [r2, c2] = br;
                    if (vector<Position> rng; [&] {
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c) {
                                Position p(r, c);
                                if (auto it = m_pos2num.find(p); it != m_pos2num.end()) {
                                    rng.push_back(p);
                                    // 2. Each Box must contain one number and the number represents the area of
                                    // that Box.
                                    if (rng.size() > 1 || it->second != h * w)
                                        return false;
                                }
                            }
                        return rng.size() == 1;
                    }()) {
                        int n = m_boxes.size();
                        m_boxes.emplace_back(tl, br);
                        // 3. However this time, some tiles can be left unboxed, the board isn't 
                        // entirely covered by boxes.
                        m_pos2boxids[rng.front()].push_back(n);
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

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2boxids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, box_ids] : m_matches) {
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& [tl, br] = m_game->m_boxes[id];
            auto& [r1, c1] = tl;
            auto& [r2, c2] = br;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            return false;
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, box_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [tl, br] = m_game->m_boxes[n];
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c)
            cells({r, c}) = m_ch;
    ++m_ch, ++m_distance;
    m_matches.erase(p);
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
    auto& [p, box_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : box_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
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
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ".";
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BoxItAgain()
{
    using namespace puzzles::BoxItAgain;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BoxItAgain.xml", "Puzzles/BoxItAgain.txt", solution_format::GOAL_STATE_ONLY);
}
