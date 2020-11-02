#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 3/Puzzle Set 1/Picnic

    Summary
    Fling the Blanket

    Description
    1. As usual, on the day of the National Holiday Picnic, the park is crowded.
    2. You brought your picnic basket (like everyone else) and your blanket (like
       everyone else).
    3. The object is to make space for everyone and to leave the park open for
       walking around.
    4. find a way to lay every picnic basket so that no blanket touches another
       one, horizontally or vertically.
    5. Also the remaining park should be accessible to everyone, so empty grass
       spaces should form a single continuous area.
    6. The number on top of the basket shows you how many tiles the basket must
       be flung.
*/

namespace puzzles::Picnic{

#define PUZ_BOUNDARY          '+'
#define PUZ_SPACE             ' '
#define PUZ_EMPTY             '.'
#define PUZ_BLANKET           'B'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    map<Position, vector<Position>> m_pos2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
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
            if (ch != ' ')
                m_pos2num[p] = ch - '0';
            m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto&& [p, n] : m_pos2num) {
        auto& perms = m_pos2perms[p];
        for (auto& os : offset) {
            auto p2 = p;
            for (int i = 0; i < n; ++i) {
                if (cells(p2 += os) != PUZ_SPACE)
                    goto next;
            }
            perms.push_back(p2);
        next:;
        }
    }
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
    bool make_move(Position p_basket, Position p_blanket);
    bool make_move2(Position p_basket, Position p_blanket);
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
    map<Position, vector<Position>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start), m_matches(g.m_pos2perms)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto p_basket = kv.first;
        auto& perms = kv.second;

        boost::remove_erase_if(perms, [&](const Position& p_blanket) {
            return cells(p_blanket) != PUZ_SPACE;
        });

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p_basket, perms[0]) ? 1 : 0;
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
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        if (char ch = m_state->cells(p); ch == PUZ_SPACE || ch == PUZ_EMPTY) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::is_continuous() const
{
    int i = boost::range::find_if(m_cells, [&](char ch) {
        return ch == PUZ_SPACE || ch == PUZ_EMPTY;
    }) - m_cells.begin();
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves({ this, Position(i / sidelen(), i % sidelen()) }, smoves);
    return smoves.size() == boost::count_if(m_cells, [&](char ch) {
        return ch == PUZ_SPACE || ch == PUZ_EMPTY;
    });
}

bool puz_state::make_move2(Position p_basket, Position p_blanket)
{
    cells(p_blanket) = PUZ_BLANKET;
    for (auto& os : offset) {
        char& ch = cells(p_basket + os);
        if (ch != PUZ_BOUNDARY)
            ch = PUZ_EMPTY;
    }

    ++m_distance;
    m_matches.erase(p_basket);
    return is_continuous();
}

bool puz_state::make_move(Position p_basket, Position p_blanket)
{
    m_distance = 0;
    make_move2(p_basket, p_blanket);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position&, vector<Position>>& kv1,
        const pair<const Position&, vector<Position>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& p_blanket : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, p_blanket))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            out << cells(p) << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Picnic()
{
    using namespace puzzles::Picnic;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Picnic.xml", "Puzzles/Picnic.txt", solution_format::GOAL_STATE_ONLY);
}
