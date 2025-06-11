#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 16/Abstract Painting

    Summary
    Abstract Logic

    Description
    1. The goal is to reveal part of the abstract painting behind the board.
    2. Outer numbers tell how many tiles form the painting on the row and column.
    3. The region of the painting can be entirely hidden or revealed.
*/

namespace puzzles::AbstractPainting{

constexpr auto PUZ_PAINTING = 'P';
constexpr auto PUZ_SPACE = '.';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
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
    map<int, int> m_rc2count;
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<int, int> m_painting_counts;
    vector<puz_region> m_regions;
    map<Position, int> m_pos2region;
    map<pair<int, int>, int> m_rc_region2count;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
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
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            rng.insert(p);
        }
    }

    auto f = [&](int rc, char ch) {
        if (ch != ' ')
            m_painting_counts[rc] = ch - '0';
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1][m_sidelen * 2 + 1]);
        f(i + m_sidelen, strs[m_sidelen * 2 + 1][i * 2]);
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        puz_region region;
        for (auto& p : smoves) {
            m_pos2region[p] = n;
            rng.erase(p);
            region.m_rng.push_back(p);
            ++region.m_rc2count[p.first];
            ++region.m_rc2count[p.second + m_sidelen];
        }
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
    bool make_move(int n);
    bool find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_painting_counts, 0,
            [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, int> m_painting_counts;
    // key: index of the row/column
    // value.elem: index of the region that can be revealed
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_painting_counts(g.m_painting_counts)
{
    for (int i = 0; i < g.m_regions.size(); ++i)
        for (auto& [rc, n] : g.m_regions[i].m_rc2count)
            if (m_painting_counts.contains(rc))
                m_matches[rc].push_back(i);

    find_matches(true);
}

bool puz_state::find_matches(bool init)
{
    set<int> hidden_ids;
    for (auto& [rc, region_ids] : m_matches)
        for (int id : region_ids)
            if (m_painting_counts[rc] < m_game->m_regions[id].m_rc2count.at(rc))
                hidden_ids.insert(id);
    for (auto& [rc, region_ids] : m_matches) 
        boost::remove_erase_if(region_ids, [&](int id) {
            return hidden_ids.contains(id);
        });
    for (auto& [rc, n] : m_painting_counts)
        if (n == 0)
            m_matches.erase(rc);
    return boost::algorithm::none_of(m_matches, [](const pair<const int, vector<int>>& kv) {
        return kv.second.empty();
    });
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    auto& region = m_game->m_regions[n];
    for (auto& p : region.m_rng) {
        auto f = [&](int rc) {
            if (m_painting_counts.contains(rc))
                --m_painting_counts[rc], ++m_distance;
        };
        f(p.first);
        f(p.second + sidelen());
        cells(p) = PUZ_PAINTING;
    }
    for (auto& [rc, region_ids] : m_matches)
        boost::remove_erase(region_ids, n);
    return find_matches(false);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [rc, region_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : region_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int rc) {
        int cnt = 0;
        for (int i = 0; i < sidelen(); ++i)
            if (cells(rc < sidelen() ? Position(rc, i) : Position(i, rc - sidelen())) == PUZ_PAINTING)
                ++cnt;
        out << (rc < sidelen() ? format("{:<2}", cnt) : format("{:2}", cnt));
    };
    for (int r = 0;; ++r) {
        // draw horizontal walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) {
                f(r);
                break;
            }
            out << cells(p);
        }
        println(out);
    }
    for (int c = 0; c < sidelen(); ++c)
        f(c + sidelen());
    println(out);
    return out;
}

}

void solve_puz_AbstractPainting()
{
    using namespace puzzles::AbstractPainting;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/AbstractPainting.xml", "Puzzles/AbstractPainting.txt", solution_format::GOAL_STATE_ONLY);
}
