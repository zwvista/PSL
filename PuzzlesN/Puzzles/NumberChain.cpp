#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: NumberChain

    Summary
    Connect the numbers in order

    Description
    1. Connect the numbers in order using Sudoku and Hidato mechanics.
*/

namespace puzzles::NumberChain{

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

const string_view dirs = "^>v<";

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    set<Position> m_area;
    map<Position, int> m_pos2num;
    int m_max_num;
    map<Position, set<Position>> m_pos2rng;

    puz_game(const vector<string>& strs, const xml_node& level);
    int& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_cells(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (auto s = str.substr(c * 2, 2); s == "xx")
                cells(p) = -1;
            else {
                m_area.insert(p);
                if (s != "..")
                    m_pos2num[p] = stoi(string(s));
            }
        }
    }
    m_max_num = m_area.size();

    for (auto& p : m_area)
        for (auto& rng = m_pos2rng[p]; auto& os : offset)
            if (auto p2 = p + os; m_area.contains(p2))
                rng.insert(p2);
}

struct puz_state : vector<int>
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(Position p, int n);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return m_area.size();}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    set<Position> m_area;
    // key: the number
    // value: the possible positions of the number
    map<int, set<Position>> m_num2rng;
};

puz_state::puz_state(const puz_game & g)
: vector<int>(g.m_cells), m_game(&g), m_area(g.m_area)
{
    for (int i = 1; i <= g.m_max_num; ++i)
        m_num2rng[i] = g.m_area;

    for (auto& [p, n] : g.m_pos2num)
        make_move(p, n);
}

bool puz_state::make_move(Position p, int n)
{
    cells(p) = n;
    auto& rng = m_game->m_pos2rng.at(p);
    auto f = [&](int n2) {
        set<Position> rng3;
        auto& rng2 = m_num2rng.at(n2);
        boost::set_intersection(rng, rng2, inserter(rng3, rng3.end()));
        if (rng3.empty())
            return false;
        rng2 = rng3;
        return true;
    };
    if (n < m_game->m_max_num && !f(n + 1))
        return false;
    if (n > 1 && !f(n - 1))
        return false;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [n, rng] = *boost::min_element(m_num2rng, [](
        const pair<const int, set<Position>>& kv1,
        const pair<const int, set<Position>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& p : rng) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << format("{:2}", cells(p));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_NumberChain()
{
    using namespace puzzles::NumberChain;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberChain.xml", "Puzzles/NumberChain.txt", solution_format::GOAL_STATE_ONLY);
}
