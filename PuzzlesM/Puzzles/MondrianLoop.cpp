#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 2/Mondrian Loop

    Summary
    Lots of artists around here

    Description
    1. Enough with impressionists, time for a nice geometric painting
       called Squarism!
    2. Divide the board in many rectangles or squares. Each
       rectangle/square can contain only one number, which represents
       its area, but it can also contain none. 
    3. The rectangles/squares can't touch each other with their sides
       (they can't share a side), but they have to form a loop by
       connecting with their corners.
    4. In the end there must be a single loop that connects all
       rectangles/squares by corners.
*/

namespace puzzles::MondrianLoop{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_UNKNOWN_CHAR = 'O';
constexpr auto PUZ_UNKNOWN = -1;

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
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_pos2num[{r, c}] = 
                    ch == PUZ_UNKNOWN_CHAR ? PUZ_UNKNOWN : ch - '0';
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
                                    int num = it->second;
                                    rng.push_back(p);
                                    // 2. Each rectangle/square can contain only one number, which represents
                                    // its area, but it can also contain none. 
                                    if (rng.size() > 1 || num != PUZ_UNKNOWN && num != h * w)
                                        return false;
                                }
                            }
                        return rng.size() == 1;
                    }()) {
                        int n = m_boxes.size();
                        m_boxes.emplace_back(tl, br);
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
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    bool check_mondrian_loop();

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
    vector<puz_box> m_used_boxes;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2boxids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto f = [&](const Position& p) {
        if (!is_valid(p)) return false;
        char ch = cells(p);
        return ch != PUZ_SPACE && ch != PUZ_EMPTY;
    };

    for (auto& [p, box_ids] : m_matches) {
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& [tl, br] = m_game->m_boxes[id];
            auto& [r1, c1] = tl;
            auto& [r2, c2] = br;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            // 3. The rectangles/squares can't touch each other with their sides
            // (they can't share a side), 
            for (int r = r1; r <= r2; ++r) {
                Position p1(r, c1 - 1), p2(r, c2 + 1);
                if (f(p1) || f(p2))
                    return true;
            }
            for (int c = c1; c <= c2; ++c) {
                Position p1(r1 - 1, c), p2(r2 + 1, c);
                if (f(p1) || f(p2))
                    return true;
            }
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
    return check_mondrian_loop() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
    auto& [tl, br] = m_game->m_boxes[n];
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    // 3. The rectangles/squares can't touch each other with their sides
    // (they can't share a side), 
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c) {
            Position p(r, c);
            cells(p) = m_ch, ++m_distance, m_matches.erase(p);
        }
    auto f = [&](const Position& p) {
        if (is_valid(p))
            cells(p) = PUZ_EMPTY;
    };
    for (int r = r1; r <= r2; ++r)
        f({r, c1 - 1}), f({r, c2 + 1});
    for (int c = c1; c <= c2; ++c)
        f({r1 - 1, c}), f({r2 + 1, c});
    ++m_ch;
    m_used_boxes.push_back({tl, br});
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

// 3. The rectangles/squares have to form a loop by
// connecting with their corners.
bool puz_state::check_mondrian_loop()
{
    // A cycle graph where every vertex has a degree of 2 is simply a cycle.
    // In a cycle graph, every vertex is connected to exactly two other vertices,
    // forming a closed loop or circuit. 
    for (auto& [tl, br] : m_used_boxes) {
        auto& [r1, c1] = tl;
        auto& [r2, c2] = br;
        vector<Position> v = {
            {r1 - 1, c1 - 1},
            {r1 - 1, c2 + 1},
            {r2 + 1, c1 - 1},
            {r2 + 1, c2 + 1},
        };
        if (boost::algorithm::any_of(v, [&](const Position& p) {
            return is_valid(p) && cells(p) == PUZ_EMPTY;
        }))
            return false;
        int n = boost::count_if(v, [&](const Position & p) {
            return boost::algorithm::any_of(m_used_boxes, [&](const puz_box& box) {
                auto& [tl2, br2] = box;
                return tl2 == p || br2 == p;
            });
        });
        if (is_goal_state() && n != 2 || n > 2)
            return false;
    }
    if (is_goal_state())
        replace(m_cells.begin(), m_cells.end(), PUZ_SPACE, PUZ_EMPTY);
    return true;
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
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ' ';
            else if (int num = it->second; num == PUZ_UNKNOWN)
                out << PUZ_UNKNOWN_CHAR;
            else
                out << num;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_MondrianLoop()
{
    using namespace puzzles::MondrianLoop;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MondrianLoop.xml", "Puzzles/MondrianLoop.txt", solution_format::GOAL_STATE_ONLY);
}
