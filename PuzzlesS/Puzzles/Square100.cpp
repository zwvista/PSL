#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 10/Square100

    Summary
    Exactly one hundred

    Description
    1. You are given a 3*3 or 4*4 square with digits in it.
    2. You have to add digits to some (or all) tiles, in order to produce
       the sum of 100 for every row and column.
    3. You can add digits before or after the given one.
*/

namespace puzzles::Square100{

constexpr auto PUZ_SPACE = 0;

// a row or a column
struct puz_area
{
    // all positions
    vector<Position> m_range;
    // all given digits
    vector<int> m_nums;
    // all permutations
    vector<vector<int>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_area> m_areas;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_areas(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            int n = str[c] - '0';
            for (int i : {r, m_sidelen + c}) {
                auto& a = m_areas[i];
                a.m_range.push_back(p);
                a.m_nums.push_back(n);
            }
        }
    }

    for (auto& area : m_areas) {
        const auto& nums = area.m_nums;
        auto& perms = area.m_perms;
        int cnt = nums.size();

        // indexes: 0--19  0--19  0--19
        // An index smaller than 10 means a digit(=index) should be added
        // before the given one.
        // An index greater than or equal to 10 means a digit(=index-10)
        // should be added after the given one.
        vector<int> indexes(cnt);
        vector<int> perm(cnt);
        for (int i = 0; i < cnt;) {
            for (int j = 0; j < cnt; ++j) {
                int index = indexes[j];
                perm[j] = index < 10 ? index * 10 + nums[j]
                    : nums[j] * 10 + index - 10;
            }
            if (boost::accumulate(perm, 0) == 100)
                perms.push_back(perm);
            for (i = 0; i < cnt && ++indexes[i] == 20; ++i)
                indexes[i] = 0;
        }
    }
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

    const puz_game* m_game;
    vector<int> m_cells;
    map<int, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_sidelen * g.m_sidelen), m_game(&g)
{
    auto f = [&](int area_id) {
        auto& perm_ids = m_matches[area_id];
        perm_ids.resize(g.m_areas.at(area_id).m_perms.size());
        boost::iota(perm_ids, 0);
    };

    for (int i = 0; i < sidelen(); ++i)
        f(i), f(sidelen() + i);

    //find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& area = m_game->m_areas.at(area_id);
        vector<int> nums;
        for (auto& p : area.m_range)
            nums.push_back(cells(p));

        auto& perms = area.m_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](int n1, int n2) {
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
    auto& area = m_game->m_areas[i];
    auto& range = area.m_range;
    auto& perm = area.m_perms[j];

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
        for (int c = 0; c < sidelen(); ++c) {
            out << format("{:3}", cells({r, c}));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Square100()
{
    using namespace puzzles::Square100;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Square100.xml", "Puzzles/Square100.txt", solution_format::GOAL_STATE_ONLY);
}
