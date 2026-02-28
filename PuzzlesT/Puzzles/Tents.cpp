#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 1/Tents

    Summary
    Each camper wants to put his Tent under the shade of a Tree. But he also
    wants his privacy!

    Description
    1. The board represents a camping field with many Trees. Campers want to set
       their Tent in the shade, horizontally or vertically adjacent to a Tree(not
       diagonally).
    2. At the same time they need their privacy, so a Tent can't have any other
       Tents near them, not even diagonally.
    3. The numbers on the borders tell you how many Tents there are in that row
       or column.
    4. Finally, each Tree has at least one Tent touching it, horizontally or
       vertically.
*/

namespace puzzles::Tents{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TENT = 'E';
constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 8> offset2 = Position::Directions8;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, vector<Position>> m_tree2tents;
    map<Position, vector<Position>> m_pos2trees;
    // key: position of the hint number
    // value: number of tents that has to be put in the row/col
    map<Position, int> m_tent_counts_rows, m_tent_counts_cols;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
{
    for (int r = 0; r <= m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c <= m_sidelen; ++c) {
            Position p(r, c);
            m_pos2trees[p];
            if (char ch = str[c]; (r == m_sidelen) != (c == m_sidelen)) {
                if (ch != PUZ_SPACE)
                    (c == m_sidelen ? m_tent_counts_rows : m_tent_counts_cols)[p] =
                        ch - '0';
            } else if (m_cells.push_back(ch); ch == PUZ_TREE)
                m_tree2tents[p];
        }
    }
    m_cells.pop_back();

    for (auto& [p, tents] : m_tree2tents)
        for (auto& os : offset)
            if (auto p2 = p + os;
                is_valid(p2) && cells(p2) == PUZ_SPACE) {
                tents.push_back(p2);
                m_pos2trees[p2].push_back(p);
            }
}

// first : all the remaining positions in the area where a tent can be put
// second : the number of tents that need to be put in the area
struct puz_area : pair<set<Position>, int>
{
    puz_area() {}
    puz_area(int tent_count)
        : pair<set<Position>, int>({}, tent_count)
    {}
    puz_area(const vector<Position>& trees)
        : pair<set<Position>, int>({trees.begin(), trees.end()}, 1)
    {}
    void add_cell(const Position& p) { first.insert(p); }
    void remove_cell(const Position& p) { first.erase(p); }
    bool put_tent(const Position& p, bool at_least_one) {
        if (first.erase(p) && (!at_least_one || second == 1))
            return --second, true;
        return false;
    }
    bool is_valid() const {
        // if second < 0, that means too many tents have been put in this area
        // if first.size() < second, that means there are not enough positions
        // for the tents to be put
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : map<Position, puz_area>
{
    puz_group() {}
    puz_group(const map<Position, vector<Position>>& tent2trees) {
        for (auto& [p, trees] : tent2trees)
            (*this)[p] = {trees};
    }
    puz_group(const map<Position, int>& tent_counts) {
        for (auto& [p, cnt] : tent_counts)
            (*this)[p] = {cnt};
    }
    void add_cell(const Position& p, const Position& p2) {
        if (auto it = find(p); it != end())
            it->second.add_cell(p2);
    }
    void remove_cell(const Position& p, const Position& p2) {
        if (auto it = find(p); it != end())
            it->second.remove_cell(p2);
    }
    puz_area* get_cell(const Position& p) {
        auto it = find(p);
        return it == end() ? nullptr : &it->second;
    }
    bool is_valid() const {
        return boost::algorithm::all_of(*this, [](const pair<const Position, puz_area>& kv) {
            return kv.second.is_valid();
        });
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
    bool make_move(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(vector{&m_grp_trees, &m_grp_rows, &m_grp_cols}, 0, [&](int acc, const puz_group* grp) {
            return acc + boost::accumulate(*grp, 0, [&](int acc2, const pair<const Position, puz_area>& kv) {
                return acc2 + kv.second.second;
            });
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    puz_group m_grp_trees;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
    unsigned int m_distance = 0;
};

// 3. The numbers on the borders tell you how many Tents there are in that row
//    or column.
// 4. Finally, each Tree has at least one Tent touching it, horizontally or
//    vertically.
puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g)
    , m_grp_trees(g.m_tree2tents)
    , m_grp_rows(g.m_tent_counts_rows)
    , m_grp_cols(g.m_tent_counts_cols)
{
    for (auto& [_1, tents] : g.m_tree2tents)
        for (auto& p : tents) {
            m_grp_rows.add_cell({p.first, sidelen()}, p);
            m_grp_cols.add_cell({sidelen(), p.second}, p);
        }
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_TENT;
    m_distance = 0;

    auto grps_remove_cell = [&](const Position& p2) {
        for (auto& p3 : m_game->m_pos2trees.at(p2))
            m_grp_trees[p3].remove_cell(p2);
        m_grp_rows.remove_cell({p2.first, sidelen()}, p2);
        m_grp_cols.remove_cell({sidelen(), p2.second}, p2);
    };
    
    for (auto& p2 : m_game->m_pos2trees.at(p))
        if (m_grp_trees[p2].put_tent(p, true))
            ++m_distance;

    for (auto* a : {m_grp_rows.get_cell({p.first, sidelen()}), m_grp_cols.get_cell({sidelen(), p.second})})
        if (a != nullptr && a->put_tent(p, false))
            if (++m_distance; a->second == 0)
                // copy the range
                for (auto rng = a->first; auto& p2 : rng)
                    grps_remove_cell(p2);

    // 2. At the same time they need their privacy, so a Tent can't have any other
    //    Tents near them, not even diagonally.
    for (auto& os : offset2)
        if (auto p2 = p + os; is_valid(p2))
            grps_remove_cell(p2);

    return m_grp_rows.is_valid() && m_grp_cols.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<const puz_area*> areas;
    for (auto grp : {&m_grp_trees, &m_grp_rows, &m_grp_cols})
        for (auto& [_1, a] : *grp)
            if (a.second > 0)
                areas.push_back(&a);

    auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2) {
        return a1->first.size() < a2->first.size();
    });
    for (auto& p : a.first)
        if (!children.emplace_back(*this).make_move(p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c) {
            if (r == sidelen() && c == sidelen())
                break;
            Position p(r, c);
            if (c == sidelen()) {
                int n = 0;
                for (int c = 0; c < sidelen(); ++c)
                    if (cells({r, c}) == PUZ_TENT)
                        ++n;
                out << format("{:<2}", n);
            } else if (r == sidelen()) {
                int n = 0;
                for (int r = 0; r < sidelen(); ++r)
                    if (cells({r, c}) == PUZ_TENT)
                        ++n;
                out << format("{:<2}", n);
            } else {
                char ch = cells(p);
                out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Tents()
{
    using namespace puzzles::Tents;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Tents.xml", "Puzzles/Tents.txt", solution_format::GOAL_STATE_ONLY);
}
