#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 1/Tents

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

namespace puzzles{ namespace Tents{

#define PUZ_SPACE        ' '
#define PUZ_TREE        'T'
#define PUZ_EMPTY        '.'
#define PUZ_TENT        'E'

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
    vector<vector<Position>> m_tree_info;
    map<Position, vector<int>> m_pos2trees;
    vector<int> m_tent_counts_rows, m_tent_counts_cols;
    int m_tent_total_count;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_tent_counts_rows(m_sidelen)
    , m_tent_counts_cols(m_sidelen)
{
    vector<Position> trees;
    for(int r = 0; r <= m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c <= m_sidelen; ++c){
            m_pos2trees[{r, c}];
            switch(char ch = str[c]){
            case PUZ_SPACE:
                m_start.push_back(PUZ_EMPTY);
                break;
            case PUZ_TREE:
                m_start.push_back(PUZ_TREE);
                trees.emplace_back(r, c);
                break;
            default:
                (c == m_sidelen ? m_tent_counts_rows[r] : m_tent_counts_cols[c]) = ch - '0';
                break;
            }
        }
    }
    m_start.pop_back();
    m_tent_total_count = boost::accumulate(m_tent_counts_rows, 0);

    m_tree_info.resize(trees.size());
    for(int i = 0; i < trees.size(); ++i){
        auto& p = trees[i];
        auto& info = m_tree_info[i];
        for(int j = 0; j < 8; j += 2){
            auto p2 = p + offset[j];
            if(is_valid(p2) && cells(p2) == PUZ_EMPTY){
                info.push_back(p2);
                m_pos2trees[p2].push_back(i);
            }
        }
    }
}

// first : all the remaining positions in the area where a tent can be put
// second : the number of tents that need to be put in the area
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int tent_count)
        : pair<vector<Position>, int>({}, tent_count)
    {}
    void add_cell(const Position& p){ first.push_back(p); }
    void remove_cell(const Position& p){ boost::remove_erase(first, p); }
    void put_tent(const Position& p, bool at_least_one){
        if(boost::algorithm::none_of_equal(first, p)) return;
        remove_cell(p);
        if(!at_least_one || at_least_one && second == 1)
            --second;
    }
    bool is_valid() const {
        // if second < 0, that means too many tents have been put in this area
        // if first.size() < second, that means there are not enough positions
        // for the tents to be put
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(const vector<int>& tent_counts){
        for(int cnt : tent_counts)
            emplace_back(cnt);
    }
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
        return m_game->m_tent_total_count - boost::count(*this, PUZ_TENT);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_trees;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
    , m_grp_trees(vector<int>(g.m_tree_info.size(), 1))
    , m_grp_rows(g.m_tent_counts_rows)
    , m_grp_cols(g.m_tent_counts_cols)
{
    for(int i = 0; i < g.m_tree_info.size(); ++i)
        for(auto& p : g.m_tree_info[i]){
            m_grp_rows[p.first].add_cell(p);
            m_grp_cols[p.second].add_cell(p);
            m_grp_trees[i].add_cell(p);
        }
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_TENT;

    auto grps_remove_cell = [&](const Position& p2){
        for(int n : m_game->m_pos2trees.at(p2))
            m_grp_trees[n].remove_cell(p2);
        m_grp_rows[p2.first].remove_cell(p2);
        m_grp_cols[p2.second].remove_cell(p2);
    };

    for(int n : m_game->m_pos2trees.at(p))
        m_grp_trees[n].put_tent(p, true);
    for(auto* a : {&m_grp_rows[p.first], &m_grp_cols[p.second]}){
        a->put_tent(p, false);
        if(a->second == 0){
            // copy the range
            auto rng = a->first;
            for(auto& p2 : rng)
                grps_remove_cell(p2);
        }
    }

    // no touch
    for(auto& os : offset){
        auto p2 = p + os;
        if(is_valid(p2))
            grps_remove_cell(p2);
    }

    return m_grp_rows.is_valid() && m_grp_cols.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<const puz_area*> areas;
    for(auto grp : {&m_grp_trees, &m_grp_rows, &m_grp_cols})
        for(auto& a : *grp)
            if(a.second > 0)
                areas.push_back(&a);

    auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2){
        return a1->first.size() < a2->first.size();
    });
    for(auto& p : a.first){
        children.push_back(*this);
        if(!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0; r <= sidelen(); ++r){
        for(int c = 0; c <= sidelen(); ++c){
            if(r == sidelen() && c == sidelen())
                break;
            if(c == sidelen())
                out << format("%-2d") % m_game->m_tent_counts_rows[r];
            else if(r == sidelen())
                out << format("%-2d") % m_game->m_tent_counts_cols[c];
            else
                out << cells({r, c}) << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Tents()
{
    using namespace puzzles::Tents;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Tents.xml", "Puzzles/Tents.txt", solution_format::GOAL_STATE_ONLY);
}
