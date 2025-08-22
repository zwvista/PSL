#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 4/Mixed Tatamis

    Summary
    Just that long

    Description
    1. Divide the board into areas (Tatami).
    2. Every Tatami must be exactly one cell wide and can be of length
       from 1 to 4 cells.
    3. A cell with a number indicates the length of the Tatami. Not all
       Tatamis have to be marked by a number.
    4. Two Tatamis of the same size must not be orthogonally adjacent.
    5. A grid dot must not be shared by the corners of four Tatamis
       (lines can't form 4-way crosses).
*/

namespace puzzles::MixedTatamis{

constexpr auto PUZ_SPACE = ' ';

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::Square2x2Offset;

struct puz_box
{
    // the length of the tatami
    char m_ch;
    // top-left and bottom-right
    pair<Position, Position> m_box;
};

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
                m_pos2num[{r, c}] = ch - '0';
    }

    for (int r1 = 0; r1 < m_sidelen; ++r1)
        for (int c1 = 0; c1 < m_sidelen; ++c1)
            for (int i = 1; i <= 4; ++i)
                for (int j = 0; j < 2; ++j) {
                    int h = i, w = 1;
                    if (j == 1) ::swap(h, w);
                    Position box_sz(h - 1, w - 1);
                    Position tl(r1, c1), br = tl + box_sz;
                    auto& [r2, c2] = br;
                    if (!(r2 < m_sidelen && c2 < m_sidelen)) continue;
                    if (vector<Position> rng; [&] {
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c) {
                                Position p(r, c);
                                if (auto it = m_pos2num.find(p); it != m_pos2num.end()) {
                                    rng.push_back(p);
                                    // 3. A cell with a number indicates the length of the Tatami.
                                    // Not all Tatamis have to be marked by a number.
                                    if (rng.size() > 1 || it->second != i)
                                        return false;
                                }
                            }
                        return true;
                    }()) {
                        int n = m_boxes.size();
                        m_boxes.push_back({char(i + '0'), {tl, br}});
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

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2boxids)
{
    find_matches(true);
}

// 5. A grid dot must not be shared by the corners of four Tatamis
// (lines can't form 4-way crosses).
bool puz_state::check_four_boxes()
{
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            vector<char> v;
            Position p(r, c);
            for (auto& os : offset2)
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
            auto& [ch, box] = m_game->m_boxes[id];
            auto& [tl, br] = box;
            auto& [r1, c1] = tl;
            auto& [r2, c2] = br;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c) {
                    Position p(r, c);
                    if (cells(p) != PUZ_SPACE)
                        return true;
                    // 4. Two Tatamis of the same size must not be orthogonally adjacent.
                    for (auto& os : offset)
                        if (auto p2 = p + os; is_valid(p2) && cells(p2) == ch)
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
    return check_four_boxes() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
    auto& [ch, box] = m_game->m_boxes[n];
    auto& [tl, br] = box;
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c) {
            Position p(r, c);
            cells(p) = ch, ++m_distance, m_matches.erase(p);
        }
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
            //out << cells(p);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << '.';
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_MixedTatamis()
{
    using namespace puzzles::MixedTatamis;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MixedTatamis.xml", "Puzzles/MixedTatamis.txt", solution_format::GOAL_STATE_ONLY);
}
