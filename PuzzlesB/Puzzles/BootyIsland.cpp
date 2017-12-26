#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 13/Booty Island

    Summary
    Overcrowded Piracy

    Description
    1. Overcrowded by Greedy Pirates (tm), this land has Treasures buried
       almost everywhere and the relative maps scattered around.
    2. In fact there's only one Treasure for each row and for each column.
    3. On the island you can see maps with a number: these tell you how
       many steps are required, horizontally or vertically, to reach a
       Treasure.
    4. For how stupid the Pirates are, they don't bury their Treasures
       touching each other, even diagonally, however at times they are so
       stupid that two or more maps point to the same Treasure!

    Bigger Islands
    5. On bigger islands, there will be two Treasures per row and column.
    6. In this case, the number on the map doesn't necessarily point to the
       closest Treasure on that row or column.
*/

namespace puzzles{ namespace BootyIsland{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_MAP            'M'
#define PUZ_TREASURE    'X'

const Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},    // nw
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    int m_treasure_count_area;
    int m_treasure_total_count;
    map<Position, int> m_pos2map;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_treasure_count_area(level.attribute("TreasuresInEachArea").as_int(1))
    , m_treasure_total_count(m_treasure_count_area * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            if (ch != PUZ_SPACE)
                m_pos2map[{r, c}] = ch - '0';
            m_start.push_back(ch == PUZ_SPACE ? PUZ_EMPTY : PUZ_MAP);
        }
    }
}

// first : all the remaining positions in the area where a treasure can be found
// second : the number of treasures that need to be found in the area
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int treasure_count_area)
        : pair<vector<Position>, int>({}, treasure_count_area)
    {}
    void add_cell(const Position& p) { first.push_back(p); }
    void remove_cell(const Position& p) { boost::remove_erase(first, p); }
    void find_treasure(const Position& p, bool at_least_one) {
        if (boost::algorithm::none_of_equal(first, p)) return;
        remove_cell(p);
        if (!at_least_one || at_least_one && second == 1)
            --second;
    }
    bool is_valid() const { 
        // if second < 0, that means too many treasures have been found in this area
        // if first.size() < second, that means there are not enough positions
        // for the treasures to be found
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int cell_count, int treasure_count_area)
        : vector<puz_area>(cell_count, puz_area(treasure_count_area)) {}
    bool is_valid() const {
        return boost::algorithm::all_of(*this, [](const puz_area& a) {
            return a.is_valid();
        });
    }
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_game->m_treasure_total_count - boost::count(*this, PUZ_TREASURE);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_maps;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
    , m_grp_maps(g.m_pos2map.size(), 1)
    , m_grp_rows(g.m_sidelen, g.m_treasure_count_area)
    , m_grp_cols(g.m_sidelen, g.m_treasure_count_area)
{
    for (int r = 0, i = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) == PUZ_MAP) {
                for (int j = 0; j < 4; ++j) {
                    auto& os = offset[j * 2];
                    int n = g.m_pos2map.at(p);
                    auto p2 = p;
                    if ([&, n]{
                        for (int k = 0; k < n; ++k)
                            if (!is_valid(p2 += os))
                                return false;
                        return cells(p2) == PUZ_EMPTY;
                    }())
                        m_grp_maps[i].add_cell(p2);
                }
                ++i;
            } else {
                m_grp_rows[r].add_cell(p);
                m_grp_cols[c].add_cell(p);
            }
        }
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_TREASURE;

    auto grps_remove_cell = [&](const Position& p2) {
        for (auto& a : m_grp_maps)
            a.remove_cell(p2);
        m_grp_rows[p2.first].remove_cell(p2);
        m_grp_cols[p2.second].remove_cell(p2);
    };

    for (auto& a : m_grp_maps)
        a.find_treasure(p, true);
    for (auto* a : {&m_grp_rows[p.first], &m_grp_cols[p.second]}) {
        a->find_treasure(p, false);
        if (a->second == 0) {
            // copy the range
            auto rng = a->first;
            for (auto& p2 : rng)
                grps_remove_cell(p2);
        }
    }

    // no touch
    for (auto& os : offset) {
        auto p2 = p + os;
        if (is_valid(p2))
            grps_remove_cell(p2);
    }

    return m_grp_rows.is_valid() && m_grp_cols.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    const puz_group* grps[] = {&m_grp_maps, &m_grp_rows, &m_grp_cols};
    vector<const puz_area*> areas;
    for (const puz_group* grp : grps)
        for (const puz_area& a : *grp)
            if (a.second > 0)
                areas.push_back(&a);

    const auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2) {
        return a1->first.size() < a2->first.size();
    });
    for (auto& p : a.first) {
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_MAP)
                out << format("%-2d") % m_game->m_pos2map.at(p);
            else
                out << ch << " ";
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_BootyIsland()
{
    using namespace puzzles::BootyIsland;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BootyIsland.xml", "Puzzles/BootyIsland.txt", solution_format::GOAL_STATE_ONLY);
}
