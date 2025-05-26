#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 2/FlowerBeds

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

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_box_info
{
    int m_area;
    pair<Position, Position> m_box;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<puz_box_info> m_boxinfos;
    map<Position, vector<int>> m_pos2boxids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (int h = 1; h <= m_sidelen - r; ++h)
                for (int w = 1; w <= m_sidelen - c; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r, c), br = tl + box_sz;
                    vector<Position> rng;
                    for (int r2 = tl.first; r2 <= br.first; ++r2)
                        for (int c2 = tl.second; c2 <= br.second; ++c2) {
                            Position p(r2, c2);
                            if (char ch = cells(p); ch == PUZ_FLOWER) {
                                rng.push_back(p);
                                if (rng.size() > 1)
                                    goto next;
                            }
                            else if (ch == PUZ_HEDGE)
                                goto next;
                        }
                    if (rng.size() == 1) {
                        int n = m_boxinfos.size();
                        puz_box_info info;
                        info.m_area = h * w;
                        info.m_box = {tl, br};
                        m_boxinfos.push_back(info);
                        for (int r2 = tl.first; r2 <= br.first; ++r2)
                            for (int c2 = tl.second; c2 <= br.second; ++c2)
                                m_pos2boxids[{r2, c2}].push_back(n);
                    }
                next:;
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

    const puz_game* m_game = nullptr;
    string m_cells;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, vector<int>> m_matches;
    map<char, int> m_ch2boxid;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_start)
, m_matches(g.m_pos2boxids)
{
    boost::replace_all(m_cells, string(1, PUZ_FLOWER), string(1, PUZ_SPACE));

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            auto& info = m_game->m_boxinfos[id];
            auto& box = info.m_box;
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c) {
                    Position p(r, c);
                    if (cells(p) != PUZ_SPACE)
                        return true;
                    // Contiguous flower beds can't have the same area extension.
                    for (auto& os : offset) {
                        auto p2 = p + os;
                        if (!is_valid(p2)) continue;
                        if (char ch = cells(p2); ch != PUZ_SPACE && ch != PUZ_HEDGE &&
                            info.m_area == m_game->m_boxinfos[m_ch2boxid.at(ch)].m_area)
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
    auto& info = m_game->m_boxinfos[n];
    auto& box = info.m_box;
    auto& tl = box.first, & br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c) {
            Position p(r, c);
            cells(p) = m_ch, ++m_distance, m_matches.erase(p);
        }
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);

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
            out << (m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.contains(p) ? '|' : ' ');
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
