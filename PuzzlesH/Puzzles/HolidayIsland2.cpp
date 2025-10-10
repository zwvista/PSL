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

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<Position, vector<set<Position>>> m_pos2perms;
    map<Position, set<Position>> m_space2hints;
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
                m_space2hints[p];
            } else {
                m_cells.push_back(PUZ_TENT);
                m_pos2num[p] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        }
        m_cells.push_back(PUZ_WATER);
    }
    m_cells.append(m_sidelen, PUZ_WATER);

    for (auto& [p, num] : m_pos2num) {
        auto& perms = m_pos2perms[p];
        puz_state2 sstart(this, num, p);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            for (auto& spath : spaths) {
                auto& s = spath.back();
                perms.push_back(s);
                for (auto& p2 : s)
                    if (p2 != p)
                        m_space2hints[p2].insert(p);
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
    void check_spaces();
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
    map<Position, set<Position>> m_space2hints;
    // key: the position of the hint
    // value.elem: index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_space2hints(g.m_space2hints)
{
    for (auto& [p, perms] : g.m_pos2perms) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(perms.size());
        boost::iota(perm_ids, 0);
    }

    check_spaces();
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_pos2perms.at(p);
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            return !boost::algorithm::all_of(perm, [&](const Position& p2) {
                char ch2 = cells(p2);
                return (p == p2 || ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY) &&
                    boost::algorithm::all_of(offset, [&](const Position& os2) {
                        auto p3 = p2 + os2;
                        return perm.contains(p3) || cells(p3) != PUZ_EMPTY;
                    });
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

void puz_state::check_spaces()
{
    for(auto it = m_space2hints.begin(); it != m_space2hints.end();)
        if (auto& [p, h] = *it; !h.empty())
            ++it;
        else {
            // Cells that cannot be reached by any camper can be nothing but a water
            cells(p) = PUZ_WATER;
            it = m_space2hints.erase(it);
        }
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

inline bool is_island(char ch) { return ch != PUZ_WATER; }

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os;
            is_island(m_state->cells(p2))) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

// 4. There is only one, continuous island.
bool puz_state::is_interconnected() const
{
    int i = boost::find_if(m_cells, is_island) - m_cells.begin();
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return smoves.size() == boost::count_if(m_cells, is_island);
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_pos2perms.at(p)[n];

    for (auto& p2 : perm)
        if (char& ch2 = cells(p2); ch2 == PUZ_SPACE) {
            ch2 = PUZ_EMPTY;
            m_space2hints.erase(p2);
        }
    for (auto& p2 : perm)
        for (auto& os : offset) {
            auto p3 = p2 + os;
            if (char& ch2 = cells(p3); ch2 == PUZ_SPACE) {
                ch2 = PUZ_WATER;
                m_space2hints.erase(p3);
            }
        }

    for (auto& [_1, h] : m_space2hints)
        h.erase(p);
    check_spaces();

    ++m_distance;
    m_matches.erase(p);
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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](auto& kv1, auto& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
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
