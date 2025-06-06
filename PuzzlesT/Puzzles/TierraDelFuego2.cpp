#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 11/Tierra Del Fuego

    Summary
    Fuegians!

    Description
    1. The board represents the 'Tierra del Fuego' archipelago, where native
       tribes, the Fuegians, live.
    2. Being organized in tribes, each tribe, marked with a different letter,
       has occupied an island in the archipelago.
    3. The archipelago is peculiar because all bodies of water separating the
       islands are identical in shape and occupied a 2*1 or 1*2 space.
    4. These bodies of water can only touch diagonally.
    5. Your task is to find these bodies of water.
    6. Please note there are no hidden tribes or islands without a tribe on it.
*/

namespace puzzles::TierraDelFuego2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_WATER = '=';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    bool check_land() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, set<char>> m_matches;
    int m_distance = 0;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state& state, const set<Position>& spaces)
        : m_state(&state), m_spaces(&spaces) { make_move(*spaces.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const set<Position>* m_spaces;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (!m_spaces->contains(*this)) return;
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_state->is_valid(p)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    set<Position> spaces;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SPACE) continue;
            set<char>& chars = m_matches[p];
            set<char> chars2;
            for (int i = 0; i < 4; ++i) {
                auto& os = offset[i];
                auto p2 = p + os;
                if (!is_valid(p2)) continue;
                char ch2 = cells(p2);
                // 3. The archipelago is peculiar because all bodies of water separating the
                // islands are identical in shape and occupied a 2 * 1 or 1 * 2 space.
                if (ch2 == PUZ_SPACE)
                    chars.insert(i + '0');
                else
                    chars2.insert(ch2);
            }
            if (int sz = chars2.size(); sz == 0)
                spaces.insert(p);
            else if (sz == 1)
                chars.insert(*chars2.begin());
        }

    while (!spaces.empty()) {       
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({ *this, spaces }, smoves);
        set<Position> spaces2;
        set<char> chars2;
        for (auto& p2 : smoves) {
            if (spaces.contains(p2))
                spaces2.insert(p2);
            else
                for (auto ch2 : m_matches.at(p2))
                    if (isalpha(ch2))
                        chars2.insert(ch2);
        }
        for (auto& p2 : spaces2)
            m_matches.at(p2).insert(chars2.begin(), chars2.end());
        for (auto& p2 : spaces2)
            spaces.erase(p2);
    }
}

bool puz_state::make_move(int n)
{
    //for (auto& p : m_game->m_water_info[n]) {
    //    cells(p) = PUZ_WATER;
    //    for (auto& os : offset) {
    //        auto p2 = p + os;
    //        if (!is_valid(p2)) continue;
    //        char& ch = cells(p2);
    //        if (ch == PUZ_SPACE)
    //            ch = PUZ_ISLAND;
    //    }
    //}

    //// 4. These bodies of water can only touch diagonally.
    //boost::remove_erase_if(m_water_ids, [&](int id) {
    //    auto& info = m_game->m_water_info[id];
    //    return boost::algorithm::any_of(info, [&](const Position& p) {
    //        return cells(p) != PUZ_SPACE;
    //    });
    //});

    //set<Position> a;
    //for (int i = 0; i < length(); ++i) {
    //    char ch = (*this)[i];
    //    if (ch != PUZ_WATER && ch != PUZ_SPACE)
    //        a.insert({i / sidelen(), i % sidelen()});
    //}
    //while (!a.empty()) {
    //    list<puz_state2> smoves;
    //    puz_move_generator<puz_state2>::gen_moves(a, smoves);
    //    set<char> tribes;
    //    for (auto& p : smoves) {
    //        a.erase(p);
    //        char ch = cells(p);
    //        if (isalpha(ch))
    //            tribes.insert(ch);
    //    }
    //    if (tribes.size() > 1)
    //        return false;
    //}

    //for (int i = 0; i < length(); ++i) {
    //    char ch = (*this)[i];
    //    if (ch != PUZ_WATER)
    //        a.insert({i / sidelen(), i % sidelen()});
    //}
    //int tribes_index = 0;
    //set<char> all_tribes;
    //while (!a.empty()) {
    //    list<puz_state2> smoves;
    //    puz_move_generator<puz_state2>::gen_moves(a, smoves);
    //    set<char> tribes;
    //    for (auto& p : smoves) {
    //        a.erase(p);
    //        char ch = cells(p);
    //        if (isalpha(ch)) {
    //            if (all_tribes.contains(ch))
    //                return false;
    //            tribes.insert(ch);
    //        }
    //    }
    //    all_tribes.insert(tribes.begin(), tribes.end());
    //    if (tribes.empty())
    //        return false;
    //    int n = tribes.size();
    //    tribes_index += n * n;
    //}

    //return is_goal_state() || !m_water_ids.empty();
    return true;
}

bool puz_state::check_land() const {
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    //for (int n : m_water_ids) {
    //    children.push_back(*this);
    //    if (!children.back().make_move(n))
    //        children.pop_back();
    //}
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << ch << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TierraDelFuego2()
{
    using namespace puzzles::TierraDelFuego2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TierraDelFuego.xml", "Puzzles/TierraDelFuego.txt", solution_format::GOAL_STATE_ONLY);
}
