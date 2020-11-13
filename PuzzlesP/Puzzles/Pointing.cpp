#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 3/Puzzle Set 2/Pointing

    Summary
    Are you pointing to me?

    Description
    1. Mark some arrows so that each arrow points to exactly one marked arrow.
*/

namespace puzzles::Pointing{

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
    vector<int> m_start;
    map<Position, set<Position>> m_pos2rng;

    puz_game(const vector<string>& strs, const xml_node& level);
    int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            int n = str[c] - '0';
            m_start.push_back(n);
            Position p(r, c);
            auto& rng = m_pos2rng[p];
            auto& os = offset[n];
            for (auto p2 = p + os; is_valid(p2); p2 += os)
                rng.insert(p2);
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& pTo);
    void make_move2(const Position& pTo);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<Position, set<Position>> m_matches;
    set<Position> m_marked;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_matches(g.m_pos2rng)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perms = kv.second;

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(*perms.begin()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& pTo)
{
    set<Position> rngFrom, rngTo;
    for (auto& [pFrom, rng] : m_matches)
        if (rng.count(pTo) == 1) {
            rngFrom.insert(pFrom);
            rngTo.insert(rng.begin(), rng.end());
        }
    m_marked.insert(pTo);
    rngTo.erase(pTo);
    for (auto& pFrom : rngFrom) {
        m_distance++;
        m_matches.erase(pFrom);
    }
    for (auto& [pFrom, rng] : m_matches)
        for (auto& pTo : rngTo)
            rng.erase(pTo);
}

bool puz_state::make_move(const Position& pTo)
{
    m_distance = 0;
    make_move2(pTo);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, set<Position>>& kv1,
        const pair<const Position, set<Position>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& pTo : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(pTo))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << m_game->cells(p) << (m_marked.count(p) == 0 ? ' ' : '*');
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Pointing()
{
    using namespace puzzles::Pointing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Pointing.xml", "Puzzles/Pointing.txt", solution_format::GOAL_STATE_ONLY);
}
