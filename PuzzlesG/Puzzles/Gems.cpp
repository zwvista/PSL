#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Gems

    Summary
    Gemscraper

    Description
    1. The board contains one Sapphire (Blue Gem) on each row and column.
    2. There are also a random amount of Pebbles (in White) on the board.
    3. A number on the border tells you how many stones you can see from
       there, up to and including the Sapphire.
    4. The Sapphire (blue) hide the Pebbles (white) behind them.
*/

namespace puzzles{ namespace Gems{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_GEM          'G'
#define PUZ_PEBBLE       'P'

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    // all permutations
    // ..GP
    // ..PG
    // ....
    // PG..
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }

    // 1. The board contains one Sapphire (Blue Gem) on each row and column.
    // 2. There are also a random amount of Pebbles (in White) on the board.
    for (int i = 0; i < m_sidelen - 2; ++i) {
        auto perm = string(m_sidelen - i - 2, PUZ_EMPTY) + PUZ_GEM + string(i, PUZ_PEBBLE) + PUZ_EMPTY;
        auto begin = next(perm.begin()), end = prev(perm.end());
        auto rbegin = next(perm.rbegin()), rend = prev(perm.rend());
        do {
            // 3. A number on the border tells you how many stones you can see from
            //    there, up toand including the Sapphire.
            // 4. The Sapphire(blue) hide the Pebbles(white) behind them.
            int n = 0;
            for (auto it = begin;; ++it) {
                if (*it == PUZ_GEM || *it == PUZ_PEBBLE)
                    ++n;
                if (*it == PUZ_GEM) break;
            }
            *perm.begin() = char(n + '0');
            n = 0;
            for (auto it = rbegin;; ++it) {
                if (*it == PUZ_GEM || *it == PUZ_PEBBLE)
                    ++n;
                if (*it == PUZ_GEM) break;
            }
            *perm.rbegin() = char(n + '0');
            m_perms.push_back(perm);
        } while(next_permutation(begin, end));
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);

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
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int i = 1; i < sidelen() - 1; ++i)
        m_matches[i] = m_matches[sidelen() + i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;

        string chars;
        for (auto& p : m_game->m_area2range[kv.first])
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_area2range[i];
    auto& perm = m_game->m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(range[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1, 
        const pair<const int, vector<int>>& kv2) {
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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        out << endl;
    }
    return out;
}

}}

void solve_puz_Gems()
{
    using namespace puzzles::Gems;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Gems.xml", "Puzzles/Gems.txt", solution_format::GOAL_STATE_ONLY);
}
