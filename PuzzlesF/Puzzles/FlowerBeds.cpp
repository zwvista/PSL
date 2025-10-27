#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 2/FlowerBeds

    Summary
    Reverse Gardener

    Description
    1. The board represents a garden where flowers are scattered around.
    2. Your task as a gardener is to divide the garden in rectangular (or square)
       flower beds.
    3. Each flower bed should contain exactly one flower.
    4. Contiguous flower beds can't have the same area extension.
    5. Green squares are hedges that can't be included in flower beds.
*/

namespace puzzles::FlowerBeds{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_FLOWER = 'F';
constexpr auto PUZ_HEDGE = 'H';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_box
{
    int m_area;
    pair<Position, Position> m_box;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    vector<puz_box> m_boxes;
    map<Position, vector<int>> m_pos2boxids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());

    for (int r1 = 0; r1 < m_sidelen; ++r1)
        for (int c1 = 0; c1 < m_sidelen; ++c1)
            for (int h = 1; h <= m_sidelen - r1; ++h)
                for (int w = 1; w <= m_sidelen - c1; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r1, c1), br = tl + box_sz;
                    auto& [r2, c2] = br;
                    if (vector<Position> rng; [&] {
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c)
                                switch (Position p(r, c); cells(p)) {
                                case PUZ_HEDGE:
                                    // 5. Green squares are hedges that can't be included in flower beds.
                                    return false;
                                case PUZ_FLOWER:
                                    // 3. Each flower bed should contain exactly one flower.
                                    rng.push_back(p);
                                    if (rng.size() > 1)
                                        return false;
                                }
                        return rng.size() == 1;
                    }()) {
                        int n = m_boxes.size();
                        m_boxes.push_back({h * w, {tl, br}});
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c)
                                m_pos2boxids[{r, c}].push_back(n);
                    }
                }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
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

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    map<char, int> m_ch2boxid;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
, m_matches(g.m_pos2boxids)
{
    boost::replace_all(m_cells, string(1, PUZ_FLOWER), string(1, PUZ_SPACE));

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, box_ids] : m_matches) {
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& [area, box] = m_game->m_boxes[id];
            auto& [tl, br] = box;
            auto& [r1, c1] = tl;
            auto& [r2, c2] = br;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c) {
                    Position p(r, c);
                    if (cells(p) != PUZ_SPACE)
                        return true;
                    // 4. Contiguous flower beds can't have the same area extension.
                    for (auto& os : offset) {
                        auto p2 = p + os;
                        if (!is_valid(p2)) continue;
                        if (char ch = cells(p2); ch != PUZ_SPACE && ch != PUZ_HEDGE &&
                            area == m_game->m_boxes[m_ch2boxid.at(ch)].m_area)
                            return true;
                    }
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
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [tl, br] = m_game->m_boxes[n].m_box;
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c) {
            Position p(r, c);
            cells(p) = m_ch, ++m_distance, m_matches.erase(p);
        }
    m_ch2boxid[m_ch++] = n;
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
        if (!children.emplace_back(*this).make_move(n))
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
            out << m_game->cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FlowerBeds()
{
    using namespace puzzles::FlowerBeds;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FlowerBeds.xml", "Puzzles/FlowerBeds.txt", solution_format::GOAL_STATE_ONLY);
}
