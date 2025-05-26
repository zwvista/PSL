#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Landscapes

    Summary
    Forests, Deserts, Oceans, Mountains

    Description
    1. Identify the landscape in every region, choosing between trees, sand,
       water and rocks.
    2. Two regions can't have the same landscape if they touch, not even
       diagonally.
*/

namespace puzzles::Landscapes{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},         // e
    {1, 1},         // se
    {1, 0},         // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},       // nw
};

constexpr Position offset2[] = {
    {0, 0},         // n
    {0, 1},         // e
    {1, 0},         // s
    {0, 0},         // w
};

struct puz_region
{
    vector<Position> m_rng;
    vector<char> m_kind = {'T', 'S', 'W', 'R'};
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_region> m_regions;
    map<Position, int> m_pos2region;
    set<Position> m_horz_walls, m_vert_walls;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i * 2];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            m_start.push_back(str_v[c * 2 + 1]);
            rng.insert(p);
        }
    }
    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        auto p0 = *rng.begin();
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, p0}, smoves);
        puz_region region;
        for (auto& p : smoves) {
            m_pos2region[p] = n;
            rng.erase(p);
            region.m_rng.push_back(p);
        }
        char ch = cells(p0);
        if (ch != PUZ_SPACE)
            region.m_kind = {ch};
        m_regions.push_back(region);
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n, char ch);
    void identify_region(int n, char ch);
    bool find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, int> m_painting_counts;
    // key: index of the region
    // value.elem: possible kinds of the region
    map<int, vector<char>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start)
    , m_game(&g)
{
    vector<int> fixed_regions;
    for (int i = 0; i < g.m_regions.size(); ++i) {
        auto& region = g.m_regions[i];
        if (region.m_kind.size() == 1)
            fixed_regions.push_back(i);
        else
            m_matches[i] = region.m_kind;
    }
    for (int i : fixed_regions)
        identify_region(i, g.m_regions[i].m_kind[0]);
}

void puz_state::identify_region(int n, char ch)
{
    for (auto& p : m_game->m_regions[n].m_rng)
        for (auto& os : offset) {
            auto p2 = p + os;
            auto it = m_game->m_pos2region.find(p2);
            if (it == m_game->m_pos2region.end()) continue;
            int n2 = it->second;
            if (n == n2) continue;
            auto it2 = m_matches.find(n2);
            if (it2 == m_matches.end()) continue;
            boost::remove_erase(it2->second, cells(p));
        }
}

bool puz_state::make_move(int n, char ch)
{
    m_distance = 0;
    auto& region = m_game->m_regions[n];
    for (auto& p : region.m_rng)
        cells(p) = ch, ++m_distance;
    identify_region(n, ch);
    m_matches.erase(n);
    return boost::algorithm::none_of(m_matches, [](const pair<const int, vector<char>>& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<char>>& kv1,
        const pair<const int, vector<char>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Landscapes()
{
    using namespace puzzles::Landscapes;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Landscapes.xml", "Puzzles/Landscapes.txt", solution_format::GOAL_STATE_ONLY);
}
