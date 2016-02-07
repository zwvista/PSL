#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 12/Botanical Park

    Summary
    Excuse me sir ? Do you know where the Harpagophytum Procumbens is ?

    Description
    1. The board represents a Botanical Park, with arrows pointing to the
       different plants.
    2. Each arrow points to at least one plant and there is exactly one
       plant in every row and in every column.
    3. Plants cannot touch, not even diagonally.

    Variant
    4. Puzzle with side 9 or bigger have TWO plants in every row and column.
*/

namespace puzzles{ namespace BotanicalPark{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_ARROW        'A'
#define PUZ_PLANT        'P'

const Position offset[] = {
    {-1, 0},    // n
    {-1, 1},    // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},    // sw
    {0, -1},    // w
    {-1, -1},    // nw
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    int m_plant_count_area;
    int m_plant_total_count;
    map<Position, int> m_pos2arrow;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size())
    , m_plant_count_area(attrs.get<int>("PlantsInEachArea", 1))
    , m_plant_total_count(m_plant_count_area * m_sidelen)
{
    for(int r = 0; r < m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c < m_sidelen; c++){
            char ch = str[c];
            if(ch != PUZ_SPACE)
                m_pos2arrow[{r, c}] = ch - '0';
            m_start.push_back(ch == PUZ_SPACE ? PUZ_EMPTY : PUZ_ARROW);
        }
    }
}

// first : all the remaining positions in the area where a plant can be hidden
// second : the number of plants that need to be found in the area
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int plant_count)
        : pair<vector<Position>, int>({}, plant_count)
    {}
    void add_cell(const Position& p){ first.push_back(p); }
    void remove_cell(const Position& p){ boost::remove_erase(first, p); }
    void place_plant(const Position& p, bool at_least_one){
        if(boost::algorithm::none_of_equal(first, p)) return;
        remove_cell(p);
        if(!at_least_one || at_least_one && second == 1)
            --second;
    }
    bool is_valid() const {
        // if second < 0, that means too many plants have been found in this area
        // if first.size() < second, that means there are not enough positions
        // for the plants to be found
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int cell_count, int plant_count_area)
        : vector<puz_area>(cell_count, puz_area(plant_count_area)) {}
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
        return m_game->m_plant_total_count - boost::count(*this, PUZ_PLANT);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_arrows;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
    , m_grp_arrows(g.m_pos2arrow.size(), 1)
    , m_grp_rows(g.m_sidelen, g.m_plant_count_area)
    , m_grp_cols(g.m_sidelen, g.m_plant_count_area)
{
    for(int r = 0, i = 0; r < sidelen(); ++r)
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            if(cells(p) == PUZ_ARROW){
                auto& os = offset[g.m_pos2arrow.at(p)];
                for(auto p2 = p + os; is_valid(p2); p2 += os)
                    if(cells(p2) == PUZ_EMPTY)
                        m_grp_arrows[i].add_cell(p2);
                ++i;
            }
            else{
                m_grp_rows[r].add_cell(p);
                m_grp_cols[c].add_cell(p);
            }
        }
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_PLANT;

    auto grps_remove_cell = [&](const Position& p2){
        for(auto& a : m_grp_arrows)
            a.remove_cell(p2);
        m_grp_rows[p2.first].remove_cell(p2);
        m_grp_cols[p2.second].remove_cell(p2);
    };

    for(auto& a : m_grp_arrows)
        a.place_plant(p, true);
    for(auto* a : {&m_grp_rows[p.first], &m_grp_cols[p.second]}){
        a->place_plant(p, false);
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
    for(auto grp : {&m_grp_arrows, &m_grp_rows, &m_grp_cols})
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
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_ARROW)
                out << format("%-2d") % m_game->m_pos2arrow.at(p);
            else
                out << ch << " ";
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_BotanicalPark()
{
    using namespace puzzles::BotanicalPark;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\BotanicalPark.xml", "Puzzles\\BotanicalPark.txt", solution_format::GOAL_STATE_ONLY);
}
