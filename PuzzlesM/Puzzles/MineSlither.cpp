#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/MineSlither

    Summary
    MineSweeper, meet corners

    Description
    1. A number tells you how many mines are around that tile.
*/

namespace puzzles::MineSlither{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_MINE = 'M';
constexpr auto PUZ_UNKNOWN = 5;

constexpr Position offset[] = {
    {0, 0},    // nw
    {0, 1},    // ne
    {1, 1},    // se
    {1, 0},    // sw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_nums;
    vector<vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    int nums(const Position& p) const { return m_nums[p.first * (m_sidelen - 1) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 1)
, m_num2perms(6)
{
    for (int r = 0; r < m_sidelen - 1; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str[c];
            m_nums.push_back(ch == PUZ_SPACE ? PUZ_UNKNOWN : ch - '0');
        }
    }

    auto& perms_all = m_num2perms[PUZ_UNKNOWN];
    for (int i = 0; i <= 4; ++i) {
        auto& perms = m_num2perms[i];
        auto perm = string(4 - i, PUZ_EMPTY) + string(i, PUZ_MINE);
        do {
            perms.push_back(perm);
            perms_all.push_back(perm);
        } while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c) {
            Position p(r, c);
            auto& perm_ids = m_matches[p];
            perm_ids.resize(g.m_num2perms.at(g.nums(p)).size());
            boost::iota(perm_ids, 0);
        }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_num2perms[m_game->nums(p)];
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(offset, perms[id], [&](const Position& os, char ch2) {
                char ch1 = cells(p + os);
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
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

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->nums(p)][n];

    for (int k = 0; k < perm.size(); ++k) {
        char& ch = cells(p + offset[k]);
        if (ch == PUZ_SPACE)
            ch = perm[k], ++m_distance;
    }

    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen() - 1; ++c)
            out << ' ' << m_game->nums({r, c});
        println(out);
    }
    return out;
}

}

void solve_puz_MineSlither()
{
    using namespace puzzles::MineSlither;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MineSlither.xml", "Puzzles/MineSlither.txt", solution_format::GOAL_STATE_ONLY);
}
