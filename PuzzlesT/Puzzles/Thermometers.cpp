#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 14/Thermometers

    Summary
    Puzzle Fever

    Description
    1. On the board a few Thermometers are laid down. Your goal is  to fill
       them according to the hints.
    2. In a Thermometer, mercury always starts at the bulb and can progressively
       fill the Thermometer towards the end.
    3. A Thermometer can also be completely empty, including the bulb.
    4. The numbers on the border tell you how many filled cells are present
       on that Row or Column. 
*/

namespace puzzles{ namespace Thermometers{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_SECTOR        '+'
#define PUZ_END            'o'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

const string bulbs = "v<^>";

struct puz_game
{
    string m_id;
    int m_sidelen;
    // elem: thermometer
    // elem.elem: position occupied by the thermometer
    vector<vector<Position>> m_thermometer_info;
    // key: position occupied by the thermometer
    // value: index of the thermometer
    map<Position, int> m_pos2thermometer;
    vector<int> m_filled_counts_rows, m_filled_counts_cols;
    int m_filled_total_count;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * (m_sidelen + 1) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_filled_counts_rows(m_sidelen)
    , m_filled_counts_cols(m_sidelen)
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r <= m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c <= m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            if (isdigit(ch))
                (c == m_sidelen ? m_filled_counts_rows[r] : m_filled_counts_cols[c])
                = ch - '0';
            else {
                int d = bulbs.find(ch);
                if (d == -1) continue;
                int n = m_thermometer_info.size();
                m_thermometer_info.emplace_back();
                auto& info = m_thermometer_info.back();
                auto& os = offset[d];
                for (auto p2 = p;; p2 += os) {
                    m_pos2thermometer[p2] = n;
                    info.push_back(p2);
                    if (cells(p2) == PUZ_END) break;
                }
            }
        }
    }
    m_filled_total_count = boost::accumulate(m_filled_counts_rows, 0);
}

// first : all the remaining positions in the area where mercury can be filled
// second : the number of cells that need to be filled in the row or column
//        or the number of cells that have been filled in the thermometer
struct puz_area : pair<vector<Position>, int>
{
    puz_area() {}
    puz_area(int filled_count)
        : pair<vector<Position>, int>({}, filled_count)
    {}
    void add_cell(const Position& p) { first.push_back(p); }
    void remove_cell(const Position& p) { boost::remove_erase(first, p);    }
    bool fill_mercury(const Position& p, bool is_grp_thermometers) {
        if (boost::algorithm::none_of_equal(first, p))
            return false;
        remove_cell(p);
        is_grp_thermometers ? ++second : --second;
        return true;
    }
    bool is_valid() const {
        // if second < 0, that means too much mercury have been filled in this area
        // if first.size() < second, that means there are not enough positions
        // for the mercury to be filled
        return second >= 0 && first.size() >= second;
    }
};

// all of the areas in the group
struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(const vector<int>& filled_counts) {
        for (int cnt : filled_counts)
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
    void check_rowcol(puz_area& area);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_game->m_filled_total_count - boost::count_if(*this, [](char ch) {
            return ch != PUZ_EMPTY;
        });
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_thermometers;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_EMPTY), m_game(&g)
    , m_grp_thermometers(vector<int>(g.m_thermometer_info.size(), 0))
    , m_grp_rows(g.m_filled_counts_rows)
    , m_grp_cols(g.m_filled_counts_cols)
{
    for (int i = 0; i < g.m_thermometer_info.size(); ++i)
        for (auto& p : g.m_thermometer_info[i])
            for (auto a : {&m_grp_thermometers[i], &m_grp_rows[p.first], &m_grp_cols[p.second]})
                a->add_cell(p);

    for (int i = 0; i < sidelen(); ++i)
        for (auto a : {&m_grp_rows[i], &m_grp_cols[i]})
            check_rowcol(*a);
}

void puz_state::check_rowcol(puz_area& area)
{
    if (area.second != 0) return;
    // copy the range
    auto rng = area.first;
    for (auto& p : rng) {
        auto& rng2 = m_grp_thermometers[m_game->m_pos2thermometer.at(p)].first;
        vector<Position> rng3(boost::range::find(rng2, p), rng2.end());
        for (auto p2 : rng3)
            for (auto a : {&m_grp_thermometers[m_game->m_pos2thermometer.at(p2)],
                &m_grp_rows[p2.first], &m_grp_cols[p2.second]})
                a->remove_cell(p2);
    }
}

bool puz_state::make_move(const Position& p)
{
    int n = m_game->m_pos2thermometer.at(p);
    auto& thermometer = m_grp_thermometers[n];
    auto& info = m_game->m_thermometer_info[n];

    int n2 = boost::range::find(info, p) - info.begin();
    for (int i = 0; i <= n2; ++i) {
        auto& p2 = info[i];
        cells(p2) =
            i == 0 ? m_game->cells(p2) :
            i == n2 ? PUZ_END : PUZ_SECTOR;
    }

    auto& rng = thermometer.first;
    vector<Position> rng2(rng.begin(), ++boost::range::find(rng, p));
    for (auto p2 : rng2) {
        if (!thermometer.fill_mercury(p2, true))
            return false;
        for (auto a : {&m_grp_rows[p2.first], &m_grp_cols[p2.second]}) {
            a->fill_mercury(p2, false);
            check_rowcol(*a);
        }
    }

    return m_grp_rows.is_valid() && m_grp_cols.is_valid();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<const puz_area*> areas;
    for (auto grp : {&m_grp_rows, &m_grp_cols})
        for (auto& a : *grp)
            if (a.second > 0)
                areas.push_back(&a);

    auto& area = **boost::min_element(areas, [](const puz_area* a1, const puz_area* a2) {
        return a1->second < a2->second;
    });
    for (auto& p : area.first) {
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c) {
            if (r == sidelen() && c == sidelen())
                break;
            if (c == sidelen())
                out << format("%-2d") % m_game->m_filled_counts_rows[r];
            else if (r == sidelen())
                out << format("%-2d") % m_game->m_filled_counts_cols[c];
            else
                out << cells({r, c}) << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Thermometers()
{
    using namespace puzzles::Thermometers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Thermometers.xml", "Puzzles/Thermometers.txt", solution_format::GOAL_STATE_ONLY);
}
