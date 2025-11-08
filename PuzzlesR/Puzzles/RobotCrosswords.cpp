#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Robot Crosswords

    Summary
    BZZZZliip 4 across?

    Description
    1. In a possible crossword for Robots, letters are substituted with digits.
    2. Each 'word' is formed by an uninterrupted sequence of numbers (i.e.
       2-3-4-5), but in any order (i.e. 3-4-2-5).
*/

namespace puzzles::RobotCrosswords{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SQUARE = '.';

struct puz_area
{
    vector<Position> m_range;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_area> m_areas;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());

    vector<Position> rng;
    string nums;
    auto g = [&]{
        if (!rng.empty()) {
            int sz = rng.size() + nums.size();
            int n1 = 1, n2 = 10 - sz;
            if (!nums.empty()) {
                boost::sort(nums);
                n1 = max(n1, nums.back() - '0' - sz + 1);
                n2 = min(n2, nums.front() - '0');
            }
            string nums_all(sz, PUZ_SPACE);
            string perm(rng.size(), PUZ_SPACE);
            puz_area area;
            area.m_range = rng;
            auto& perms = area.m_perms;
            for (int i = n1; i <= n2; ++i) {
                boost::iota(nums_all, i + '0');
                boost::set_difference(nums_all, nums, perm.begin());
                do
                    perms.push_back(perm);
                while (boost::next_permutation(perm));
            }
            m_areas.push_back(area);
        }
        rng.clear(), nums.clear();
    };
    auto f = [&](Position p, const Position& os) {
        for (int i = 0; i < m_sidelen; ++i, p += os) {
            char ch = cells(p);
            if (ch == PUZ_SQUARE)
                g();
            else if (ch == PUZ_SPACE)
                rng.push_back(p);
            else
                nums.push_back(ch);
        }
        g();
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f({i, 0}, {0, 1});        // e
        f({0, i}, {1, 0});        // s
    }
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
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        auto& perm_ids = m_matches[i];
        auto& area = g.m_areas[i];
        perm_ids.resize(area.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [n, perm_ids] : m_matches) {
        string nums;
        auto& area = m_game->m_areas[n];
        for (auto& p : area.m_range)
            nums.push_back(cells(p));

        auto& perms = area.m_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(n, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_areas[i];
    auto& perm = area.m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area.m_range[k]) = perm[k];

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
    auto& [i, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(i, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_RobotCrosswords()
{
    using namespace puzzles::RobotCrosswords;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/RobotCrosswords.xml", "Puzzles/RobotCrosswords.txt", solution_format::GOAL_STATE_ONLY);
}
