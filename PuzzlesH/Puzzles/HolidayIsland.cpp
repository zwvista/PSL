#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 11/Holiday Island

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

namespace puzzles{ namespace HolidayIsland{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_TENT        'T'
#define PUZ_WATER        'W'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_WATER);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_WATER);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch != PUZ_SPACE) {
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_start.push_back(PUZ_TENT);
            } else
                m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_WATER);
    }
    m_start.append(m_sidelen, PUZ_WATER);
}

struct puz_area
{
    set<Position> m_inner, m_outer;
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p);
    bool make_move2(const Position& p);
    int adjust_area(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_pos2area, 0, [this](
            int acc, const pair<const Position, puz_area>& kv) {
            return acc + m_game->m_pos2num.at(kv.first) + 1 - kv.second.m_inner.size();
        });
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    unsigned int m_distance = 0;
    map<Position, puz_area> m_pos2area;
    Position m_starting;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_pos2num) {
        auto& pnum = kv.first;
        m_pos2area[pnum].m_inner.insert(pnum);
    }
    adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
    for (auto& kv : m_pos2area) {
        const auto& pnum = kv.first;
        auto& area = kv.second;
        auto& outer = area.m_outer;

        outer.clear();
        for (auto& p : area.m_inner)
            for (auto& os : offset) {
                auto p2 = p + os;
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    outer.insert(p2);
            }

        if (!init)
            switch(outer.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(*outer.begin()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s);

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
    make_move(s.m_starting);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (m_state->cells(*this) == PUZ_TENT) return;

    for (auto& os : offset) {
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (ch == PUZ_EMPTY || ch == PUZ_TENT) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

struct puz_state3 : Position
{
    puz_state3(const puz_state& s);

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

puz_state3::puz_state3(const puz_state& s)
: m_state(&s)
{
    make_move(s.m_starting);
}

void puz_state3::gen_children(list<puz_state3>& children) const
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

bool puz_state::make_move2(const Position& p)
{
    cells(m_starting = p) = PUZ_EMPTY;
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    vector<Position> rng_tent, rng_empty;
    for (const auto& p2 : smoves)
        (cells(p2) == PUZ_TENT ? rng_tent : rng_empty).push_back(p2);

    for (const auto& p2 : rng_tent) {
        auto& area = m_pos2area.at(p2);
        auto& inner = area.m_inner;
        int sz = inner.size();
        int num = m_game->m_pos2num.at(p2) + 1;
        inner.insert(rng_empty.begin(), rng_empty.end());
        if (inner.size() > num)
            return false;
        m_distance += inner.size() - sz;
        if (inner.size() == num) {
            for (const auto& p3 : inner)
                for (auto& os : offset) {
                    char& ch = cells(p3 + os);
                    if (ch == PUZ_SPACE)
                        ch = PUZ_WATER;
                }
            m_pos2area.erase(p2);
        }
    }

    // There is only one, continuous island.
    list<puz_state3> smoves2;
    puz_move_generator<puz_state3>::gen_moves(*this, smoves2);
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

bool puz_state::make_move(const Position& p)
{
    m_distance = 0;
    if (!make_move2(p))
        return false;
    int m;
    while ((m = adjust_area(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_pos2area, [&](
        const pair<const Position, puz_area>& kv1,
        const pair<const Position, puz_area>& kv2) {
        //return kv1.second.m_outer.size() < kv2.second.m_outer.size();
        return m_game->m_pos2num.at(kv1.first) < m_game->m_pos2num.at(kv2.first);
    });
    for (auto& p : kv.second.m_outer) {
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_TENT)
                out << format("%-2d") % m_game->m_pos2num.at(p);
            else
                out << (ch == PUZ_SPACE ? PUZ_WATER : ch) << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_HolidayIsland()
{
    using namespace puzzles::HolidayIsland;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HolidayIsland.xml", "Puzzles/HolidayIsland.txt", solution_format::GOAL_STATE_ONLY);
}
