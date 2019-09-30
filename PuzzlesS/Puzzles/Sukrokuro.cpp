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

#define PUZ_SPACE        ' '
#define PUZ_DOT          '.'

struct puz_area
{
    // elem.key : the position
    // elem.value : whether there is a dot between the cell and the one next to it
    vector<pair<Position, bool>> m_range;
    int m_sum;
};

typedef pair<Position, bool> puz_key;

struct puz_game
{
    string m_id;
    Position m_size;
    map<puz_key, puz_area> m_pos2area;
    vector<vector<Position>> m_rowscols;
    map<puz_key, bool> m_blanks;
    vector<vector<int>> m_perms;
    map<int, vector<int>> m_rc2permids;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_size(strs.size(), strs[0].length() / 4)
{
    m_rowscols.resize(rows() + cols());
    for (int r = 0; r < rows(); ++r) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            const string s1 = str.substr(c * 4, 2);
            const string s2 = str.substr(c * 4 + 2, 2);
            if (s1[0] == ' ') {
                m_blanks[{p, true}] = s1[1] == PUZ_DOT;
                m_blanks[{p, false}] = s2[1] == PUZ_DOT;
                m_rowscols[r].push_back(p);
                m_rowscols[rows() + c].push_back(p);
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

    for (auto& kv : m_pos2area) {
        const auto& p = kv.first.first;
        bool is_vert = kv.first.second;
        auto os = is_vert ? Position(1, 0) : Position(0, 1);
        auto& rng = kv.second.m_range;
        for (auto p2 = p + os; m_blanks.count({p2, is_vert}) != 0; p2 += os)
            rng.emplace_back(p2, m_blanks[{p2, is_vert}]);
    }

    vector<int> perm(7);
    boost::iota(perm, 1);
    do
        m_perms.push_back(perm);
    while (boost::next_permutation(perm));

    for (int i = 0; i < rows() + cols(); ++i) {
        auto& rng = m_rowscols[i];
        if (rng.size() != 7) continue;
        auto& perm_ids = m_rc2permids[i];
        perm_ids.resize(m_perms.size());
        boost::iota(perm_ids, 0);
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const puz_key& key, int j);
    void make_move2(const puz_key& key, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<Position, int> m_cells;
    map<Position, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
//: m_cells(g.m_blanks), m_game(&g)
{
    //for (auto& kv : g.m_pos2area) {
    //    auto& perm_ids = m_matches[kv.first];
    //    auto& area = kv.second;
    //    perm_ids.resize(g.m_sum2perms.at({area.m_sum, area.m_range.size()}).size());
    //    boost::iota(perm_ids, 0);
    //}

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    //for (auto& kv : m_matches) {
    //    const auto& key = kv.first;
    //    auto& perm_ids = kv.second;

    //    vector<int> nums;
    //    auto& area = m_game->m_pos2area.at(key);
    //    for (auto& p : area.m_range)
    //        nums.push_back(m_cells.at(p));

    //    auto& perms = m_game->m_sum2perms.at({area.m_sum, area.m_range.size()});
    //    boost::remove_erase_if(perm_ids, [&](int id) {
    //        return !boost::equal(nums, perms[id], [](int n1, int n2) {
    //            return n1 == 0 || n1 == n2;
    //        });
    //    });

    //    if (!init)
    //        switch(perm_ids.size()) {
    //        case 0:
    //            return 0;
    //        //case 1:
    //        //    return make_move2(key, perm_ids.front()), 1;
    //        }
    //}
    return 2;
}

void puz_state::make_move2(const puz_key& key, int j)
{
    //auto& area = m_game->m_pos2area.at(key);
    //auto& perm = m_game->m_sum2perms.at({area.m_sum, area.m_range.size()})[j];

    //for (int k = 0; k < perm.size(); ++k)
    //    m_cells.at(area.m_range[k]) = perm[k];

    //++m_distance;
    //m_matches.erase(key);
}

bool puz_state::make_move(const puz_key& key, int j)
{
    m_distance = 0;
    make_move2(key, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    //auto& kv = *boost::min_element(m_matches, [](
    //    const pair<const puz_key, vector<int>>& kv1,
    //    const pair<const puz_key, vector<int>>& kv2) {
    //    return kv1.second.size() < kv2.second.size();
    //});
    //for (int n : kv.second) {
    //    children.push_back(*this);
    //    if (!children.back().make_move(kv.first, n))
    //        children.pop_back();
    //}
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0; r < sidelen(); ++r) {
    //    for (int c = 0; c < sidelen(); ++c) {
    //        Position p(r, c);
    //        out << (m_game->m_blanks.count(p) != 0 ? char(m_cells.at(p) + '0') : '.') << ' ';
    //    }
    //    out << endl;
    //}
    return out;
}

}

void solve_puz_Sukrokuro()
{
    using namespace puzzles::Sukrokuro;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Sukrokuro.xml", "Puzzles/Sukrokuro.txt", solution_format::GOAL_STATE_ONLY);
}
