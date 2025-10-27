#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 9/Skydoku

    Summary
    Takes Sudoku to new heights!

    Description
    1. Plays like a cross between Sudoku and Skyscrapers.
    2. Hints on the borders tell you -like Skyscrapers- how many buildings
       you see from there.
    3. In the inner board regular Sudoku rules apply, except that in some
       levels you only have numbers 1 to 8.
*/

namespace puzzles::Skydoku{

constexpr auto PUZ_SPACE = ' ';

struct puz_area_info
{
    vector<Position> m_range;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    vector<vector<Position>> m_area2range;
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 3)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_cells.push_back(ch);
            Position p(r, c);
            if (r > 0 && r < m_sidelen - 1)
                m_area2range[r].push_back(p);
            if (c > 0 && c < m_sidelen - 1)
                m_area2range[m_sidelen + c].push_back(p);
            if (r > 0 && r < m_sidelen - 1 && c > 0 && c < m_sidelen - 1)
                if (m_sidelen == 10)
                    m_area2range[20 + (r - 1) / 2 * 2 + 1 + (c - 1) / 4].push_back(p);
                else
                    m_area2range[22 + (r - 1) / 3 * 3 + 1 + (c - 1) / 3].push_back(p);
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

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int i = 1; i < sidelen() - 1; ++i)
        m_matches[i] = m_matches[sidelen() + i] = m_matches[sidelen() * 2 + i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& range = m_game->m_area2range[area_id];

        string nums;
        for (auto& p : range)
            nums.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto perm = m_game->m_perms[id];
            if (range.size() < sidelen())
                perm = perm.substr(1, range.size());
            return !boost::equal(nums, perm, [](char ch1, char ch2) {
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
    auto perm = m_game->m_perms[j];
    if (range.size() < sidelen())
        perm = perm.substr(1, range.size());

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
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1, 
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(area_id, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_Skydoku()
{
    using namespace puzzles::Skydoku;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Skydoku.xml", "Puzzles/Skydoku.txt", solution_format::GOAL_STATE_ONLY);
}
