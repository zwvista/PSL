#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 4/Kakuro

    Summary
    Fill the board with numbers 1 to 9 according to the sums

    Description
    1. Your goal is to write a number in every blank tile (without a diagonal
       line).
    2. The number on the top of a column or at the left of a row, gives you
       the sum of the numbers in that column or row.
    3. You can write numbers 1 to 9 in the tiles, however no same number should
       appear in a consecutive row or column.
    4. Tiles which only have a diagonal line aren't used in the game.
*/

namespace puzzles::Kakuro{

constexpr auto PUZ_SPACE = ' ';

struct puz_area
{
    vector<Position> m_range;
    int m_sum;
};

using puz_key = pair<Position, bool>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<puz_key, puz_area> m_pos2area;
    map<Position, int> m_blanks;
    map<pair<int, int>, vector<vector<int>>> m_sum2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen * 4; c += 4) {
            Position p(r, c / 4);
            auto s1 = str.substr(c, 2);
            auto s2 = str.substr(c + 2, 2);
            if (s1 == "  ")
                m_blanks[p] = 0;
            else {
                int n1 = stoi(string(s1));
                if (n1 != 0)
                    m_pos2area[{p, true}].m_sum = n1;
                int n2 = stoi(string(s2));
                if (n2 != 0)
                    m_pos2area[{p, false}].m_sum = n2;
            }
        }
    }

    for (auto& [key, area] : m_pos2area) {
        const auto& p = key.first;
        auto os = key.second ? Position(1, 0) : Position(0, 1);
        auto& ps = area.m_range;
        int cnt = 0;
        for (auto p2 = p + os; m_blanks.contains(p2); p2 += os)
            ++cnt, ps.push_back(p2);
        m_sum2perms[{area.m_sum, cnt}];
    }

    for (auto& [kv, perms] : m_sum2perms) {
        auto& [sum, cnt] = kv;

        vector<int> perm(cnt, 1);
        for (int i = 0; i < cnt;) {
            if (boost::accumulate(perm, 0) == sum &&
                set<int>(perm.begin(), perm.end()).size() == perm.size())
                perms.push_back(perm);
            for (i = 0; i < cnt && ++perm[i] == 10; ++i)
                perm[i] = 1;
        }
    };
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
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

    const puz_game* m_game;
    map<Position, int> m_cells;
    map<puz_key, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_blanks), m_game(&g)
{
    for (auto& [p, area] : g.m_pos2area) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_sum2perms.at({area.m_sum, area.m_range.size()}).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [key, perm_ids] : m_matches) {
        vector<int> nums;
        auto& area = m_game->m_pos2area.at(key);
        for (auto& p : area.m_range)
            nums.push_back(m_cells.at(p));

        auto& perms = m_game->m_sum2perms.at({area.m_sum, area.m_range.size()});
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(nums, perms[id], [](int n1, int n2) {
                return n1 == 0 || n1 == n2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(key, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const puz_key& key, int j)
{
    auto& area = m_game->m_pos2area.at(key);
    auto& perm = m_game->m_sum2perms.at({area.m_sum, area.m_range.size()})[j];

    for (int k = 0; k < perm.size(); ++k)
        m_cells.at(area.m_range[k]) = perm[k];

    ++m_distance;
    m_matches.erase(key);
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
    auto& [key, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const puz_key, vector<int>>& kv1,
        const pair<const puz_key, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(key, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << (m_game->m_blanks.contains(p) ? char(m_cells.at(p) + '0') : '.') << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Kakuro()
{
    using namespace puzzles::Kakuro;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Kakuro.xml", "Puzzles/Kakuro.txt", solution_format::GOAL_STATE_ONLY);
}
