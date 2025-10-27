#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 17/Desert Dunes

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
constexpr auto PUZ_OASIS = 'S';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::Square2x2Offset;

// 4. Dunes cannot touch each other horizontally or vertically.
// 5. No area of desert of 2x2 should be empty of Dunes.
constexpr string_view perms2x2[] = {
    "...D",
    "..D.",
    ".D..",
    "D...",
    ".DD.",
    "D..D",
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // key: position of the oasis
    // elem: number of oases that the Oasis dweller can reach
    map<Position, int> m_pos2num;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(ch);
            else
                m_cells.push_back(PUZ_OASIS), m_pos2num[p] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
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
    bool check_oases();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: position of the cell
    // value.elem: index of the perms
    map<Position, vector<int>> m_matches;
    // key: position of the oasis
    // elem: number of oases that the Oasis dweller can reach
    map<Position, int> m_pos2num;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c) {
            auto& perm_ids = m_matches[{r, c}];
            perm_ids.resize(size(perms2x2));
            boost::iota(perm_ids, 0);
        }
    find_matches(true);
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& state, const Position& p_start)
        : m_state(&state), m_p_start(&p_start) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const Position* m_p_start;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (*this != *m_p_start && m_state->cells(*this) == PUZ_OASIS) return;
    for (auto& os : offset) {
        auto p = *this + os;
        char ch = m_state->cells(p);
        if (ch != PUZ_BOUNDARY && ch != PUZ_DUNE) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(offset2, perms2x2[id], [&](const Position& os, char ch2) {
                char ch1 = cells(p + os);
                return ch1 == PUZ_SPACE || ch1 == ch2 ||
                    ch1 == PUZ_OASIS && ch2 == PUZ_EMPTY;
            });
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return check_oases() ? 2 : 0;
}

bool puz_state::check_oases()
{
    for (auto& [p, num] : m_game->m_pos2num) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({*this, p});
        int num2 = boost::accumulate(smoves, 0, [&](int acc, const Position& p2) {
            return acc + (cells(p2) == PUZ_OASIS ? 1 : 0);
        }) - 1;
        if (num2 < num || m_matches.empty() && num2 > num)
            return false;
        m_pos2num[p] = num2;
    }
    return true;
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
    auto& perm = perms2x2[n];
    for (int i = 0; i < 4; ++i) {
        char& ch = cells(p + offset2[i]);
        if (ch == PUZ_SPACE)
            ch = perm[i];
    }
    ++m_distance;
    m_matches.erase(p);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, nums] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : nums)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_OASIS)
                out << format("{:<2}", m_pos2num.at(p));
            else
                out << ch << ' ';
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
