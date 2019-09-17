#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 2/Digital Path

    Summary
    Nurikabe for robots

    Description
    1. Fill some tiles with numbers. The numbers form a Nurikabe, that is
       a path interconnected horizontally or vertically and which can' t
       cover a 2x2 area.
    2. All numbers in an area must be the same and all of them must be
       equal to the number of those numbers in the area.
    3. All regions must have at least one number.
    4. Two orthogonally adjacent tiles across areas must be different.
*/

namespace puzzles::DigitalPath{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_UNKNOWN      -1

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

const Position offset3[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
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
    string m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    // key.key : number of the numbers
    // key.value : size of the area
    // all permutations
    map<pair<int, int>, vector<string>> m_info2perms;
    map<int, vector<int>> m_area2permids;
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
        if (walls.count(p_wall) == 0) {
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
        area.m_rng.assign(smoves.begin(), smoves.end());
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            if (char ch = cells(p); ch != PUZ_SPACE)
                area.m_num = ch - '0';
        }
    }

    for (auto& area : m_areas) {
        int sz = area.size(), num = area.m_num;
        auto& perms = m_info2perms[{num, sz}];
        if (!perms.empty()) continue;
        for (int n = 1; n <= sz; ++n) {
            if (num != PUZ_UNKNOWN && n != num) continue;
            auto perm = string(sz - n, PUZ_EMPTY) + string(n, char(n + '0'));
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
        m_area2permids[i] = perm_ids;
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
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);
    bool is_continuous() const;
    bool check_2x2();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start), m_matches(g.m_area2permids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;
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
            // 4. Two orthogonally adjacent tiles across areas must be different.
            for (int k = 0; k < sz; ++k) {
                auto& p = rng[k];
                auto ch1 = perm[k];
                if (ch1 == PUZ_SPACE || ch1 == PUZ_EMPTY) continue;
                for (auto& os : offset)
                    if (auto p2 = p + os; is_valid(p2) && area_id != m_game->m_pos2area.at(p2) && ch1 == cells(p2))
                        return true;
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
    return is_continuous() && check_2x2() ? 2 : 0;
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
            if (ch != PUZ_EMPTY)
                a.insert(p);
        }

    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

// A Nurikabe can't cover a 2x2 area.
bool puz_state::check_2x2()
{
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (boost::algorithm::all_of(offset3, [&](const Position& os) {
                char ch = cells(p + os);
                return ch != PUZ_SPACE && ch != PUZ_EMPTY;
            }))
                return false;
        }
    return true;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    int sz = area.size(), num = area.m_num;
    auto& range = area.m_rng;
    auto& perm = m_game->m_info2perms.at({num, sz})[j];

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
            out << (m_game->m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_DigitalPath()
{
    using namespace puzzles::DigitalPath;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DigitalPath.xml", "Puzzles/DigitalPath.txt", solution_format::GOAL_STATE_ONLY);
}
