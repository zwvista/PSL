#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 1/Parks

    Summary
    Put one Tree in each Park, row and column.(two in bigger levels)

    Description
    1. In Parks, you have many differently coloured areas(Parks) on the board.
    2. The goal is to plant Trees, following these rules:
    3. A Tree can't touch another Tree, not even diagonally.
    4. Each park must have exactly ONE Tree.
    5. There must be exactly ONE Tree in each row and each column.
    6. Remember a Tree CANNOT touch another Tree diagonally,
       but it CAN be on the same diagonal line.
    7. Larger puzzles have TWO Trees in each park, each row and each column.
*/

namespace puzzles::Parks{

constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_SPACE = '.';

constexpr Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},         // e
    {1, 1},         // se
    {1, 0},         // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},       // nw
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    int m_tree_count_area;
    int m_tree_total_count;
    map<Position, int> m_pos2park;
    vector<Position> m_trees;
    vector<Position> m_spaces;
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
        auto p = *this + offset[i * 2];
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
    , m_tree_count_area(level.attribute("TreesInEachArea").as_int(1))
    , m_tree_total_count(m_tree_count_area * m_sidelen)
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
            if (ch == PUZ_TREE)
                m_trees.push_back(p);
            else if (ch == PUZ_SPACE)
                m_spaces.push_back(p);
            if (ch != PUZ_SPACE)
                rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        for (auto& p : smoves) {
            m_pos2park[p] = n;
            rng.erase(p);
        }
    }
    for (auto& p : m_spaces)
        m_pos2park[p] = 0;
}

// first : all the remaining positions in the area where a tree can be planted
// second : the number of trees that need to be planted in the area
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int tree_count_area)
        : pair<vector<Position>, int>({}, tree_count_area)
    {}
    void add_cell(const Position& p) { first.push_back(p); }
    void remove_cell(const Position& p) { boost::remove_erase(first, p); }
    void plant_tree(const Position& p) { remove_cell(p); --second; }
    bool is_valid() const {
        // if second < 0, that means too many trees have been planted in this area
        // if first.size() < second, that means there are not enough positions
        // for the trees to be planted
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int cell_count, int tree_count_area)
        : vector<puz_area>(cell_count, puz_area(tree_count_area)) {}
    bool is_valid() const {
        return boost::algorithm::all_of(*this, [](const puz_area& a) {
            return a.is_valid();
        });
    }
};

struct puz_groups
{
    puz_groups() {}
    puz_groups(const puz_game* g)
        : m_game(g)
        , m_parks(g->m_sidelen, g->m_tree_count_area)
        , m_rows(g->m_sidelen, g->m_tree_count_area)
        , m_cols(g->m_sidelen, g->m_tree_count_area)
    {}

    array<puz_area*, 3> get_areas(const Position& p) {
        return {
            &m_parks[m_game->m_pos2park.at(p)],
            &m_rows[p.first],
            &m_cols[p.second]
        };
    }
    void add_cell(const Position& p) {
        for (auto a : get_areas(p))
            a->add_cell(p);
    }
    void remove_cell(const Position& p) {
        for (auto a : get_areas(p))
            a->remove_cell(p);
    }
    void plant_tree(const Position& p) {
        for (auto a : get_areas(p)) {
            a->plant_tree(p);
            if (a->second == 0) {
                // copy the range
                auto rng = a->first;
                for (auto& p2 : rng)
                    remove_cell(p2);
            }
        }
    }
    bool is_valid() const {
        return m_parks.is_valid() && m_rows.is_valid() && m_cols.is_valid();
    }
    const puz_area& get_best_candidate_area() const;

    const puz_game* m_game = nullptr;
    puz_group m_parks;
    puz_group m_rows;
    puz_group m_cols;
};

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_groups.m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_groups.m_game->m_tree_total_count - boost::count(*this, PUZ_TREE);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    puz_groups m_groups;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_groups(&g)
{
    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c)
            m_groups.add_cell({r, c});

    for (auto& p : g.m_spaces)
        m_groups.remove_cell(p);

    for (auto& p : g.m_trees)
        make_move(p);
}

const puz_area& puz_groups::get_best_candidate_area() const
{
    vector<const puz_area*> areas;
    for (auto grp : {&m_parks, &m_rows, &m_cols})
        for (auto& a : *grp)
            if (a.second > 0)
                areas.push_back(&a);

    return **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2) {
        return a1->first.size() < a2->first.size();
    });
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_TREE;
    m_groups.plant_tree(p);

    // no touch
    for (auto& os : offset) {
        auto p2 = p + os;
        if (is_valid(p2))
            m_groups.remove_cell(p2);
    }

    return m_groups.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& a = m_groups.get_best_candidate_area();
    for (auto& p : a.first) {
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_groups.m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_groups.m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Parks()
{
    using namespace puzzles::Parks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Parks.xml", "Puzzles/Parks.txt", solution_format::GOAL_STATE_ONLY);
}
