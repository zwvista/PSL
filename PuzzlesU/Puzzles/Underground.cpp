#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 2/Underground

    Summary
    Subway entrances

    Description
    1. Each neighbourhood contains one entrance to the Underground.
    2. For each entrance there is a corresponding entrance in a different neighbourhood.
    3. The arrows of two corresponding entrances must point to each other.
    4. Between two corresponding entrances there cannot be any other entrance.
    5. Two corresponding entrances cannot be in adjacent neighbourhood, i.e.
       there must be at least one neighbourhood between them.
*/

namespace puzzles::Underground{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

constexpr string_view dirs = "^>v<";

using puz_area = vector<Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    map<Position, char> m_pos2dir;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_horz_walls, * m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
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
, m_sidelen(strs.size() / 2 - 1)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            m_cells.push_back(ch);
            if (ch != PUZ_SPACE)
                m_pos2dir[p] = ch;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_areas.push_back({smoves.begin(), smoves.end()});
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
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

    const puz_game* m_game = nullptr;
    string m_cells;
    map<pair<Position, int>, vector<pair<Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, infos] : m_matches) {
        //boost::remove_erase_if(infos, [&](const pair<Position, int>& info) {
        //    auto& perm = m_game->m_pos2perms.at(info.first)[info.second];
        //    for (int i = 0, sz = perm.size(); i < sz; ++i) {
        //        auto& p2 = perm[i];
        //        int dt = dots(p2);
        //        if (i < sz - 1 && !m_matches.contains({p2, PUZ_FROM}) ||
        //            i > 0 && !m_matches.contains({p2, PUZ_TO}))
        //            return true;
        //    }
        //    return false;
        //});

        if (!init)
            switch(infos.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(infos[0].first, infos[0].second), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    //auto& perm = m_game->m_pos2perms.at(p)[n];
    //for (int i = 0, sz = perm.size(); i < sz; ++i) {
    //    auto& p2 = perm[i];
    //    auto f = [&](const Position& p3, int fromto) {
    //        int dir = boost::find(offset, p3 - p2) - offset;
    //        dots(p2) |= 1 << dir;
    //        dots(p3) |= 1 << (dir + 2) % 4;
    //        m_matches.erase({p2, fromto}), ++m_distance;
    //    };
    //    if (i < sz - 1)
    //        f(perm[i + 1], PUZ_FROM);
    //    if (i > 0)
    //        f(perm[i - 1], PUZ_TO);
    //}
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
    auto& [p, infos] = *boost::min_element(m_matches, [](
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv1,
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& info : infos)
        if (children.push_back(*this); !children.back().make_move(info.first, info.second))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({ r, c }) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Underground()
{
    using namespace puzzles::Underground;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Underground.xml", "Puzzles/Underground.txt", solution_format::GOAL_STATE_ONLY);
}
