#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/The City Rises

    Summary
    City Planner Revenge

    Description
    1. The board represents a piece of land where a new town should be built.
    2. Each area describes a section of land, where the town concil has decided
       to place as many city blocks as the number in it.
    3. Town blocks inside an area are horizontally or vertically contiguous.
    4. Blocks in different areas cannot touch horizontally or vertically.
    5. Areas without number can have any number of blocks, but there can't be
       empty areas.
    6. Lastly, two neighbouring areas can't have the same number of blocks in them.
*/

namespace puzzles::TheCityRises{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_BLOCK = 'X';
constexpr auto PUZ_UNKNOWN = -1;

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
    
struct puz_area
{
    int m_num = PUZ_UNKNOWN;
    vector<Position> m_rng;
    int size() const { return m_rng.size(); }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    // key.key : number of the blocks
    // key.value : size of the area
    // value : all permutations
    map<pair<int, int>, vector<string>> m_info2perms;
    map<int, vector<int>> m_area2permids;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
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

struct puz_state3 : Position
{
    puz_state3(const set<Position>& a, const Position& p_start)
    : m_area(&a) { make_move(p_start); }
    
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;
    
    const set<Position>* m_area;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->contains(p)) {
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
            if (char ch = str_v[c * 2 + 1]; ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& area = m_areas.emplace_back();
        area.m_rng.assign(smoves.begin(), smoves.end());
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                area.m_num = it->second;
        }
    }

    for (auto& area : m_areas) {
        int sz = area.size(), num = area.m_num;
        auto& perms = m_info2perms[{num, sz}];
        if (!perms.empty()) continue;
        // 5. Areas without number can have any number of blocks, but there can't be
        // empty areas.
        for (int n = 1; n <= sz; ++n) {
            if (num != PUZ_UNKNOWN && n != num) continue;
            auto perm = string(sz - n, PUZ_EMPTY) + string(n, PUZ_BLOCK);
            do
                perms.push_back(perm);
            while (boost::next_permutation(perm));
        }
    }

    for (int i = 0; i < m_areas.size(); ++i) {
        auto& area = m_areas[i];
        int sz = area.size(), num = area.m_num;
        auto& rng = area.m_rng;
        auto& perms = m_info2perms.at({num, sz});
        vector<int> perm_ids(perms.size());
        boost::iota(perm_ids, 0);
        // 3. Town blocks inside an area are horizontally or vertically contiguous.
        boost::remove_erase_if(perm_ids, [&](int j) {
            auto& perm = perms[j];
            set<Position> a;
            for (int k = 0; k < sz; ++k)
                if (perm[k] == PUZ_BLOCK)
                    a.insert(rng[k]);
            auto& p = rng[perm.find(PUZ_BLOCK)];
            auto smoves = puz_move_generator<puz_state3>::gen_moves({a, p});
            return smoves.size() != a.size();
        });
        m_area2permids[i] = perm_ids;
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
    bool make_move(int i, int j);
    void make_move2(int i, int j);
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
    map<int, vector<int>> m_matches;
    map<int, int> m_area2num;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_matches(g.m_area2permids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& area = m_game->m_areas[area_id];
        int sz = area.size(), num = area.m_num;
        auto& rng = area.m_rng;
        auto& perms = m_game->m_info2perms.at({num, sz});

        string chars;
        for (auto& p : rng)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // 4. Blocks in different areas cannot touch horizontally or vertically.
            // 6. Two neighbouring areas can't have the same number of blocks in them.
            for (int k = 0; k < sz; ++k) {
                auto& p = rng[k];
                auto ch1 = perm[k];
                int num1 = boost::count(perm, PUZ_BLOCK);
                for (auto& os : offset) {
                    auto p2 = p + os;
                    if (!is_valid(p2)) continue;
                    char ch2 = cells(p2);
                    int area_id2 = m_game->m_pos2area.at(p2);
                    if (area_id == area_id2) continue;
                    if (ch1 == PUZ_BLOCK && ch2 == PUZ_BLOCK)
                        return true;
                    if (auto it = m_area2num.find(area_id2); it != m_area2num.end() && it->second == num1)
                        return true;
                }
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    int sz = area.size(), num = area.m_num;
    auto& range = area.m_rng;
    auto& perm = m_game->m_info2perms.at({num, sz})[j];

    for (int k = 0; k < sz; ++k)
        cells(range[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
    m_area2num[i] = boost::count(perm, PUZ_BLOCK);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(area_id, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
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

void solve_puz_TheCityRises()
{
    using namespace puzzles::TheCityRises;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheCityRises.xml", "Puzzles/TheCityRises.txt", solution_format::GOAL_STATE_ONLY);
}
