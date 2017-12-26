#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 2/Futoshiki

    Summary
    Fill the rows and columns with numbers, respecting the relations

    Description
    1. In a manner similar to Sudoku, you have to put in each row and column
       numbers ranging from 1 to N, where N is the puzzle board size.
    2. The hints you have are the 'less than'/'greater than' signs between tiles.
    3. Remember you can't repeat the same number in a row or column.

    Variation
    4. Some boards, instead of having less/greater signs, have just a line
       separating the tiles.
    5. That separator hints at two tiles with consecutive numbers, i.e. 1-2
       or 3-4..
    6. Please note that in this variation consecutive numbers MUST have a
       line separating the tiles. Otherwise they're not consecutive.
    7. This Variation is a taste of a similar game: 'Consecutives'.
*/

namespace puzzles{ namespace Futoshiki{

#define PUZ_SPACE        ' '
#define PUZ_ROW_LT        '<'
#define PUZ_ROW_GT        '>'
#define PUZ_ROW_CS        '|'
#define PUZ_COL_LT        '^'
#define PUZ_COL_GT        'v'
#define PUZ_COL_CS        '-'
#define PUZ_NOT_CS        '.'

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<vector<Position>> m_area2range;
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    m_start = boost::accumulate(strs, string());
    boost::replace(m_start, PUZ_COL_LT, PUZ_ROW_LT);
    boost::replace(m_start, PUZ_COL_GT, PUZ_ROW_GT);
    boost::replace(m_start, PUZ_COL_CS, PUZ_ROW_CS);
    bool ltgt_mode = m_start.find(PUZ_ROW_CS) == -1;

    for (int r = 0; r < m_sidelen; ++r) {
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }

    string perm(m_sidelen / 2 + 1, PUZ_SPACE);
    string perm2(m_sidelen, PUZ_SPACE);

    boost::iota(perm, '1');
    do{
        for (int i = m_sidelen - 1, j = i / 2;; i -= 2, --j) {
            perm2[i] = perm[j];
            if (i == 0) break;
            perm2[i - 1] = ltgt_mode ? 
                perm[j - 1] < perm[j] ? PUZ_ROW_LT : PUZ_ROW_GT :
                myabs(perm[j - 1] - perm[j]) == 1 ? PUZ_ROW_CS : PUZ_NOT_CS;
        }
        m_perms.push_back(perm2);
    }while(boost::next_permutation(perm));
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
    bool is_goal_state() const {return get_heuristic() == 0;}
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

    for (int i = 0; i < sidelen(); i += 2)
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
        for (auto& p : m_game->m_area2range[area_id])
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE && ch2 != PUZ_ROW_CS || ch1 == ch2;
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
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            if (r % 2 == 1)
                ch = ch == PUZ_ROW_LT ? PUZ_COL_LT :
                    ch == PUZ_ROW_GT ? PUZ_COL_GT :
                    ch == PUZ_ROW_CS ? PUZ_COL_CS : ch;
            out << ch << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Futoshiki()
{
    using namespace puzzles::Futoshiki;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Futoshiki.xml", "Puzzles/Futoshiki.txt", solution_format::GOAL_STATE_ONLY);
}
