#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 1/Skyscrapers

    Summary
    Guess skyscrapers heights, judging from the skyline

    Description
    1. The grid in the center represents a city from above. Each cell contain
       a skyscraper, of different height.
    2. The goal is to guess the height of each Skyscraper.
    3. Each row and column can't have two Skyscrapers of the same height.
    4. The numbers on the boarders tell you how many skyscrapers you see from
       there, keeping mind that a higher skyscraper hides a lower one. 
       Skyscrapers are numbered from 1(lowest) to the grid size(highest).
    5. Each row and column can't have similar Skyscrapers.
*/

namespace puzzles{ namespace Skyscrapers{

#define PUZ_SPACE        ' '

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    // all permutations
    // 4 1 2 3 4 1
    // 3 1 2 4 3 2
    // ...
    // 1 4 3 2 1 4
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_start.push_back(ch);
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }

    string perm(m_sidelen, ' ');
    auto f = [&](int border, int start, int end, int step) {
        int h = 0;
        perm[border] = '0';
        for (int i = start; i != end; i += step)
            if (perm[i] > h)
                h = perm[i], ++perm[border];
    };

    auto begin = next(perm.begin()), end = prev(perm.end());
    iota(begin, end, '1');
    do {
        f(0, 1, m_sidelen - 1, 1);
        f(m_sidelen - 1, m_sidelen - 2, 0, -1);
        m_perms.push_back(perm);
    } while(next_permutation(begin, end));
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
: m_cells(g.m_start), m_game(&g)
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

        string nums;
        for (auto& p : m_game->m_area2range[area_id])
            nums.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](char ch1, char ch2) {
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

void solve_puz_Skyscrapers()
{
    using namespace puzzles::Skyscrapers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Skyscrapers.xml", "Puzzles/Skyscrapers.txt", solution_format::GOAL_STATE_ONLY);
}
