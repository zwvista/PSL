#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 11/Orchards

    Summary
    Plant the trees. Very close, this time!

    Description
    1. In a reverse of 'Parks', you're now planting Trees close together in
       neighboring country areas.
    2. These are Apple Trees, which must cross-pollinate, thus must be planted
       in pairs - horizontally or vertically touching.
    3. A Tree must be touching just one other Tree: you can't put three or
       more contiguous Trees.
    4. At the same time, like in Parks, every country area must have exactly
       two Trees in it.
*/

namespace puzzles::Orchards{

constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_SPACE = '.';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_tree_info
{
    // Trees must be planted in pairs
    Position m_trees[2];
    // there are no 3 or more contiguous trees,
    // so the 6 tiles around the paired trees must be empty
    Position m_spaces[6];
};

const puz_tree_info tree_info[] = {
    {{{0, 0}, {-1, 0}}, {{-2, 0}, {-1, 1}, {0, 1}, {1, 0}, {0, -1}, {-1, -1}}},
    {{{0, 0}, {0, 1}}, {{-1, 0}, {-1, 1}, {0, 2}, {1, 1}, {1, 0}, {0, -1}}},
    {{{0, 0}, {1, 0}}, {{-1, 0}, {0, 1}, {1, 1}, {2, 0}, {1, -1}, {0, -1}}},
    {{{0, 0}, {0, -1}}, {{-1, 0}, {0, 1}, {1, 0}, {1, -1}, {0, -2}, {-1, -1}}},
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_area_count;
    map<Position, int> m_pos2area;
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
, m_sidelen(strs.size() / 2)
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
            rng.insert(p);
        }
    }

    for (m_area_count = 0; !rng.empty(); ++m_area_count) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        for (auto& p : smoves) {
            m_pos2area[p] = m_area_count;
            rng.erase(p);
        }
    }
}

// first : all the remaining positions in the area where a tree can be planted
// second : the number of trees that need to be planted in the area
struct puz_area : pair<set<Position>, int>
{
    puz_area() : pair<set<Position>, int>({}, 2) {}
    void add_cell(const Position& p) { first.insert(p); }
    void remove_cell(const Position& p) { first.erase(p); }
    bool can_plant_tree(const Position& p) { return first.contains(p); }
    void plant_tree(const Position& p) { first.erase(p); --second; }
    bool is_valid() const {
        // if second < 0, that means too many trees have been planted in this area
        // if first.size() < second, that means there are not enough positions
        // for the trees to be planted
        return second >= 0 && first.size() >= second;
    }
};

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    puz_area& get_area(const Position& p) { return m_areas[m_game->m_pos2area.at(p)]; }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return 2 * m_game->m_area_count - boost::count(m_cells, PUZ_TREE);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    vector<puz_area> m_areas;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g), m_areas(g.m_area_count)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            get_area(p).add_cell(p);
        }
}

bool puz_state::make_move(const Position& p, int n)
{
    auto& info = tree_info[n];
    for (auto& os : info.m_trees) {
        auto p2 = p + os;
        // trees must be inside
        if (!is_valid(p2))
            return false;
        cells(p2) = PUZ_TREE;
        auto& a = get_area(p2);
        // the position must be valid in the area
        if (!a.can_plant_tree(p2))
            return false;
        a.plant_tree(p2);
        // the area must remain valid after the tree is planted
        if (!a.is_valid())
            return false;
    }
    for (auto& os : info.m_spaces) {
        auto p2 = p + os;
        // positions around the trees can be outside
        if (!is_valid(p2))
            continue;
        auto& a = get_area(p2);
        a.remove_cell(p2);
        // the area must remain valid after the position is marked as empty
        if (!a.is_valid())
            return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& a = *boost::min_element(m_areas, [](const puz_area& a1, const puz_area& a2) {
        auto f = [](const puz_area& a) {
            // ignore the areas where no trees need to be planted
            return a.second == 0 ? 100 : a.first.size();
        };
        return f(a1) < f(a2);
    });
    for (auto& p : a.first)
        for (int i = 0; i < 4; ++i)
            if (children.push_back(*this); !children.back().make_move(p, i))
                children.pop_back();
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

void solve_puz_Orchards()
{
    using namespace puzzles::Orchards;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Orchards.xml", "Puzzles/Orchards.txt", solution_format::GOAL_STATE_ONLY);
}
