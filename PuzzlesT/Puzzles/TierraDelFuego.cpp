#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 11/Tierra Del Fuego

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

namespace puzzles::TierraDelFuego{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_WATER = '=';
constexpr auto PUZ_BOUNDARY = '+';

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
    map<char, set<Position>> m_ch2rng;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            m_start.push_back(ch);
            if (ch != PUZ_SPACE)
                m_ch2rng[ch].insert(p);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
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
    bool make_move(const Position& p, char ch);
    void make_move2(const Position& p, char ch);
    int find_matches(bool init);
    bool check_tribes() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: position of the cell
    // value.elem: direction of the water if it is '0' ~ '3'
    //             letter of the tribe if it is 'A' ~ 'Z'
    map<Position, set<char>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    find_matches(false);
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& state, const set<Position>& spaces)
        : m_state(&state), m_spaces(&spaces) {
        make_move(*spaces.begin());
    }

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
        char ch = m_state->cells(p);
        if (ch != PUZ_BOUNDARY && ch != PUZ_WATER) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

int puz_state::find_matches(bool init)
{
    set<Position> spaces;
    m_matches.clear();
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SPACE) continue;
            set<char>& chars = m_matches[p];
            set<char> chars2;
            // 4. These bodies of water can only touch diagonally.
            bool canBeWater = boost::algorithm::none_of(offset, [&](auto& os2) {
                return cells(p + os2) == PUZ_WATER;
            });
            for (int i = 0; i < 4; ++i) {
                auto& os = offset[i];
                auto p2 = p + os;
                char ch2 = cells(p2);
                // 3. The archipelago is peculiar because all bodies of water separating the
                // islands are identical in shape and occupied a 2 * 1 or 1 * 2 space.
                if (ch2 == PUZ_SPACE) {
                    // 4. These bodies of water can only touch diagonally.
                    bool canBeWater2 = boost::algorithm::none_of(offset, [&](auto& os2) {
                        return cells(p2 + os2) == PUZ_WATER;
                    });
                    if (canBeWater && canBeWater2)
                        chars.insert(i + '0');
                } else if (ch2 != PUZ_BOUNDARY && ch2 != PUZ_WATER)
                    chars2.insert(ch2);
            }
            if (int sz = chars2.size(); sz == 0)
                // space not adjacent to any letter(tribe)
                spaces.insert(p);
            else if (sz == 1)
                // space adjacent to the same letter(tribe)
                chars.insert(*chars2.begin());
            // space adjacent to different letters(tribes)
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
        // find all letters(tribes) that are reachable to the space
        for (auto& p2 : spaces2)
            m_matches.at(p2).insert(chars2.begin(), chars2.end());
        for (auto& p2 : spaces2)
            spaces.erase(p2);
    }

    if (!check_tribes()) return 0;

    if (!init)
        for (auto& [p, chars] : m_matches) {
            switch (chars.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, *chars.begin()), 1;
            }
        }
    return 2;
}

bool puz_state::make_move(const Position& p, char ch)
{
    m_distance = 0;
    make_move2(p, ch);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::make_move2(const Position& p, char ch)
{
    if (isdigit(ch))
        cells(p) = cells(p + offset[ch - '0']) = PUZ_WATER, m_distance += 2;
    else
        cells(p) = ch, ++m_distance;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& area) : m_area(&area) {
        make_move(*area.begin());
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_area;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->contains(p)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// 2. Being organized in tribes, each tribe, marked with a different letter,
// has occupied an island in the archipelago.
bool puz_state::check_tribes() const
{
    map<char, set<Position>> ch2area;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (isalpha(ch))
                ch2area[ch].insert(p);
        }
    for (auto& [ch, area] : ch2area) {
        for (auto& [p, chars] : m_matches)
            if (chars.contains(ch))
                area.insert(p);
        list<puz_state3> smoves;
        puz_move_generator<puz_state3>::gen_moves(area, smoves);
        if (area.size() != smoves.size())
            return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, chars] = *boost::min_element(m_matches, [](
        const pair<const Position, set<char>>& kv1,
        const pair<const Position, set<char>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : chars) {
        children.push_back(*this);
        if (!children.back().make_move(p, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (isalpha(ch) &&
                !m_game->m_ch2rng.at(ch).contains(p) ? '.' : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TierraDelFuego()
{
    using namespace puzzles::TierraDelFuego;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TierraDelFuego.xml", "Puzzles/TierraDelFuego.txt", solution_format::GOAL_STATE_ONLY);
}
