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

#define PUZ_SPACE        ' '
#define PUZ_FLOWER       'F'
#define PUZ_HEDGE        'H'

const Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

typedef pair<Position, Position> puz_box;

struct puz_game    
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<puz_box> m_boxes;
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
                    if (rng.size() != 1) continue;
                    int n = m_boxes.size();
                    m_boxes.emplace_back(tl, br);
                    for (int r2 = tl.first; r2 <= br.first; ++r2)
                        for (int c2 = tl.second; c2 <= br.second; ++c2)
                            m_pos2boxids[{r2, c2}].push_back(n);
                next:;
                }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, vector<int>> m_matches;
    map<char, int> m_ch2boxid;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    m_matches = g.m_pos2boxids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            auto& box = m_game->m_boxes[id];
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            return false;
        });

        if (!init)
            switch(kv.second.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(kv.first, kv.second.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto f = [](const puz_box& box) {
        return Position(box.second.first - box.first.first, box.second.second - box.first.second);
    };

    auto& box = m_game->m_boxes[n];
    auto& tl = box.first, & br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c) {
            Position p2(r, c);
            cells(p) = m_ch, ++m_distance;
            // Contiguous flower beds can't have the same area extension.
            for (auto& os : offset) {
                auto p3 = p2 + os;
                if (!is_valid(p3)) continue;
                char ch = cells(p3);
                if (ch != PUZ_SPACE && ch != m_ch &&
                    f(box) == f(m_game->m_boxes[m_ch2boxid.at(ch)]))
                    return false;
            }
        }
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);

    m_ch2boxid[m_ch++] = n;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
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
        if (!children.back().make_move(kv.first, n))
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
            out << m_game->cells(p);
        }
        out << endl;
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
