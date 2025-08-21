#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 17/Nooks

    Summary
    ...in a forest

    Description
    1. Fill some tiles with hedges, so that each number (where someone is playing hide and seek)
       finds itself in the nook.
    2. a Nook is a dead end, one tile wide, with a number in it.
    3. a Nook contains a number that shows you how many tiles can be seen in a straight line from
       there, including the tile itself.
    4. The resulting maze should be a single one-tile path connected horizontally or vertically
       where there are no 2x2 areas of the same type (hedge or path).
*/

namespace puzzles::Nooks{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_NOOK = 'N';
constexpr auto PUZ_HEDGE = '=';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

constexpr Position offset2[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
};

struct puz_move
{
    int m_dir;
    set<Position> m_hedges, m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    map<Position, vector<puz_move>> m_pos2moves;
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
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
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_NOOK);
                m_pos2num[p] = ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, num] : m_pos2num) {
        auto& moves = m_pos2moves[p];
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            if (set<Position> hedges, empties; [&] {
                for (int j = 0; j < 4; ++j)
                    if (j != i)
                        if (auto p2 = p + offset[j]; cells(p2) != PUZ_BOUNDARY)
                            hedges.insert(p2);
                for (auto p2 = p + os; cells(p2) != PUZ_BOUNDARY && empties.size() < num - 1; p2 += os)
                    empties.insert(p2);
                return empties.size() == num - 1;
            }())
                moves.push_back({i, hedges, empties});
        }
    }

    for (int i = 1; i <= 3; ++i) {
        auto perm = string(i, PUZ_EMPTY) + string(4 - i, PUZ_HEDGE);
        do
            m_perms.push_back(perm);
        while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches_hint) < tie(x.m_cells, x.m_matches_hint);
    }
    bool make_move_hint(const Position& p, int n);
    void make_move_hint2(const Position& p, int n);
    bool make_move_square(const Position& p, int n);
    map<Position, vector<int>>::iterator make_move_square2(const Position& p, int n);
    int find_matches(bool init);
    bool is_interconnected() const;
    bool check_square();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches_hint.size() + m_matches_square.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches_hint, m_matches_square;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
{
    for (auto& [p, moves] : g.m_pos2moves) {
        auto& v = m_matches_hint[p];
        v.resize(moves.size());
        boost::iota(v, 0);
    }

    vector<int> v2(g.m_perms.size());
    boost::iota(v2, 0);
    for (int r = 1; r < g.m_sidelen - 2; ++r)
        for (int c = 1; c < g.m_sidelen - 2; ++c)
            m_matches_square[{r, c}] = v2;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches_hint) {
        auto& moves = m_game->m_pos2moves.at(p);
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_1, hedges, empties] = moves[id];
            return boost::algorithm::any_of(hedges, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_HEDGE;
            }) || boost::algorithm::any_of(empties, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_EMPTY && ch != PUZ_NOOK;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, move_ids[0]), 1;
            }
    }
    return init || check_square() ? 2 : 0;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

inline bool is_maze(char ch) { return ch != PUZ_HEDGE && ch != PUZ_BOUNDARY; }

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; is_maze(m_state->cells(p2))) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

// 4. The resulting maze should be a single one-tile path connected horizontally or vertically
bool puz_state::is_interconnected() const
{
    int i = m_cells.find(PUZ_NOOK);
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return is_maze(cells(p));
    }) == boost::count_if(m_cells, is_maze);
}

bool puz_state::check_square()
{
    for (auto it = m_matches_square.begin(); it != m_matches_square.end();) {
        auto& [p, perm_ids] = *it;
        boost::remove_erase_if(perm_ids, [&](int n) {
            auto& perm = m_game->m_perms[n];
            return !boost::equal(offset2, perm, [&](const Position& os, char ch2) {
                char ch = cells(p + os);
                return ch == PUZ_SPACE || ch == ch2 || ch == PUZ_NOOK && ch2 == PUZ_EMPTY;
            });
        });
        switch (perm_ids.size()) {
        case 0:
            return false;
        case 1:
            it = make_move_square2(p, perm_ids[0]);
            break;
        default:
            ++it;
            break;
        }
    }
    return is_interconnected();
}

void puz_state::make_move_hint2(const Position& p, int n)
{
    auto& [_1, hedges, empties] = m_game->m_pos2moves.at(p)[n];
    for (auto& p2 : hedges)
        cells(p2) = PUZ_HEDGE;
    for (auto& p2 : empties)
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
    ++m_distance, m_matches_hint.erase(p);
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    make_move_hint2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

map<Position, vector<int>>::iterator puz_state::make_move_square2(const Position& p, int n)
{
    auto& perm = m_game->m_perms[n];
    for (int i = 0; i < 4; ++i) {
        Position p2 = p + offset2[i];
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = perm[i];
    }
    return ++m_distance, m_matches_square.erase(m_matches_square.find(p));
}

bool puz_state::make_move_square(const Position& p, int n)
{
    m_distance = 0;
    make_move_square2(p, n);
    return is_interconnected();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches_hint.empty()) {
        auto& [p, move_ids] = *boost::min_element(m_matches_hint, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : move_ids)
            if (children.push_back(*this); !children.back().make_move_hint(p, n))
                children.pop_back();
    } else {
        auto& [p, perm_ids] = *boost::min_element(m_matches_square, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : perm_ids)
            if (children.push_back(*this); !children.back().make_move_square(p, n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_NOOK)
                out << ch << m_game->m_pos2num.at(p);
            else
                out << ch << ch;
            out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Nooks()
{
    using namespace puzzles::Nooks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Nooks.xml", "Puzzles/Nooks.txt", solution_format::GOAL_STATE_ONLY);
}
