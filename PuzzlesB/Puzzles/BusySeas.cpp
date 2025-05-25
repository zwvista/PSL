#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 14/Busy Seas

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
constexpr auto PUZ_BOUNDARY = 'B';

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
    map<Position, int> m_pos2num;
    string m_start;

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
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_BOAT);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

typedef pair<vector<Position>, string> puz_move;

struct puz_state
{
    puz_state() {}
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

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<puz_move>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_pos2num)
        m_matches[kv.first];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perms = kv.second;
        perms.clear();

        vector<Position> rng_s, rng_l;
        for (auto& os : offset)
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    rng_s.push_back(p2);
                else if (ch == PUZ_LIGHTHOUSE)
                    rng_l.push_back(p2);
                else if (ch == PUZ_BOUNDARY || ch == PUZ_BOAT)
                    break;
            }

        int ns = rng_s.size(), nl = rng_l.size();
        int n = m_game->m_pos2num.at(p), m = n - nl;

        if (m >= 0 && m <= ns) {
            auto perm = string(ns - m, PUZ_EMPTY) + string(m, PUZ_LIGHTHOUSE);
            vector<Position> rng(m);
            do {
                for (int i = 0, j = 0; i < perm.length(); ++i)
                    if (perm[i] == PUZ_LIGHTHOUSE)
                        rng[j++] = rng_s[i];
                perms.emplace_back(rng_s, perm);
            } while(boost::next_permutation(perm));
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
    auto& rng = kv.first;
    auto& str = kv.second;
    for (int i = 0; i < rng.size(); ++i)
        cells(rng[i]) = str[i];

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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<puz_move>>& kv1,
        const pair<const Position, vector<puz_move>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& kv2 : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, kv2))
            children.pop_back();
    }
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
