#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 2/Hidden Stars

    Summary
    Each Arrow points to a Star and every Star has an arrow pointing at it

    Description
    1. In the board you have to find hidden stars.
    2. Each star is pointed at by at least one Arrow and each Arrow points
       to at least one star.
    3. The number on the borders tell you how many Stars there on that row
       or column.

    Variant
    4. Some levels have a variation of these rules: Stars must be pointed
       by one and only one Arrow.
*/

namespace puzzles::HiddenStars{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_ARROW        'A'
#define PUZ_STAR        'S'

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
    bool m_only_one_arrow;
    map<Position, int> m_pos2arrow;
    vector<int> m_star_counts_rows, m_star_counts_cols;
    int m_star_total_count;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_only_one_arrow(level.attribute("OnlyOneArrow").as_int() != 0)
    , m_star_counts_rows(m_sidelen)
    , m_star_counts_cols(m_sidelen)
{
    for (int r = 0, n = 0; r < m_sidelen + 1; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen + 1; c++) {
            char ch = str[c];
            if (ch == PUZ_SPACE)
                m_start.push_back(PUZ_EMPTY);
            else {
                n = ch - '0';
                if (c == m_sidelen)
                    m_star_counts_rows[r] = n;
                else if (r == m_sidelen)
                    m_star_counts_cols[c] = n;
                else {
                    m_pos2arrow[{r, c}] = n;
                    m_start.push_back(PUZ_ARROW);
                }
            }
        }
    }
    m_star_total_count = boost::accumulate(m_star_counts_rows, 0);
}

// first : all the remaining positions in the area where a star can be hidden
// second : the number of stars that need to be found in the area
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int star_count)
        : pair<vector<Position>, int>({}, star_count)
    {}
    void add_cell(const Position& p) { first.push_back(p); }
    void remove_cell(const Position& p) { boost::remove_erase(first, p); }
    void place_star(const Position& p, bool at_least_one) {
        if (boost::algorithm::none_of_equal(first, p)) return;
        remove_cell(p);
        if (!at_least_one || at_least_one && second == 1)
            --second;
    }
    bool is_valid() const {
        // if second < 0, that means too many stars have been found in this area
        // if first.size() < second, that means there are not enough positions
        // for the stars to be found
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(const vector<int>& star_counts) {
        for (int cnt : star_counts)
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
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { 
        return m_game->m_star_total_count - boost::count(*this, PUZ_STAR);
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
, m_grp_arrows(vector<int>(g.m_pos2arrow.size(), 1))
, m_grp_rows(g.m_star_counts_rows)
, m_grp_cols(g.m_star_counts_cols)
{
    int i = 0;
    for (auto& kv : g.m_pos2arrow) {
        auto& p = kv.first;
        auto& os = offset[kv.second];
        for (auto p2 = p + os; is_valid(p2); p2 += os)
            if (cells(p2) == PUZ_EMPTY) {
                m_grp_rows[p2.first].add_cell(p2);
                m_grp_cols[p2.second].add_cell(p2);
                m_grp_arrows[i].add_cell(p2);
            }
        ++i;
    }
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_STAR;

    auto f = [&](puz_area& a, bool at_least_one) {
        a.place_star(p, at_least_one);
        if (at_least_one || a.second > 0) return;
        // copy the range
        auto rng = a.first;
        for (auto& p2 : rng) {
            for (auto& a2 : m_grp_arrows)
                a2.remove_cell(p2);
            m_grp_rows[p2.first].remove_cell(p2);
            m_grp_cols[p2.second].remove_cell(p2);
        }
    };

    for (auto& a : m_grp_arrows)
        f(a, !m_game->m_only_one_arrow);
    f(m_grp_rows[p.first], false);
    f(m_grp_cols[p.second], false);

    return m_grp_rows.is_valid() && m_grp_cols.is_valid() &&
        (!m_game->m_only_one_arrow || m_grp_arrows.is_valid());
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<const puz_area*> areas;
    for (auto grp : {&m_grp_arrows, &m_grp_rows, &m_grp_cols})
        for (auto& a : *grp)
            if (a.second > 0)
                areas.push_back(&a);

    auto& a = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2) {
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
    auto f = [&](int n) { out << format("%-2d") % n; };

    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                ;
            else if (c == sidelen())
                f(m_game->m_star_counts_rows.at(r));
            else if (r == sidelen())
                f(m_game->m_star_counts_cols.at(c));
            else {
                Position p(r, c);
                char ch = cells(p);
                if (ch == PUZ_ARROW)
                    f(m_game->m_pos2arrow.at(p));
                else
                    out << ch << " ";
            }
        out << endl;
    }
    return out;
}

}

void solve_puz_HiddenStars()
{
    using namespace puzzles::HiddenStars;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HiddenStars.xml", "Puzzles/HiddenStars.txt", solution_format::GOAL_STATE_ONLY);
}
