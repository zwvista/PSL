#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 4/Landscaper

    Summary
    Plant Trees and Flowers with enough variety 

    Description
    1. Your goal as a landscaper is to plant some Trees and Flowers on the
       field, in every available tile.
    2. In doing this, you must assure the scenery is varied enough:
    3. No more than two consecutive Trees or Flowers should appear horizontally
       or vertically.
    4. Every row and column should have an equal number of Trees and Flowers.
    5. Each row disposition must be unique, i.e. the same arrangement of Trees
       and Flowers can't appear on two rows.
    6. Each column disposition must be unique as well.

    Odd-size levels
    7. Please note that in odd-size levels, the number of Trees and Flowers
       obviously won't be equal on a row or column. However each row and
       column will have the same number of Flowers and Trees.
    8. Also, the number of Trees will always be greater than that of Flowers
       (i.e. 3 Flowers and 4 Trees, 4 Flowers and 5 Trees, etc).
*/

namespace puzzles::Landscaper{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_FLOWER = 'F';

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
, m_area2range(m_sidelen * 2)
{
    m_cells = boost::accumulate(strs, string());

    for (int r = 0; r < m_sidelen; ++r) {
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }

    string perm(m_sidelen, PUZ_SPACE);
    auto f = [&](int n1, int n2) {
        for (int i = 0; i < n1; ++i)
            perm[i] = PUZ_FLOWER;
        for (int i = 0; i < n2; ++i)
            perm[i + n1] = PUZ_TREE;
        do {
            bool no_more_than_two = true;
            for (int i = 1, n = 1; i < m_sidelen; ++i) {
                n = perm[i] == perm[i - 1] ? n + 1 : 1;
                if (n > 2) {
                    no_more_than_two = false;
                    break;
                }
            }
            if (no_more_than_two)
                m_perms.push_back(perm);
        } while(boost::next_permutation(perm));
    };
    if (m_sidelen % 2 == 0)
        f(m_sidelen / 2, m_sidelen / 2);
    else
        f(m_sidelen / 2, m_sidelen / 2 + 1);
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
    set<int> m_perm_id_rows, m_perm_id_cols;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int i = 0; i < sidelen(); ++i)
        m_matches[i] = m_matches[sidelen() + i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& p2 : m_game->m_area2range[p])
            chars.push_back(cells(p2));

        auto& ids = p < sidelen() ? m_perm_id_rows : m_perm_id_cols;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, m_game->m_perms.at(id), [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }) || ids.contains(id);
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_area2range[i];
    auto& perm = m_game->m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area[k]) = perm[k];

    auto& ids = i < sidelen() ? m_perm_id_rows : m_perm_id_cols;
    ids.insert(j);

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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << ch << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Landscaper()
{
    using namespace puzzles::Landscaper;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Landscaper.xml", "Puzzles/Landscaper.txt", solution_format::GOAL_STATE_ONLY);
}
