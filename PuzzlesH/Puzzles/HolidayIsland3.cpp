#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 11/Holiday Island

    Summary
    This time the campers won't have their way!

    Description
    1. This time the resort is an island, the place is packed and the campers
       (Tents) must compromise!
    2. The board represents an Island, where there are a few Tents, identified
       by the numbers.
    3. Your job is to find the water surrounding the island, with these rules:
    4. There is only one, continuous island.
    5. The numbers tell you how many tiles that camper can walk from his Tent,
       by moving horizontally or vertically. A camper can't cross water or 
       other Tents.
*/

namespace puzzles::HolidayIsland3{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TENT = 'T';
constexpr auto PUZ_WATER = 'W';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<int, vector<Position>> m_num2rng;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_WATER);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_WATER);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE) {
                m_cells.push_back(PUZ_SPACE);
            } else {
                m_cells.push_back(PUZ_TENT);
                int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_pos2num[p] = n;
                m_num2rng[n].push_back(p);
            }
        }
        m_cells.push_back(PUZ_WATER);
    }
    m_cells.append(m_sidelen, PUZ_WATER);
}

using puz_move = pair<set<Position>, set<Position>>; // area, waters

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches, m_num2rng) < tie(x.m_cells, x.m_matches, x.m_num2rng);
    }
    bool make_move(const Position& p, const puz_move& move);
    void make_move2(const Position& p, const puz_move& move);
    void find_moves();
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { 
        return boost::accumulate(m_num2rng, 0, [](int acc, const pair<const int, vector<Position>>& kv) {
            return acc + kv.second.size();
        }) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<puz_move>> m_matches;
    unsigned int m_distance = 0;
    map<int, vector<Position>> m_num2rng;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_num2rng(g.m_num2rng)
{
    find_moves();
    find_matches(true);
}

struct puz_state2 : set<Position>
{
    puz_state2(const puz_state* state, int num, const Position& p)
        : m_state(state), m_num(num) {make_move(p);}

    bool is_goal_state() const { return size() == m_num + 1; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_state* m_state;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // 5. A camper can walk from his Tent,
            // by moving horizontally or vertically.A camper can't cross water or 
            // other Tents.
            auto p2 = p + os;
            if (char ch2 = m_state->cells(p2); 
                ch2 == PUZ_SPACE && !contains(p2))
                children.emplace_back(*this).make_move(p2);
        }
}

void puz_state::find_moves()
{
    if (m_num2rng.empty())
        return;
    auto& [num, rng] = *m_num2rng.begin();
    for (auto& p : rng) {
        puz_state2 sstart(this, num, p);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            for (auto& spath : spaths) {
                auto& s = spath.back();
                auto& [area, waters] = m_matches[p].emplace_back();
                area = s;
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os; !s.contains(p3))
                            if (char ch3 = cells(p3); ch3 == PUZ_SPACE)
                                waters.insert(p3);
            }
    }
    m_num2rng.erase(num);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, moves] : m_matches) {
        boost::remove_erase_if(moves, [&](const puz_move& move) {
            auto& [area, waters] = move;
            return !boost::algorithm::all_of(area, [&](const Position& p2) {
                char ch2 = cells(p2);
                return p == p2 || ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            }) || !boost::algorithm::all_of(waters, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_WATER;
            });
        });

        if (!init)
            switch(moves.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, moves.front()), 1;
            }
    }
    return is_interconnected() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        switch (auto p2 = *this + os; m_state->cells(p2)) {
        case PUZ_SPACE:
        case PUZ_EMPTY:
        case PUZ_TENT:
            children.emplace_back(*this).make_move(p2);
        }
}

// 4. There is only one, continuous island.
bool puz_state::is_interconnected() const
{
    auto is_island = [](char ch) {
        return ch == PUZ_TENT || ch == PUZ_EMPTY;
    };
    int i = boost::find_if(m_cells, is_island) - m_cells.begin();
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return is_island(cells(p));
    }) == boost::count_if(m_cells, is_island);
}

void puz_state::make_move2(const Position& p, const puz_move& move)
{
    auto& [area, waters] = move;
    for (auto& p2 : area)
        if (char& ch2 = cells(p2); ch2 == PUZ_SPACE)
            ch2 = PUZ_EMPTY;
    for (auto& p2 : waters)
        if (char& ch2 = cells(p2); ch2 == PUZ_SPACE)
            ch2 = PUZ_WATER;
    ++m_distance, m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, const puz_move& move)
{
    m_distance = 0;
    make_move2(p, move);
    int m;
    while ((m = find_matches(false)) == 1);
    if (m == 2 && m_matches.empty())
        find_moves();
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_matches.empty())
        return;
    auto& [p, moves] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<puz_move>>& kv1,
        const pair<const Position, vector<puz_move>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& move : moves)
        if (!children.emplace_back(*this).make_move(p, move))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_TENT)
                out << format("{:<2}", m_game->m_pos2num.at(p));
            else
                out << (ch == PUZ_SPACE ? PUZ_WATER : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_HolidayIsland3()
{
    using namespace puzzles::HolidayIsland3;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HolidayIsland.xml", "Puzzles/HolidayIsland.txt", solution_format::GOAL_STATE_ONLY);
}
