#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Castle Bailey

    Summary
    Towers, keeps and curtain walls

    Description
    1. A very convoluted Medieval Architect devised a very weird Bailey
       (or Ward, the inner area contained in the Outer Walls of a Castle).
    2. He deployed quite a few towers (the circles) and put a number on
       top of each one.
    3. The number tells you how many pieces (squares) of wall it touches.
    4. So the number can go from 0 (no walls around the tower) to 4 (tower
       entirely surrounded by walls).
    5. Board borders don't count as walls, so there you'll have two walls
       at most (or one in corners).
    6. To facilitate movement in the castle, the Bailey must have a single
       continuous area (Garden).
*/

namespace puzzles::CastleBailey{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {0, 0},    // nw
    {0, 1},    // ne
    {1, 1},    // se
    {1, 0},    // sw
};

constexpr Position offset2[] = {
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
    vector<vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 1)
, m_num2perms(5)
{
    for (int r = 0; r < m_sidelen - 1; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str[c];
            if (ch != ' ')
                m_pos2num[{r, c}] = ch - '0';
        }
    }

    for (int i = 0; i <= 4; ++i) {
        auto& perms = m_num2perms[i];
        auto perm = string(4 - i, PUZ_EMPTY) + string(i, PUZ_WALL);
        do
            perms.push_back(perm);
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
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    bool make_move_space(const Position& p, char ch);
    int find_matches(bool init);
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < sidelen(); ++i)
        cells({i, 0}) = cells({i, sidelen() - 1}) =
        cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

    for (auto& [p, n] : g.m_pos2num) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_num2perms[n].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& os : offset)
            chars.push_back(cells(p + os));

        auto& perms = m_game->m_num2perms[m_game->m_pos2num.at(p)];
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY ||
                    ch1 == PUZ_SPACE || ch1 == ch2;
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

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const Position& starting)
        : m_state(&s) { make_move(starting); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset2) {
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (ch == PUZ_SPACE || ch == PUZ_EMPTY) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_continuous() const
{
    int i = m_cells.find(PUZ_EMPTY);
    if (i == -1)
        return true;
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        {*this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return cells(p) == PUZ_EMPTY;
    }) == boost::count(m_cells, PUZ_EMPTY);
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];

    for (int k = 0; k < perm.size(); ++k) {
        char& ch = cells(p + offset[k]);
        if (ch == PUZ_SPACE)
            ch = perm[k], ++m_distance;
    }

    ++m_distance;
    m_matches.erase(p);
    return is_continuous();
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

bool puz_state::make_move_space(const Position& p, char ch)
{
    m_distance = 1;
    cells(p) = ch;
    return is_continuous();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : perm_ids) {
            children.push_back(*this);
            if (!children.back().make_move(p, n))
                children.pop_back();
        }
    } else {
        int i = m_cells.find(PUZ_SPACE);
        Position p(i / sidelen(), i % sidelen());
        for (char ch : {PUZ_EMPTY, PUZ_WALL}) {
            children.push_back(*this);
            if (!children.back().make_move_space(p, ch))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_CastleBailey()
{
    using namespace puzzles::CastleBailey;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CastleBailey.xml", "Puzzles/CastleBailey.txt", solution_format::GOAL_STATE_ONLY);
}
