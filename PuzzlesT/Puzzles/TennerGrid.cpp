#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/TennerGrid

    Summary
    Counting up to 10

    Description
    1. You goal is to enter every digit, from 0 to 9, in each row of the Grid.
    2. The number on the bottom row gives you the sum for that column.
    3. Digit can repeat on the same column, however digits in contiguous tiles
       must be different, even diagonally. Obviously digits can't repeat on
       the same row.
*/

namespace puzzles::TennerGrid{

constexpr auto PUZ_UNKNOWN = -1;
    
constexpr Position offset[] = {
    {-1, 0},    // n
    {-1, 1},    // ne
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},    // sw
    {-1, -1},    // nw
};

struct puz_area_info
{
    vector<Position> m_range;
    vector<int> m_nums;
    vector<vector<int>> m_perms;

    puz_area_info() {
        m_nums = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    }
};

struct puz_game
{
    string m_id;
    Position m_size;
    vector<int> m_cells;
    vector<puz_area_info> m_area_info;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
    int cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_size(strs.size() - 1, strs[0].size() / 2)
, m_area_info(rows() + cols())
{
    for (int r = 0; r <= rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto s = str.substr(c * 2, 2);
            int n = s == "  " ? PUZ_UNKNOWN : stoi(string(s));
            m_cells.push_back(n);

            if (n == PUZ_UNKNOWN)
                m_area_info[c].m_range.push_back(p);

            if (r == rows()) continue;
            auto& info_r = m_area_info[r + cols()];
            if (n == PUZ_UNKNOWN)
                info_r.m_range.push_back(p);
            else
                boost::remove_erase(info_r.m_nums, n);
        }
    }

    for (int c = 0; c < cols(); ++c) {
        auto& info = m_area_info[c];
        auto& perms = info.m_perms;
        int cnt = info.m_range.size();

        vector<int> nums(rows());
        vector<int> perm(cnt);
        for (int i = 0; i < cnt;) {
            for (int r = 0, j = 0; r < rows(); ++r) {
                int n = cells({r, c});
                nums[r] = n == PUZ_UNKNOWN ? perm[j++] : n;
            }
            if (boost::range::adjacent_find(nums) == nums.end() &&
                boost::accumulate(nums, 0) == cells({rows(), c}))
                perms.push_back(perm);
            for (i = 0; i < cnt && ++perm[i] == cols(); ++i)
                perm[i] = 0;
        }
    }

    for (int r = 0; r < rows(); ++r) {
        auto& info = m_area_info[r + cols()];
        auto& perm = info.m_nums;
        do
            info.m_perms.push_back(perm);
        while (boost::next_permutation(perm));
    }
}

struct puz_state : vector<int>
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    int cells(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * cols() + p.second]; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_area_info.size(); ++i) {
        auto& info = g.m_area_info[i];
        if (info.m_perms.empty()) continue;
        auto& perm_ids = m_matches[i];
        perm_ids.resize(info.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& info = m_game->m_area_info[area_id];
        vector<int> nums;
        for (auto& p : info.m_range)
            nums.push_back(cells(p));

        auto& perms = info.m_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](int n1, int n2) {
                return n1 == PUZ_UNKNOWN || n1 == n2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& info = m_game->m_area_info[i];
    auto& range = info.m_range;
    auto& perm = info.m_perms[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = range[k];
        int n = cells(p) = perm[k];

        // no touching
        for (auto& os : offset) {
            auto p2 = p + os;
            if (is_valid(p2) && cells(p2) == n)
                return false;
        }
    }

    ++m_distance;
    m_matches.erase(i);
    return true;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if (!make_move2(i, j))
        return false;
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
    for (int r = 0; r <= rows(); ++r) {
        for (int c = 0; c < cols(); ++c)
            out << format("{:3}", cells({r, c}));
        println(out);
    }
    return out;
}

}

void solve_puz_TennerGrid()
{
    using namespace puzzles::TennerGrid;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TennerGrid.xml", "Puzzles/TennerGrid.txt", solution_format::GOAL_STATE_ONLY);
}
