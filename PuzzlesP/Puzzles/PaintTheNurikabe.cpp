#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Paint The Nurikabe

    Summary
    Paint areas, find Nurikabes

    Description
    1. By painting (filling) the areas you have to complete a Nurikabe.
       Specifically:
    2. A number indicates how many painted tiles are adjacent to it.
    3. The painted tiles form an orthogonally continuous area, like a
       Nurikabe.
    4. There can't be any 2*2 area of the same color(painted or empty).
*/

namespace puzzles::PaintTheNurikabe{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_PAINTED      'P'
#define PUZ_BOUNDARY     '+'
#define PUZ_UNKNOWN       5
    
const Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
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

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, int> m_pos2num;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    // all permutations
    vector<vector<string>> m_num2perms;
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
    , m_sidelen(strs.size() / 2 + 2)
    , m_num2perms(5)
{
    set<Position> rng;
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2 - 2];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (str_h[c * 2 - 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen - 1) break;
        m_start.push_back(PUZ_BOUNDARY);
        auto& str_v = strs[r * 2 - 1];
        for (int c = 1;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2 - 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen - 1) break;
            char ch = str_v[c * 2 - 1];
            m_start.push_back(PUZ_SPACE);
            rng.insert(p);
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        m_areas.emplace_back();
        auto& area = m_areas.back();
        for (auto& p : smoves) {
            area.push_back(p);
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }

    for (int i = 0; i <= 4; ++i) {
        auto perm = string(4 - i, PUZ_EMPTY) + string(i, PUZ_PAINTED);
        do
            m_num2perms[i].push_back(perm);
        while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
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
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_pos2num) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(g.m_num2perms[kv.second].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perm_ids = kv.second;

        map<int, vector<int>> area2dirs;
        string chars;
        for (int i = 0; i < 4; ++i) {
            auto p2 = p + offset[i];
            chars.push_back(cells(p2));
            if (m_game->m_pos2area.contains(p2))
                area2dirs[m_game->m_pos2area.at(p2)].push_back(i);
        }

        auto& perms = m_game->m_num2perms[m_game->m_pos2num.at(p)];
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            return !boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2 ||
                    ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY;
            }) || boost::algorithm::any_of(area2dirs, [&](const pair<const int, vector<int>>& kv) {
                auto& v = kv.second;
                return boost::algorithm::any_of(v, [&](int k) {
                    return perm[k] != perm[v[0]];
                });
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
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
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->count(p2) != 0) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

// 3. The painted tiles form an orthogonally continuous area.
bool puz_state::is_continuous() const
{
    set<Position> a;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SPACE || ch == PUZ_PAINTED)
                a.insert(p);
        }

    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

// There can't be any 2*2 area of the same color(painted or empty).
bool puz_state::check_2x2()
{
    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c) {
            Position p(r, c);
            vector<Position> rngPainted, rngSpace, rngEmpty;
            for (auto& os : offset3) {
                auto p2 = p + os;
                switch(cells(p2)) {
                case PUZ_PAINTED: rngPainted.push_back(p2); break;
                case PUZ_SPACE: rngSpace.push_back(p2); break;
                case PUZ_EMPTY: rngEmpty.push_back(p2); break;
                }
            }
            if (rngPainted.size() == 4 || rngEmpty.size() == 4)
                return false;
            auto f = [&](const Position& p, char ch) {
                for (auto& p2 : m_game->m_areas[m_game->m_pos2area.at(p)])
                    cells(p2) = ch;
            };
            if (rngPainted.size() == 3 && rngSpace.size() == 1)
                f(rngSpace[0], PUZ_EMPTY);
            if (rngEmpty.size() == 3 && rngSpace.size() == 1)
                f(rngSpace[0], PUZ_PAINTED);
        }
    return true;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];

    map<int, vector<int>> area2dirs;
    for (int k = 0; k < perm.size(); ++k) {
        auto p2 = p + offset[k];
        if (m_game->m_pos2area.contains(p2))
            area2dirs[m_game->m_pos2area.at(p2)].push_back(k);
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = perm[k];
    }
    for (auto& kv : area2dirs) {
        int k = kv.second[0];
        for (auto& p2 : m_game->m_areas[kv.first]) {
            char& ch = cells(p2);
            if (ch == PUZ_SPACE)
                ch = perm[k];
        }
    }

    ++m_distance;
    m_matches.erase(p);
    return check_2x2() && is_continuous();
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
    for (int r = 1;; ++r) {
        // draw horz-walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
            auto it = m_game->m_pos2num.find(p);
            if (it == m_game->m_pos2num.end())
                out << ' ';
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PaintTheNurikabe()
{
    using namespace puzzles::PaintTheNurikabe;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PaintTheNurikabe.xml", "Puzzles/PaintTheNurikabe.txt", solution_format::GOAL_STATE_ONLY);
}
