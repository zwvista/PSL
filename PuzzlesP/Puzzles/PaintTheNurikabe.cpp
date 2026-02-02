#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 16/Paint The Nurikabe

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

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_PAINTED = 'P';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_UNKNOWN = 5;
    
constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

constexpr array<Position, 4> offset3 = Position::Square2x2Offset;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
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
        make_move_hint(p_start);
    }

    void make_move_hint(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall))
            children.emplace_back(*this).make_move_hint(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 + 2)
    , m_num2perms(5)
{
    set<Position> rng;
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2 - 2];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (str_h[c * 2 - 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen - 1) break;
        m_cells.push_back(PUZ_BOUNDARY);
        string_view str_v = strs[r * 2 - 1];
        for (int c = 1;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2 - 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen - 1) break;
            char ch = str_v[c * 2 - 1];
            m_cells.push_back(PUZ_SPACE);
            rng.insert(p);
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
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
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move_hint(const Position& p, int n);
    bool make_move_hint2(const Position& p, int n);
    bool make_move_area(int n, char ch);
    int find_matches(bool init);
    bool is_interconnected() const;
    bool check_2x2();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_painted_areas.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    set<int> m_painted_areas;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    for (auto& [p, n] : g.m_pos2num) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_num2perms[n].size());
        boost::iota(perm_ids, 0);
    }
    for (int i = 0; i < g.m_areas.size(); ++i)
        m_painted_areas.insert(i);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
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
                return make_move_hint2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state& s, const Position& p) : m_state(&s) { make_move_hint(p); }

    void make_move_hint(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        switch (auto p2 = *this + os; m_state->cells(p2)) {
        case PUZ_SPACE:
        case PUZ_PAINTED:
            children.emplace_back(*this).make_move_hint(p2);
        }
}

// 3. The painted tiles form an orthogonally continuous area.
bool puz_state::is_interconnected() const
{
    int i = m_cells.find(PUZ_PAINTED);
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {*this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return cells(p) == PUZ_PAINTED;
    }) == boost::count(m_cells, PUZ_PAINTED);
}

// There can't be any 2*2 area of the same color(painted or empty).
bool puz_state::check_2x2()
{
    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c) {
            Position p(r, c);
            vector<Position> rngPainted, rngSpace, rngEmpty;
            for (auto& os : offset3)
                switch (auto p2 = p + os; cells(p2)) {
                case PUZ_PAINTED: rngPainted.push_back(p2); break;
                case PUZ_SPACE: rngSpace.push_back(p2); break;
                case PUZ_EMPTY: rngEmpty.push_back(p2); break;
                }
            if (rngPainted.size() == 4 || rngEmpty.size() == 4)
                return false;
            auto f = [&](const Position& p, char ch) {
                for (auto& p2 : m_game->m_areas[m_game->m_pos2area.at(p)])
                    if (cells(p2) = ch;
                        m_painted_areas.erase(m_game->m_pos2area.at(p2)))
                        ++m_distance;
            };
            if (rngPainted.size() == 3 && rngSpace.size() == 1)
                f(rngSpace[0], PUZ_EMPTY);
            if (rngEmpty.size() == 3 && rngSpace.size() == 1)
                f(rngSpace[0], PUZ_PAINTED);
        }
    return true;
}

bool puz_state::make_move_hint2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];

    map<int, vector<int>> area2dirs;
    for (int k = 0; k < perm.size(); ++k) {
        auto p2 = p + offset[k];
        if (m_game->m_pos2area.contains(p2))
            area2dirs[m_game->m_pos2area.at(p2)].push_back(k);
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = perm[k];
    }
    for (auto& [area_id, dirs] : area2dirs) {
        for (int k = dirs[0]; auto& p2 : m_game->m_areas[area_id])
            if (char& ch = cells(p2); ch == PUZ_SPACE)
                ch = perm[k];
        if (m_painted_areas.erase(area_id))
            ++m_distance;
    }

    ++m_distance, m_matches.erase(p);
    return check_2x2() && is_interconnected();
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move_hint2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_area(int n, char ch)
{
    for (auto& p2 : m_game->m_areas[n])
        cells(p2) = ch;
    m_distance = 1, m_painted_areas.erase(n);
    return check_2x2() && is_interconnected();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : perm_ids)
            if (!children.emplace_back(*this).make_move_hint(p, n))
                children.pop_back();
    } else {
        int n = *m_painted_areas.begin();
        for (char ch : {PUZ_PAINTED, PUZ_EMPTY})
            if (!children.emplace_back(*this).make_move_area(n, ch))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
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
