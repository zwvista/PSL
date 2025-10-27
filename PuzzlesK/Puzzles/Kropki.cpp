#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 6/Kropki

    Summary
    Fill the rows and columns with numbers, respecting the relations

    Description
    1. The Goal is to enter numbers 1 to board size once in every row and
       column.
    2. A Dot between two tiles give you hints about the two numbers:
    3. Black Dot - one number is twice the other.
    4. White Dot - the numbers are consecutive.
    5. Where the numbers are 1 and 2, there can be either a Black Dot(2 is
       1*2) or a White Dot(1 and 2 are consecutive).
    6. Please note that when two numbers are either consecutive or doubles,
       there MUST be a Dot between them!

    Variant
    7. In later 9*9 levels you will also have bordered and coloured areas,
       which must also contain all the numbers 1 to 9.
*/

namespace puzzles::Kropki{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BLACK = 'B';
constexpr auto PUZ_WHITE = 'W';
constexpr auto PUZ_NOT_BH = '.';
    
constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_game;
struct puz_area
{
    vector<Position> m_rng;
    string m_dots;
    vector<pair<int, int>> m_pos_id_pairs;
    vector<int> m_perm_ids;
    void add_pos(const Position& p) { m_rng.push_back(p); }
    void prepare(const puz_game* g);
    bool add_perm(const vector<int>& perm, const puz_game* g);
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_bordered;
    string m_cells;
    vector<puz_area> m_areas;
    vector<string> m_perms;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
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

void puz_area::prepare(const puz_game* g) {
    boost::sort(m_rng);
    for (int i = 0; i < m_rng.size() - 1; ++i)
        for (int j = i + 1; j < m_rng.size(); ++j) {
            const auto& p = m_rng[i];
            auto os = m_rng[j] - p;
            if (os == offset[1] || os == offset[2]) {
                m_pos_id_pairs.emplace_back(i, j);
                m_dots.push_back(g->cells({p.first * 2 + (os == offset[1] ? 0 : 1), p.second}));
            }
        }
}

bool puz_area::add_perm(const vector<int>& perm, const puz_game* g)
{
    string dots;
    int i01 = -1;
    for (int i = 0; i < m_pos_id_pairs.size(); ++i) {
        const auto& kv = m_pos_id_pairs[i];
        int n1 = perm[kv.first], n2 = perm[kv.second];
        if (n1 > n2) ::swap(n1, n2);
        dots.push_back(n2 - n1 == 1 ? PUZ_WHITE :
            n2 == n1 * 2 ? PUZ_BLACK : PUZ_NOT_BH);
        if (n1 == 1 && n2 == 2)
            i01 = i;
    }
    bool b = m_dots == dots || i01 != -1 &&
        m_dots == (dots[i01] = PUZ_BLACK, dots);
    if (b)
        m_perm_ids.push_back(g->m_perms.size());
    return b;
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_bordered(level.attribute("Bordered").as_int() == 1)
    , m_sidelen(strs[0].size())
    , m_areas(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen * 2 - 1; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_cells.push_back(ch == PUZ_SPACE ? PUZ_NOT_BH : ch);
        }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_areas[r].add_pos(p);
            m_areas[m_sidelen + c].add_pos(p);
        }

    if (m_bordered) {
        set<Position> rng;
        for (int r = 0;; ++r) {
            // horizontal walls
            string_view str_h = strs[(r + m_sidelen) * 2 - 1];
            for (int c = 0; c < m_sidelen; ++c)
                if (str_h[c * 2 + 1] == '-')
                    m_horz_walls.emplace(r, c);
            if (r == m_sidelen) break;
            string_view str_v = strs[(r + m_sidelen) * 2];
            for (int c = 0;; ++c) {
                // vertical walls
                if (str_v[c * 2] == '|')
                    m_vert_walls.emplace(r, c);
                if (c == m_sidelen) break;
                rng.emplace(r, c);
            }
        }
        while (!rng.empty()) {
            auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
            m_areas.emplace_back();
            for (const auto& p : smoves) {
                m_areas.back().add_pos(p);
                rng.erase(p);
            }
        }
    }

    for (auto& a : m_areas)
        a.prepare(this);

    vector<int> perm(m_sidelen);
    string perm2(m_sidelen, PUZ_SPACE);
    boost::iota(perm, 1);
    do
        if ([&]{
            bool b = false;
            for (auto& a : m_areas)
                if (a.add_perm(perm, this))
                    b = true;
            return b;
        }()) {
            for (int i = 0; i < m_sidelen; ++i)
                perm2[i] = perm[i] + '0';
            m_perms.push_back(perm2);
        }
    while (boost::next_permutation(perm));
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
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

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    for (int i = 0; i < g.m_areas.size(); ++i)
        m_matches[i] = g.m_areas[i].m_perm_ids;
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& area = m_game->m_areas[area_id];
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(area, perms[id], [&](const Position& p, char ch2) {
                char ch1 = cells(p);
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
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_areas[i].m_rng;
    auto& perm = m_game->m_perms[j];

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
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(area_id, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen() * 2 - 1; ++r) {
        for (int c = 0; c < sidelen() * 2 - 1; ++c)
            out << (r % 2 == 0 ?
            c % 2 == 0 ? cells({r / 2, c / 2}) :
            m_game->cells({r, c / 2}) :
            c % 2 == 1 ? PUZ_SPACE :
            m_game->cells({r, c / 2}));
        println(out);
    }

    if (m_game->m_bordered)
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

void solve_puz_Kropki()
{
    using namespace puzzles::Kropki;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Kropki.xml", "Puzzles/Kropki.txt", solution_format::GOAL_STATE_ONLY);
}
