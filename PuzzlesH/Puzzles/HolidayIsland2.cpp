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

namespace puzzles::HolidayIsland2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TENT = 'T';
constexpr auto PUZ_WATER = 'W';
constexpr auto PUZ_WATER_MOVE_ID = -1;

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_move
{
    Position m_p_hint;
    set<Position> m_area;
    set<Position> m_waters;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, int num, const Position& p)
        : m_game(game), m_num(num) {make_move(p);}

    bool is_goal_state() const { return size() == m_num + 1; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // 5. A camper can walk from his Tent,
            // by moving horizontally or vertically.A camper can't cross water or 
            // other Tents.
            auto p2 = p + os;
            if (char ch2 = m_game->cells(p2); 
                ch2 == PUZ_SPACE && !contains(p2)) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
}

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
                m_pos2move_ids[p].push_back(PUZ_WATER_MOVE_ID);
            } else {
                m_cells.push_back(PUZ_TENT);
                m_pos2num[p] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        }
        m_cells.push_back(PUZ_WATER);
    }
    m_cells.append(m_sidelen, PUZ_WATER);

    for (auto& [p, num] : m_pos2num) {
        puz_state2 sstart(this, num, p);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            for (auto& spath : spaths) {
                auto& s = spath.back();
                int n = m_moves.size();
                auto& [p_hint, area, waters] = m_moves.emplace_back();
                p_hint = p, area = s;
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os; !s.contains(p3))
                            if (char ch3 = cells(p3); ch3 == PUZ_SPACE)
                                waters.insert(p3);
                for (auto& p2 : s)
                    m_pos2move_ids[p2].push_back(n);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        boost::remove_erase_if(perm_ids, [&](int id) {
            if (id == PUZ_WATER_MOVE_ID)
                return cells(p) != PUZ_SPACE;

            auto& [p_hint, area, waters] = m_game->m_moves[id];
            return !boost::algorithm::all_of(area, [&](const Position& p2) {
                char ch2 = cells(p2);
                return p_hint == p2 || ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            }) || !boost::algorithm::all_of(waters, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_WATER;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
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
            children.push_back(*this);
            children.back().make_move(p2);
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

void puz_state::make_move2(const Position& p, int n)
{
    if (n == PUZ_WATER_MOVE_ID)
        cells(p) = PUZ_WATER, ++m_distance, m_matches.erase(p);
    else {
        auto& [_1, area, waters] = m_game->m_moves[n];
        for (auto& p2 : area) {
            char& ch2 = cells(p2);
            if (ch2 != PUZ_EMPTY)
                ++m_distance, m_matches.erase(p2);
            if (ch2 == PUZ_SPACE)
                ch2 = PUZ_EMPTY;
        }
        for (auto& p2 : waters)
            if (char& ch2 = cells(p2); ch2 == PUZ_SPACE)
                ch2 = PUZ_WATER, ++m_distance, m_matches.erase(p2);
    }
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
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

void solve_puz_HolidayIsland2()
{
    using namespace puzzles::HolidayIsland2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HolidayIsland.xml", "Puzzles/HolidayIsland.txt", solution_format::GOAL_STATE_ONLY);
}
