#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 14/Busy Seas

    Summary
    More seafaring puzzles

    Description
    1. You are at sea and you need to find the lighthouses and light the boats.
    2. Each boat has a number on it that tells you how many lighthouses are lighting it.
    3. A lighthouse lights all the tiles horizontally and vertically. Its
       light is stopped by the first boat it meets.
    4. Lighthouses can touch boats and other lighthouses.
*/

namespace puzzles::BusySeas{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_BOAT = 'T';
constexpr auto PUZ_LIGHTHOUSE = 'L';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;

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
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(ch);
            else {
                m_cells.push_back(PUZ_BOAT);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

using puz_move = pair<vector<Position>, string>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, const puz_move& kv);
    void make_move2(const Position& p, const puz_move& kv);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<puz_move>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [p, n] : g.m_pos2num)
        m_matches[p];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perms] : m_matches) {
        perms.clear();

        vector<Position> rng;
        int m = m_game->m_pos2num.at(p);
        for (auto& os : offset)
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    rng.push_back(p2);
                else if (ch == PUZ_LIGHTHOUSE)
                    --m;
                else if (ch == PUZ_BOUNDARY || ch == PUZ_BOAT)
                    break;
            }

        if (int n = rng.size(); m >= 0 && m <= n) {
            auto perm = string(n - m, PUZ_EMPTY) + string(m, PUZ_LIGHTHOUSE);
            do
                perms.emplace_back(rng, perm);
            while(boost::next_permutation(perm));
        }

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perms.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, const puz_move& kv)
{
    auto& [rng, perm] = kv;
    for (int i = 0; i < rng.size(); ++i)
        cells(rng[i]) = perm[i];

    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, const puz_move& kv)
{
    m_distance = 0;
    make_move2(p, kv);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perms] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<puz_move>>& kv1,
        const pair<const Position, vector<puz_move>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& kv : perms)
        if (!children.emplace_back(*this).make_move(p, kv))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_BOAT)
                out << format("{:2}", m_game->m_pos2num.at(p));
            else
                out << ' ' << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BusySeas()
{
    using namespace puzzles::BusySeas;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BusySeas.xml", "Puzzles/BusySeas.txt", solution_format::GOAL_STATE_ONLY);
}
