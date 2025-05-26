#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 4/Sukrokuro

    Summary
    All mixed up!

    Description
    1. Sukrokuro combines Sudoku, Kropki and Kakuro.
    2. Fill in the white cells, one number in each, so that each column and row
       contains the nubmers 1 through 7 exactly once.
    3. Black cells contain clues, which tell you the sum of the number in
       consecutive cells at its right and downward.
    4. A dot separated tiles where the absolute difference between the numbers
       is 1.
    5. If a dot is absent between two cells, the difference between the numbers
       must be more than 1.
*/

namespace puzzles::Sukrokuro{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_DOT = '.';

struct puz_area
{
    vector<Position> m_range;
    int m_sum;
};

typedef pair<Position, bool> puz_key;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<puz_key, puz_area> m_pos2area;
    vector<vector<Position>> m_rowscols;
    map<puz_key, bool> m_blanks;
    vector<vector<int>> m_perms;
    map<int, vector<int>> m_rc2permids;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_rowscols.resize(m_sidelen * 2);
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            const string s1 = str.substr(c * 4, 2);
            const string s2 = str.substr(c * 4 + 2, 2);
            if (s1[0] == ' ') {
                m_blanks[{p, true}] = s1[1] == PUZ_DOT;
                m_blanks[{p, false}] = s2[1] == PUZ_DOT;
                m_rowscols[r].push_back(p);
                m_rowscols[m_sidelen + c].push_back(p);
            } else {
                int n1 = stoi(s1);
                if (n1 != 0)
                    m_pos2area[{p, true}].m_sum = n1;
                int n2 = stoi(s2);
                if (n2 != 0)
                    m_pos2area[{p, false}].m_sum = n2;
            }
        }
    }

    vector<int> perm(7);
    boost::iota(perm, 1);
    do
        m_perms.push_back(perm);
    while (boost::next_permutation(perm));

    for (int index = 0; index < m_sidelen * 2; ++index) {
        auto& rc = m_rowscols[index];
        if (rc.size() != 7) continue;
        auto& perm_ids = m_rc2permids[index];
        perm_ids.resize(m_perms.size());
        boost::iota(perm_ids, 0);
    }
    
    for (auto& kv : m_pos2area) {
        const auto& p = kv.first.first;
        bool is_vert = kv.first.second;
        auto& area = kv.second;
        auto os = is_vert ? Position(1, 0) : Position(0, 1);
        auto& rng = area.m_range;
        for (auto p2 = p + os; m_blanks.contains({p2, is_vert}); p2 += os)
            rng.push_back(p2);
        int index = is_vert ? m_sidelen + p.second : p.first;
        auto& perm_ids = m_rc2permids.at(index);
        auto& rc = m_rowscols[index];
        int n = boost::range::find(rc, rng[0]) - rc.begin(), sz = rng.size();
        boost::remove_erase_if(perm_ids, [&](int id){
            auto& perm = m_perms[id];
            for (int i = n; i < n + sz - 1; ++i)
                if (m_blanks.at({rc[i], is_vert}) != (abs(perm[i] - perm[i + 1]) == 1))
                    return true;
            int sum = 0;
            for (int i = n; i < n + sz; ++i)
                sum += perm[i];
            return sum != area.m_sum;
        });
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, int> m_cells;
    map<int, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_matches(g.m_rc2permids)
{
    for(auto&& [kv, isdiff1] : g.m_blanks)
        m_cells[kv.first];
        
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int index = kv.first;
        auto& perm_ids = kv.second;

        vector<int> nums;
        for (auto& p : m_game->m_rowscols[index])
            nums.push_back(m_cells.at(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = m_game->m_perms[id];
            return !boost::equal(nums, perm, [](int n1, int n2) {
                return n1 == 0 || n1 == n2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(index, perm_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_rowscols[i];
    auto& perm = m_game->m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        m_cells.at(range[k]) = perm[k];

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
    for (int r = 0; r < m_game->m_sidelen; ++r) {
        for (int c = 0; c < m_game->m_sidelen; ++c) {
            Position p(r, c);
            out << (m_game->m_blanks.contains({p, true}) ? char(m_cells.at(p) + '0') : '.') << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Sukrokuro()
{
    using namespace puzzles::Sukrokuro;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Sukrokuro.xml", "Puzzles/Sukrokuro.txt", solution_format::GOAL_STATE_ONLY);
}
