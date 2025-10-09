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
        : m_game(game), m_num(num), m_p(p) {}

    bool is_goal_state() const { return size() == m_num; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    int m_num;
    Position m_p;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    auto f = [&] (const Position& p) {
        for (auto& os : offset) {
            // 5. A camper can walk from his Tent,
            // by moving horizontally or vertically.A camper can't cross water or 
            // other Tents.
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if (ch2 == PUZ_SPACE && !contains(p2)) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
    };
    if (empty())
        f(m_p);
    else
        for (auto& p : *this)
            f(p);
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_WATER);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_WATER);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_cells.push_back(PUZ_TENT);
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
    bool make_move2(const Position& p, int n);
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
    // key: the position of the hint
    // value: index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [p, perms] : g.m_pos2perms) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    // key: a space tile
    // value.elem: position of the number from which a path
    //             containing that space tile can be formed
    map<Position, set<Position>> space2hints;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_SPACE)
                space2hints[p];
        }

    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_pos2perms.at(p);
        boost::remove_erase_if(perm_ids, [&](int id) {
            return boost::algorithm::any_of(perms[id], [&](const Position& p2) {
                return cells(p2) == PUZ_WATER;
            });
        });
        for (int id : perm_ids)
            for (auto& p2 : perms[id])
                space2hints.at(p2).insert(p);

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    bool changed = false;
    for (auto& [p, h] : space2hints) {
        if (!h.empty()) continue;
        // Cells that cannot be reached by any camper can be nothing but a water
        cells(p) = PUZ_WATER;
        changed = true;
    }
    if (changed && !init)
        for (auto& [p, perm_ids] : m_matches)
            if (perm_ids.size() == 1)
                return make_move2(p, perm_ids.front()) ? 1 : 0;
    return is_interconnected() ? 2 : 0;
}

struct puz_state4 : Position
{
    puz_state4(const puz_state& s);

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state4>& children) const;

    const puz_state* m_state;
};

puz_state4::puz_state4(const puz_state& s)
: m_state(&s)
{
    //make_move(s.m_starting);
}

void puz_state4::gen_children(list<puz_state4>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (ch != PUZ_WATER) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

// 4. There is only one, continuous island.
bool puz_state::is_interconnected() const
{
    //set<Position> a;
    //for (int r = 1; r < sidelen() - 1; ++r)
    //    for (int c = 1; c < sidelen() - 1; ++c) {
    //        Position p(r, c);
    //        char ch = cells(p);
    //        if (ch == PUZ_SPACE || ch == PUZ_WALL)
    //            a.insert(p);
    //    }

    //auto smoves = puz_move_generator<puz_state3>::gen_moves(a);
    //return smoves.size() == a.size();
    return true;
}

bool puz_state::make_move2(const Position& p, int n)
{
    //cells(m_starting = p) = PUZ_EMPTY;
    //auto smoves = puz_move_generator<puz_state3>::gen_moves(*this);
    //vector<Position> rng_tent, rng_empty;
    //for (const auto& p2 : smoves)
    //    (cells(p2) == PUZ_TENT ? rng_tent : rng_empty).push_back(p2);

    //for (const auto& p2 : rng_tent) {
    //    auto& area = m_pos2area.at(p2);
    //    auto& inner = area.m_inner;
    //    int sz = inner.size();
    //    int num = m_game->m_pos2num.at(p2) + 1;
    //    inner.insert(rng_empty.begin(), rng_empty.end());
    //    if (inner.size() > num)
    //        return false;
    //    m_distance += inner.size() - sz;
    //    if (inner.size() == num) {
    //        for (const auto& p3 : inner)
    //            for (auto& os : offset) {
    //                char& ch = cells(p3 + os);
    //                if (ch == PUZ_SPACE)
    //                    ch = PUZ_WATER;
    //            }
    //        m_pos2area.erase(p2);
    //    }
    //}

    // There is only one, continuous island.
    auto smoves2 = puz_move_generator<puz_state4>::gen_moves(*this);
    set<Position> rng1(smoves2.begin(), smoves2.end());

    set<Position> rng2, rng3;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p2(r, c);
            if (cells(p2) != PUZ_WATER)
                rng2.insert(p2);
        }
    boost::set_difference(rng2, rng1, inserter(rng3, rng3.end()));
    return boost::algorithm::all_of(rng3, [this](const Position& p2) {
        return cells(p2) == PUZ_SPACE;
    });
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    //auto& [pnum, area] = *boost::min_element(m_pos2area, [&](
    //    const pair<const Position, puz_area>& kv1,
    //    const pair<const Position, puz_area>& kv2) {
    //    //return kv1.second.m_outer.size() < kv2.second.m_outer.size();
    //    return m_game->m_pos2num.at(kv1.first) < m_game->m_pos2num.at(kv2.first);
    //});
    //for (auto& p : area.m_outer)
    //    if (children.push_back(*this); !children.back().make_move(p))
    //        children.pop_back();
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
        "Puzzles/HolidayIsland.xml", "Puzzles/HolidayIsland2.txt", solution_format::GOAL_STATE_ONLY);
}
