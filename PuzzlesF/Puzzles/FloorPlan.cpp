#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 3/Puzzle Set 1/Floor Plan

    Summary
    Blueprints to fill in

    Description
    1. The board represents a blueprint of an office floor.
    2. Cells with a number represent an office. On the floor every office is
       interconnected and can be reached by every other office.
    3. The number on a cell indicates how many offices it connects to. No two
       offices with the same number can be adjacent.
*/

namespace puzzles::FloorPlan{

#define PUZ_BOUNDARY          '+'
#define PUZ_SPACE             ' '
#define PUZ_EMPTY             '.'
#define PUZ_NUM               'N'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
const Position offset2[] = {
    {0, 0},         // o
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<vector<char>> m_perms;

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
            char ch = str[c - 1];
            m_start.push_back(ch);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (int i = 1; i <= 4; ++i) {
        vector<char> v(1, i == 0 ? PUZ_EMPTY : i + '0');
        v.insert(v.end(), 4 - i, PUZ_EMPTY);
        v.insert(v.end(), i, PUZ_NUM);
        auto begin = v.begin() + 1, end = v.end();
        do
            m_perms.push_back(v);
        while (next_permutation(begin, end));
    }
    auto vv = m_perms;
    for (auto& v : vv)
        v[0] = PUZ_EMPTY;
    m_perms.emplace_back(5, PUZ_EMPTY);
    m_perms.insert(m_perms.end(), vv.begin(), vv.end());
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(Position p, int n);
    bool make_move2(Position p, int n);
    int find_matches(bool init);
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    vector v(g.m_perms.size(), 0);
    boost::iota(v, 0);
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c)
            m_matches[{r, c}] = v;
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto p = kv.first;
        auto& perm_ids = kv.second;

        vector<char> chars;
        for (auto& os : offset2)
            chars.push_back(cells(p + os));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = m_game->m_perms[id];
            return !boost::equal(chars, perm, [&](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2 || ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY ||
                    ch1 == PUZ_NUM && isdigit(ch2) || isdigit(ch1) && ch2 == PUZ_NUM;
            }) || boost::algorithm::any_of(offset, [&](const Position& os) {
                char ch1 = cells(p + os), ch2 = perm[0];
                return isdigit(ch1) && isdigit(ch2) && ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p_start) : m_state(s) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (char ch = m_state->cells(p); ch != PUZ_EMPTY && ch != PUZ_BOUNDARY) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::is_continuous() const
{
    int i = boost::range::find_if(m_cells, [&](char ch) {
        return ch != PUZ_EMPTY && ch != PUZ_BOUNDARY;
    }) - m_cells.begin();
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves({ this, Position(i / sidelen(), i % sidelen()) }, smoves);
    return boost::count_if(smoves, [&](const puz_state2& s2) {
        return cells(s2) != PUZ_SPACE;
    }) == boost::count_if(m_cells, [&](char ch) {
        return ch != PUZ_SPACE && ch != PUZ_EMPTY && ch != PUZ_BOUNDARY;
    });
}

bool puz_state::make_move2(Position p, int n)
{
    auto& perm = m_game->m_perms[n];
    for (int i = 0; i < 5; ++i) {
        auto& os = offset2[i];
        char& ch = cells(p + os);
        if (ch == PUZ_SPACE || ch == PUZ_NUM)
            ch = perm[i];
    }

    ++m_distance;
    m_matches.erase(p);
    return is_continuous();
}

bool puz_state::make_move(Position p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position&, vector<int>>& kv1,
        const pair<const Position&, vector<int>>& kv2) {
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
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            char ch = cells({ r, c });
            out << ch << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_FloorPlan()
{
    using namespace puzzles::FloorPlan;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FloorPlan.xml", "Puzzles/FloorPlan.txt", solution_format::GOAL_STATE_ONLY);
}
