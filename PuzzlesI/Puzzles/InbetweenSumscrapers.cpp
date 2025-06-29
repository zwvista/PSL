#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 6/Inbetween Sumscrapers

    Summary
    Sumscrapers on the ground

    Description
    1. Find two Skyscrapers and fill the remaining cells with numbers.
    2. Each row and column contains two skyscrapers.
    3. The remaining cells contain numbers increasing from 1 to N-2 (N being
       the board size).
    4. Numbers appear once in every row and column.
    5. Hints on the border give you the sums of the numbers between the skyscrapers.
*/

namespace puzzles::InbetweenSumscrapers{

constexpr auto PUZ_SKYSCRAPER = -1;
constexpr auto PUZ_SPACE = -2;
constexpr auto PUZ_SKYSCRAPER_S = " X";
constexpr auto PUZ_SPACE_S = "  ";

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    // all permutations
    vector<vector<int>> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            auto s = str.substr(c * 2, 2);
            int n = s == "  " ? PUZ_SPACE : stoi(string(s));
            m_cells.push_back(n);
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }

    // space 1 2 3 Skyscraper Skyscraper
    vector<int> perm(m_sidelen);
    perm[0] = perm[1] = PUZ_SKYSCRAPER;
    auto begin = perm.begin(), end = prev(perm.end());
    iota(next(begin, 2), end, 1);
    do {
        // 5. Hints on the border give you the sums of the numbers between the skyscrapers.
        int sum = -1;
        for (auto it = begin; ; ++it)
            if (int n = *it; n == PUZ_SKYSCRAPER)
                if (sum == -1)
                    sum = 0;
                else
                    break;
            else if (sum >= 0)
                sum += n;
        *end = sum;
        m_perms.push_back(perm);
    } while(next_permutation(begin, end));
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
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
    vector<int> m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int i = 0; i < sidelen() - 1; ++i)
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
            return !boost::equal(chars, perms[id], [](int n1, int n2) {
                return n1 == PUZ_SPACE || n1 == n2;
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

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            if (int n = cells({r, c}); n == PUZ_SPACE)
                out << PUZ_SPACE_S;
            else if (n == PUZ_SKYSCRAPER)
                out << PUZ_SKYSCRAPER_S;
            else
                out << format("{:2}", n);
        println(out);
    }
    return out;
}

}

void solve_puz_InbetweenSumscrapers()
{
    using namespace puzzles::InbetweenSumscrapers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/InbetweenSumscrapers.xml", "Puzzles/InbetweenSumscrapers.txt", solution_format::GOAL_STATE_ONLY);
}
