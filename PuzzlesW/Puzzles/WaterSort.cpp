#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Water pouring puzzle

    Summary
    Water pouring puzzle
      
    Description
    1. The game involves a finite collection of water jugs of known integer capacities
       (in terms of a liquid measure such as liters or gallons).
    2. Initially each jug contains a known integer volume of liquid, not necessarily equal to its capacity.
    3. Puzzles of this type ask how many steps of pouring water from one jug to another
       (until either one jug becomes empty or the other becomes full) are needed to reach a goal state,
       specified in terms of the volume of liquid that must be present in some jug or jugs.
*/

namespace puzzles::WaterSort{

constexpr auto PUZ_UNKNOWN = -1;

struct puz_game
{
    string m_id;
    vector<int> m_quantities, m_capacities, m_goals;
    bool m_has_tap_sink;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_move : pair<int, int> { using pair::pair; };

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_has_tap_sink(level.attribute("tap_sink").as_int() != 0)
{
    using qi::lit;
    using qi::int_;
    using ascii::space;
    {
        const string start = level.attribute("start").value();
        string::const_iterator first = start.begin();
        qi::phrase_parse(first, start.end(),
            int_[phx::push_back(phx::ref(m_quantities), qi::_1)] >> '/' >>
            int_[phx::push_back(phx::ref(m_capacities), qi::_1)] >>
            *(
                lit(',') >>
                int_[phx::push_back(phx::ref(m_quantities), qi::_1)] >> '/' >>
                int_[phx::push_back(phx::ref(m_capacities), qi::_1)]
             ), space);
    }
    {
        const string goal = level.attribute("goal").value();
        string::const_iterator first = goal.begin();
        qi::phrase_parse(first, goal.end(),
            (int_[phx::push_back(phx::ref(m_goals), qi::_1)] |
                lit('*')[phx::push_back(phx::ref(m_goals), PUZ_UNKNOWN)]) >>
            *(
                lit(',') >>
                (int_[phx::push_back(phx::ref(m_goals), qi::_1)] |
                lit('*')[phx::push_back(phx::ref(m_goals), PUZ_UNKNOWN)])
             ), space);
    }
    if (m_has_tap_sink) {
        m_quantities.insert(m_quantities.begin(), PUZ_UNKNOWN);
        m_capacities.insert(m_capacities.begin(), PUZ_UNKNOWN);
        m_goals.insert(m_goals.begin(), PUZ_UNKNOWN);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const { 
        return pair{m_quantities, m_move} < pair{x.m_quantities, x.m_move};
    }
    void make_move(int i, int j);

    //solve_puzzle interface
    bool is_goal_state() const {
        return boost::equal(m_quantities, m_game->m_goals, [](int a, int b) {
            return b == PUZ_UNKNOWN || a == b;
        });
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return is_goal_state() ? 0 : 1; }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_quantities;
    boost::optional<puz_move> m_move;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_quantities(g.m_quantities)
{
}

void puz_state::make_move(int i, int j)
{
    int &q1 = m_quantities[i], &q2 = m_quantities[j];
    int c1 = m_game->m_capacities[i], c2 = m_game->m_capacities[j];
    if (q1 == PUZ_UNKNOWN)
        // fill the jug with water from the tap
        q2 = c2;
    else if (q2 == PUZ_UNKNOWN)
        // empty the jug by pouring water into the sink
        q1 = 0;
    else {
        // pour water from one jug to another
        int n = c2 - q2;
        if (q1 < n)
            // not enough water to fill the second jug
            q2 += q1, q1 = 0;
        else
            // fill the second jug and leave some water in the first one
            q1 -= n, q2 = c2;
    }
    m_move = puz_move(i, j);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < m_quantities.size(); i++)
        for (int j = 0; j < m_quantities.size(); j++) {
            if (i == j) continue;
            children.push_back(*this);
            children.back().make_move(i, j);
        }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move) {
        int i = m_move->first, j = m_move->second;
        string a, b;
        if (m_game->m_has_tap_sink) {
            a = i == 0 ? "tap" : boost::lexical_cast<string>(i);
            b = j == 0 ? "sink" : boost::lexical_cast<string>(j);
        }
        else {
            a = boost::lexical_cast<string>(i + 1);
            b = boost::lexical_cast<string>(j + 1);
        }
        out << format("move: {} -> {}\n", a, b);
    }
    for (int i = 0; i < m_quantities.size(); i++) {
        if (i == 0 && m_game->m_has_tap_sink) continue;
        out << m_quantities[i] << "/" << m_game->m_capacities[i];
        if (i < m_quantities.size() - 1)
            out << ",";
    }
    println(out);
    return out;
}

}

void solve_puz_WaterSort()
{
    using namespace puzzles::WaterSort;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WaterSort.xml", "Puzzles/WaterSort.txt");
}
