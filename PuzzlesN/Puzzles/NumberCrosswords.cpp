#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Number Crosswords

    Summary
    More crosswords for Robots

    Description
    1. Blacken some tiles, so that some of the numbers remain visible.
    2. Numbers outside the grid show the sums of the numbers in the
       remaining tiles in that row or column.
*/

namespace puzzles{ namespace NumberCrosswords{

#define PUZ_EMPTY        0
#define PUZ_UNKNOWN      -1

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    // all permutations
    map<int, vector<vector<int>>> m_area2perms;
    int cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }

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
            Position p(r, c);
            auto s = str.substr(c * 2, 2);
            int n = s != "  " ? stoi(s) : PUZ_UNKNOWN;
            m_start.push_back(n);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }

    int cnt = m_sidelen - 1;
    vector<int> perm(m_sidelen, 0);
    for (int i = 0; i < cnt; ++i)
        for (int j = 0; j < 2; ++j) {
            int area_id = i + j * m_sidelen;
            auto& area = m_area2range.at(area_id);
            auto& perms = m_area2perms[area_id];
            // 2. Numbers outside the grid show the sums of the numbers in the
            // remaining tiles in that row or column.
            int sum = cells(area.back());
            // An index of 0 means the tile should be blackened.
            // An index of 1 means the tile should not be blackened.
            vector<int> indexes(cnt, 0);
            indexes.push_back(1);
            for (int k = 0; k < cnt;) {
                int sum2 = 0;
                for (int m = 0; m < cnt; ++m)
                    if (indexes[m] == 1)
                        sum2 += cells(area[m]);
                if (sum == PUZ_UNKNOWN || sum == sum2) {
                    for (int m = 0; m < cnt; ++m)
                        perm[m] = indexes[m] == 0 ? 0 : cells(area[m]);
                    perm.back() = sum2;
                    perms.push_back(perm);
                }
                for (k = 0; k < cnt && ++indexes[k] == 2; ++k)
                    indexes[k] = 0;
            }
        }
}

struct puz_state
{
    puz_state() {}
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
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<int> m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (int i = 0; i < sidelen() - 1; ++i)
        for (int j = 0; j < 2; ++j) {
            int area_id = i + j * sidelen();
            vector<int> perm_ids(g.m_area2perms.at(area_id).size());
            boost::iota(perm_ids, 0);
            m_matches[area_id] = perm_ids;
            if (j == 0)
                for (const auto& p : g.m_area2range.at(area_id))
                    cells(p) = PUZ_UNKNOWN;
        }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;
        auto& perms = m_game->m_area2perms.at(area_id);

        vector<int> digits;
        for (auto& p : m_game->m_area2range[kv.first])
            digits.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(digits, perms[id], [](int n1, int n2) {
                return n1 == PUZ_UNKNOWN || (n1 == PUZ_EMPTY) == (n2 == PUZ_EMPTY);
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
    auto& perm = m_game->m_area2perms.at(i)[j];

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
            Position p(r, c);
            int n = cells(p);
            if (n == PUZ_UNKNOWN || n == PUZ_EMPTY)
                out << (n == PUZ_UNKNOWN ? "  " : " .");
            else
                out << boost::format("%2d") % n;
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_NumberCrosswords()
{
    using namespace puzzles::NumberCrosswords;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberCrosswords.xml", "Puzzles/NumberCrosswords.txt", solution_format::GOAL_STATE_ONLY);
}
