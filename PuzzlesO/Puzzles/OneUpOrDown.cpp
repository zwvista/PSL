#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 3/Puzzle Set 1/One Up or Down

    Summary
    Plus or minus

    Description
    1. Fill the board like a Sudoku.
    2. If the difference between two adjacent numbers is 1 (1 and 2, 2 and 3,
       3 and 4, etc) there will be a sign between them.
    3. If the sign is between two tiles on the same row:
    4. The plus sign means the tile on the right will be [number on the left] + 1.
    5. The minus sign means the tile on the right will be [number on the left] - 1.
    6. If the sign is between two tiles on the same column:
    7. The plus sign means the tile on the bottom will be [number on the top] + 1.
    8. The minus sign means the tile on the bottom will be [number on the top] - 1.
*/

namespace puzzles::OneUpOrDown{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_PLUS = '+';
constexpr auto PUZ_MINUS = '-';
    
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
    string m_start;
    map<int, vector<Position>> m_index2area;
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_start.push_back(r % 2 == 0 && c % 2 == 0 || ch != PUZ_SPACE ? ch : PUZ_EMPTY);
            Position p(r, c);
            if (r % 2 == 0)
                m_index2area[r].push_back(p);
            if (c % 2 == 0)
                m_index2area[m_sidelen + c].push_back(p);
        }
    }

    int n = m_sidelen / 2 + 1;
    string perm(m_sidelen, PUZ_SPACE);
    string perm2(n, PUZ_SPACE);
    boost::iota(perm2, '1');
    do {
        perm[0] = perm2[0];
        for (int i = 1; i < n; ++i) {
            char ch = perm2[i];
            int d = ch - perm2[i - 1];
            perm[i * 2] = ch;
            perm[i * 2 - 1] = d == 1 ? PUZ_PLUS : d == -1 ? PUZ_MINUS : PUZ_EMPTY;
        }
        m_perms.push_back(perm);
    } while (boost::next_permutation(perm2));
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

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    vector<int> v(g.m_perms.size());
    boost::iota(v, 0);
    for (auto& [index, area] : g.m_index2area)
        m_matches[index] = v;
    find_matches(false);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& [area_id, perm_ids] : m_matches) {
        string chars;
        for (auto& p : m_game->m_index2area.at(area_id))
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
    auto& range = m_game->m_index2area.at(i);
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
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(area_id, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({ r, c });
        println(out);
    }

    return out;
}

}

void solve_puz_OneUpOrDown()
{
    using namespace puzzles::OneUpOrDown;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/OneUpOrDown.xml", "Puzzles/OneUpOrDown.txt", solution_format::GOAL_STATE_ONLY);
}
