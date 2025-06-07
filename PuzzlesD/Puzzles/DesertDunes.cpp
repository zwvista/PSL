#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 17/Desert Dunes

    Summary
    Hide and seek in the desert

    Description
    1. Put some dunes on the desert so that each Oasis dweller can reach the
       number of Oases marked on it.
    2. The desert among dunes (including oases) should be all connected
       horizontally or vertically.
    3. Dwellers can move horizontally or vertically.
    4. Dunes cannot touch each other horizontally or vertically.
    5. No area of desert of 2x2 should be empty of Dunes.
*/

namespace puzzles::DesertDunes{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_DUNE = 'D';
constexpr auto PUZ_BOUNDARY = '+';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};
constexpr Position offset2[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
};

// 4. Dunes cannot touch each other horizontally or vertically.
// 5. No area of desert of 2x2 should be empty of Dunes.
constexpr string_view perms2x2[] = {
    "...D",
    "..D.",
    ".D..",
    "D...",
    ".D.D",
    "D.D.",
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, int> m_pos2num;

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
                m_pos2num[p] = ch - '0';
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

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
    // value.elem: index of the perms
    map<Position, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto& [p, n] : m_game->m_pos2num) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(size(perms2x2));
        boost::iota(perm_ids, 0);
    }
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
            children.push_back(*this);
            children.back().make_move(p);
    }
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& os : offset2)
            chars.push_back(cells(p + os));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms2x2[id], [](char ch1, char ch2) {
                return ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY ||
                    ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::make_move2(const Position& p, int n)
{
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, nums] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : nums) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_DesertDunes()
{
    using namespace puzzles::DesertDunes;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DesertDunes.xml", "Puzzles/DesertDunes.txt", solution_format::GOAL_STATE_ONLY);
}
