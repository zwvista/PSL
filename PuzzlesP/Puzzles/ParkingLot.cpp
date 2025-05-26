#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 11/Parking Lot

    Summary
    BEEEEP BEEEEEEP !!!

    Description
    1. The board represents a parking lot seen from above.
    2. Each number identifies a car and all cars are identified by a number,
       there are no hidden cars.
    3. Cars can be regular sports cars (2*1 tiles) or limousines (3*1 tiles)
       and can be oriented horizontally or vertically.
    4. The number in itself specifies how far the car can move forward or
       backward, in tiles.
    5. For example, a car that has one tile free in front and one tile free
       in the back, would be marked with a '2'.
    6. Find all the cars !!
*/

namespace puzzles::ParkingLot{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_CAR = 'x';

struct puz_car_kind
{
    string m_str;
    vector<vector<Position>> m_offset_car;
    vector<Position> m_offset_move;
} car_kinds[] = {
    {   // horizontal sports car
        "h-",
        {{{0, -1}, {0, 0}}, {{0, 0}, {0, 1}}},
        {{0, -1}, {0, 1}}
    },
    {   // horizontal limousine
        "H--",
        {{{0, -2}, {0, -1}, {0, 0}}, {{0, -1}, {0, 0}, {0, 1}}, {{0, 0}, {0, 1}, {0, 2}}},
        {{0, -1}, {0, 1}}
    },
    {   // vertical sports car
        "v|",
        {{{-1, 0}, {0, 0}}, {{0, 0}, {1, 0}}},
        {{-1, 0}, {1, 0}}
    },
    {   // vertical limousine
        "V||",
        {{{-2, 0}, {-1, 0}, {0, 0}}, {{-1, 0}, {0, 0}, {1, 0}}, {{0, 0}, {1, 0}, {2, 0}}},
        {{-1, 0}, {1, 0}}
    }
};

struct puz_hint_perm
{
    string m_str;
    vector<Position> m_car;
    vector<Position> m_empty;
    vector<Position> m_other;
};

struct puz_hint_info
{
    // how far the car can move forward or backward
    int m_move_count;
    // all permutations
    vector<puz_hint_perm> m_hint_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_hint_info> m_pos2hintinfo;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
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
            char ch = str[c];
            if (ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                m_start.push_back(PUZ_CAR);
                m_pos2hintinfo[{r, c}].m_move_count = ch - '0';
            }
        }
    }

    puz_hint_perm hp;
    for (auto& kv : m_pos2hintinfo) {
        const auto& pn = kv.first;
        auto& info = kv.second;
        int n = info.m_move_count;

        for (auto& ck : car_kinds) {
            hp.m_str = ck.m_str;
            for (auto& oss : ck.m_offset_car) {
                hp.m_car.clear();
                for (auto& os : oss) {
                    auto p2 = pn + os;
                    if (!is_valid(p2) || cells(p2) == PUZ_CAR && p2 != pn)
                        goto next_car1;
                    hp.m_car.push_back(p2);
                }
                for (int i = 0; i <= n; ++i) {
                    hp.m_empty.clear();
                    hp.m_other.clear();
                    for (int j = 0; j < 2; ++j) {
                        auto os = ck.m_offset_move[j];
                        auto p2 = (j == 0 ? hp.m_car.front() : hp.m_car.back()) + os;
                        for (int m = 1; m <= (j == 0 ? i : n - i); ++m) {
                            if (!is_valid(p2) || cells(p2) != PUZ_SPACE)
                                goto next_car2;
                            hp.m_empty.push_back(p2);
                            p2 += os;
                        }
                        if (is_valid(p2))
                            hp.m_other.push_back(p2);
                    }
                    info.m_hint_perms.push_back(hp);
                next_car2:;
                }
            next_car1:;
            }
        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_pos2hintinfo) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(kv.second.m_hint_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto& hint_perms = m_game->m_pos2hintinfo.at(p).m_hint_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& hp = hint_perms[id];
            return boost::algorithm::any_of(hp.m_car, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_CAR;
            }) || boost::algorithm::any_of(hp.m_empty, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_EMPTY;
            }) || boost::algorithm::any_of(hp.m_other, [&](const Position& p2) {
                char ch = cells(p2);
                return ch == PUZ_EMPTY;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& hp = m_game->m_pos2hintinfo.at(p).m_hint_perms[n];
    for (int i = 0; i < hp.m_car.size(); ++i)
        cells(hp.m_car[i]) = hp.m_str[i];
    for (auto& p2 : hp.m_empty)
        cells(p2) = PUZ_EMPTY;
    for (auto& p2 : hp.m_other) {
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = PUZ_CAR;
    }

    ++m_distance;
    m_matches.erase(p);

    // pruning
    set<Position> rng1, rng2;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p2(r, c);
            if (cells(p2) == PUZ_CAR)
                rng1.insert(p2);
        }
    for (auto& kv : m_matches) {
        auto& hint_perms = m_game->m_pos2hintinfo.at(kv.first).m_hint_perms;
        for (int id : kv.second)
            for (auto& p2 : hint_perms[id].m_car)
                rng2.insert(p2);
    }
    return boost::includes(rng2, rng1);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1, 
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto it = m_game->m_pos2hintinfo.find(p);
            if (it == m_game->m_pos2hintinfo.end())
                out << " .";
            else
                out << format("{:2}", it->second.m_move_count);
        }
        println(out);
    }
    println(out);
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << format("{:<2}", ch == PUZ_SPACE ? PUZ_EMPTY : ch);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ParkingLot()
{
    using namespace puzzles::ParkingLot;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ParkingLot.xml", "Puzzles/ParkingLot.txt", solution_format::GOAL_STATE_ONLY);
}
