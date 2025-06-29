#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 8/Warehouse

    Summary
    Packed with crates

    Description
    1. The board represents a warehouse, packed with boxes which you must find.
    2. Each box contains a symbol:
    3. a cross means it's a square box.
    4. a horizontal bar means the box is wider than taller.
    5. a vertical bar means the box is taller than wider.
    6. A grid dot must not be shared by the corners of four boxes.
*/

namespace puzzles::Warehouse{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_HORZ = 'H';
constexpr auto PUZ_VERT = 'V';
constexpr auto PUZ_CROSS = '+';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset3[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

// top-left and bottom-right
using puz_box = pair<Position, Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2symbol;
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
            if (char ch = str[c]; ch != ' ')
                m_pos2symbol[{r, c}] = ch;
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
                                if (auto it = m_pos2symbol.find(p); it != m_pos2symbol.end()) {
                                    rng.push_back(p);
                                    // 2. Each box contains a symbol:
                                    // 3. a cross means it's a square box.
                                    // 4. a horizontal bar means the box is wider than taller.
                                    // 5. a vertical bar means the box is taller than wider.
                                    if (char ch = it->second; rng.size() > 1 ||
                                        !(ch == PUZ_VERT && h > w ||
                                            ch == PUZ_HORZ && h < w ||
                                            ch == PUZ_CROSS && h == w))
                                        return false;
                                }
                            }
                        return rng.size() == 1;
                    }()) {
                        int n = m_boxes.size();
                        m_boxes.emplace_back(tl, br);
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c)
                                m_pos2boxids[{r, c}].push_back(n);
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
    bool check_four_boxes();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the symbol
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2boxids)
{
    find_matches(true);
}

// 6. A grid dot must not be shared by the corners of four boxes.
bool puz_state::check_four_boxes()
{
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            vector<char> v;
            Position p(r, c);
            for (auto& os : offset3)
                v.push_back(cells(p + os));
            // 0 1
            // 2 3
            if (v[0] != v[1] && v[0] != v[2] && v[3] != v[1] && v[3] != v[2])
                return false;
        }
    return true;
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
                return make_move2(box_ids[0]), 1;
            }
    }
    return check_four_boxes() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
    auto& [tl, br] = m_game->m_boxes[n];
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c) {
            Position p(r, c);
            cells(p) = m_ch, ++m_distance, m_matches.erase(p);
        }
    ++m_ch;
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
    for (int n : box_ids)
        if (children.push_back(*this); !children.back().make_move(n))
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
            if (auto it = m_game->m_pos2symbol.find(p); it == m_game->m_pos2symbol.end())
                out << '.';
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Warehouse()
{
    using namespace puzzles::Warehouse;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Warehouse.xml", "Puzzles/Warehouse.txt", solution_format::GOAL_STATE_ONLY);
}
