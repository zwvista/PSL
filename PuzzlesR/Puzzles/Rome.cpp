#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 2/Rome

    Summary
    All roads lead to ...

    Description
    1. All the roads lead to Rome.
    2. Hence you should fill the remaining spaces with arrows and in the
       end, starting at any tile and following the arrows, you should get
       at the Rome icon.
    3. Arrows in an area should all be different, i.e. there can't be two
       similar arrows in an area.
*/

namespace puzzles::Rome{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ROME = 'R';

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

const string tool_dirs = "^>v<";

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    // all permutations
    map<int, vector<string>> m_size2perms;
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
            if (ch != PUZ_ROME)
                rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        m_areas.emplace_back(smoves.begin(), smoves.end());
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }

    for (auto& area : m_areas) {
        int cnt = area.size();
        auto& perms = m_size2perms[cnt];
        if (!perms.empty()) continue;
        vector<int> indexes(cnt);
        string perm(cnt, ' ');
        for (int i = 0; i < cnt;) {
            set<int> s(indexes.begin(), indexes.end());
            if (s.size() == cnt) {
                for (int j = 0; j < cnt; ++j)
                    perm[j] = tool_dirs[indexes[j]];
                perms.push_back(perm);
            }
            for (i = 0; i < cnt && ++indexes[i] == 4; ++i)
                indexes[i] = 0;
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
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);
    bool check_lead_to_rome();

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
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        vector<int> perm_ids(g.m_size2perms.at(g.m_areas[i].size()).size());
        boost::iota(perm_ids, 0);
        m_matches[i] = perm_ids;
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& area = m_game->m_areas[area_id];
        auto& perms = m_game->m_size2perms.at(area.size());

        string chars;
        for (auto& p : area)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return check_lead_to_rome() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state& s, const Position& starting) : m_state(&s) {
        make_move(starting);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    char ch = m_state->cells(*this);
    if (ch == PUZ_SPACE || ch == PUZ_ROME)
        return;
    auto& os = offset[tool_dirs.find(ch)];
    auto p2 = *this + os;
    if (m_state->is_valid(p2)) {
        children.push_back(*this);
        children.back().make_move(p2);
    }
}

bool puz_state::check_lead_to_rome()
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch != PUZ_SPACE && ch != PUZ_ROME)
                rng.insert(p);
        }
    while (!rng.empty()) {
        list<puz_state3> smoves;
        // find all tiles reachable from the first space tile
        puz_move_generator<puz_state3>::gen_moves({*this, *rng.begin()}, smoves);
        if (char ch = cells(smoves.back()); ch != PUZ_SPACE && ch != PUZ_ROME)
            return false;
        for (auto& p : smoves)
            rng.erase(p);
    }
    return true;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_areas[i];
    auto& perm = m_game->m_size2perms.at(range.size())[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(range[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1, 
        const pair<const int, vector<int>>& kv2) {
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

void solve_puz_Rome()
{
    using namespace puzzles::Rome;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Rome.xml", "Puzzles/Rome.txt", solution_format::GOAL_STATE_ONLY);
}
