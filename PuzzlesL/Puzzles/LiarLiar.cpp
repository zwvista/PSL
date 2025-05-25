#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 2/Liar Liar

    Summary
    Tiles on fire

    Description
    1. Mark some tiles according to these rules:
    2. Cells with numbers are never marked.
    3. A number in a cell indicates how many marked cells must be placed.
       adjacent to its four sides.
    4. However, in each region there is one (and only one) wrong number
       (it shows a wrong amount of marked cells).
    5. Two marked cells must not be orthogonally adjacent.
    6. All of the non-marked cells must be connected.
*/

namespace puzzles::LiarLiar{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_MARKED       'X'

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_hint
{
    int m_area_id, m_hint_id;
    int m_num;
    vector<Position> m_rng;
};
    
struct puz_perm_info
{
    vector<string> m_perms;
    vector<int> m_counts;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // 1st dimension : the index of the area
    // 2nd dimension : the numbers in the area
    // elem : the position of the number
    vector<vector<Position>> m_areas;
    map<Position, puz_hint> m_pos2hint;
    map<int, puz_perm_info> m_size2perminfo;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
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
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        auto& area = m_areas.emplace_back();
        int m = 0;
        for (auto& p : smoves) {
            rng.erase(p);
            if (char ch = cells(p); ch != PUZ_SPACE) {
                area.push_back(p);
                auto& o = m_pos2hint[p];
                o.m_num = ch - '0';
                o.m_area_id = n;
                o.m_hint_id = m++;
            }
        }
    }

    for (auto& area : m_areas)
        for (auto& p : area) {
            auto& o = m_pos2hint.at(p);
            for (auto& os : offset)
                if (auto p2 = p + os; is_valid(p2) && cells(p2) == PUZ_SPACE)
                    o.m_rng.push_back(p2);
            int sz = o.m_rng.size();
            auto& o2 = m_size2perminfo[sz];
            if (!o2.m_perms.empty()) continue;
            for (int i = 0; i <= sz; ++i) {
                o2.m_counts.push_back(o2.m_perms.size());
                auto perm = string(sz - i, PUZ_EMPTY) + string(i, PUZ_MARKED);
                do
                    o2.m_perms.push_back(perm);
                while (boost::next_permutation(perm));
            }
            o2.m_counts.push_back(o2.m_perms.size());
        }
}

struct puz_state
{
    puz_state() {}
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
    bool make_move(int area_id, int liar_id, const Position& p, int n);
    void make_move2(int area_id, int liar_id, const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    map<int, int> m_area2liar;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto&& [p, o] : g.m_pos2hint) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_size2perminfo.at(o.m_rng.size()).m_perms.size());
        boost::iota(perm_ids, 0);
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perm_ids = kv.second;
        auto& o = m_game->m_pos2hint.at(p);
        int sz = m_game->m_areas[o.m_area_id].size();
        auto& rng = o.m_rng;
        int sz2 = rng.size();
        auto& o2 = m_game->m_size2perminfo.at(sz2);
        auto& perms = o2.m_perms;

        string chars;
        for (auto& p : rng)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // 5. Two marked cells must not be orthogonally adjacent.
            for (int k = 0; k < sz2; ++k) {
                auto& p = rng[k];
                if (char ch1 = perm[k]; ch1 == PUZ_MARKED)
                    for (auto& os : offset)
                        if (auto p2 = p + os; is_valid(p2) && ch1 == cells(p2))
                            return true;
            }
            auto it = m_area2liar.find(o.m_area_id);
            if (it != m_area2liar.end()) {
                int liar_id = it->second;
                return (o.m_hint_id == liar_id) ==
                    (o.m_num < o2.m_counts.size() - 1 && id >= o2.m_counts[o.m_num] && id < o2.m_counts[o.m_num + 1]);
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            }
    }
    return is_continuous() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; m_rng->count(p2) != 0) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

bool puz_state::is_continuous() const
{
    set<Position> a;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch != PUZ_MARKED)
                a.insert(p);
        }

    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

void puz_state::make_move2(int area_id, int liar_id, const Position& p, int n)
{
    auto& o = m_game->m_pos2hint.at(p);
    int sz = m_game->m_areas[o.m_area_id].size();
    auto& range = o.m_rng;
    int sz2 = range.size();
    auto& o2 = m_game->m_size2perminfo.at(sz2);
    auto& perm = o2.m_perms[n];

    for (int k = 0; k < perm.size(); ++k)
        cells(range[k]) = perm[k];

    m_area2liar[area_id] = liar_id;
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(int area_id, int liar_id, const Position& p, int n)
{
    m_distance = 0;
    make_move2(area_id, liar_id, p, n);
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
    auto& p = kv.first;
    auto& o = m_game->m_pos2hint.at(p);
    auto& o2 = m_game->m_size2perminfo.at(o.m_rng.size());
    auto f = [&](int liar_id, const vector<int>& perm_ids) {
        for (int n : perm_ids) {
            children.push_back(*this);
            if (!children.back().make_move(o.m_area_id, liar_id, p, n))
                children.pop_back();
        }
    };
    auto it = m_area2liar.find(o.m_area_id);
    if (it != m_area2liar.end())
        f(it->second, kv.second);
    else {
        int sz = m_game->m_areas[o.m_area_id].size();
        for (int liar_id = 0; liar_id < sz; ++liar_id) {
            auto perm_ids = kv.second;
            boost::remove_erase_if(perm_ids, [&](int n) {
                return (o.m_hint_id == liar_id) ==
                    (o.m_num < o2.m_counts.size() - 1 && n >= o2.m_counts[o.m_num] && n < o2.m_counts[o.m_num + 1]);
            });
            f(liar_id, perm_ids);
        }
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

void solve_puz_LiarLiar()
{
    using namespace puzzles::LiarLiar;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LiarLiar.xml", "Puzzles/LiarLiar.txt", solution_format::GOAL_STATE_ONLY);
}
