#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle_p.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 1/Abc

    Summary
    Fill the board with ABC

    Description
    1. The goal is to put the letter A B and C in the board.
    2. Each letter appears once in every row and column.
    3. The letters on the borders tell you what letter you see from there.
    4. Since most puzzles can contain empty spaces, the hint on the board
       doesn't always match the tile next to it.
    5. Bigger puzzles can contain the letter 'D'. In these cases, the name
       of the puzzle is 'Abcd'. Further on, you might also encounter 'E',
       'F' etc.
*/

namespace puzzles::Abc{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    char m_letter_max;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_area2range;
    // all permutations
    // A space A B C C
    // A space A C B B
    // ...
    // C C B A space A
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    m_cells = boost::accumulate(strs, string());
    m_letter_max = *boost::max_element(m_cells);
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }

    string perm(m_sidelen, PUZ_EMPTY);
    auto begin = next(perm.begin()), end = prev(perm.end());
    auto rbegin = next(perm.rbegin()), rend = prev(perm.rend());
    // 1. The goal is to put the letter A B and C in the board.
    // 2. Each letter appear once in every row and column.
    // space ... A B C
    iota(next(begin, m_sidelen - 2 - (m_letter_max - 'A' + 1)), end, 'A');
    do {
        // 3. The letters on the borders tell you what letter you see from there.
        // determine the letter on the left and top borders
        *perm.begin() = *find_if(begin, end, [](char ch) {return ch != PUZ_EMPTY;});
        // determine the letter on the right and bottom borders
        *perm.rbegin() = *find_if(rbegin, rend, [](char ch) {return ch != PUZ_EMPTY;});
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
    void format_to_string(back_insert_iterator<string> it) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
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
    for (auto& [area_id, perm_ids] : m_matches) {
        string chars;
        for (auto& p : m_game->m_area2range[area_id])
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
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1, 
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(area_id, n))
            children.pop_back();
}

void puz_state::format_to_string(back_insert_iterator<string> it) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            format_to(it, "{} ", cells({r, c}));
        format_to(it, "\n");
    }
}

}

void solve_puz_Abc()
{
    using namespace puzzles::Abc;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Abc.xml", "Puzzles/Abc.txt", solution_format::GOAL_STATE_ONLY);
}
