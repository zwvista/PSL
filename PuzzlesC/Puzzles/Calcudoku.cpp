#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 4/Calcudoku

    Summary
    Mathematical Sudoku

    Description
    1. Write numbers ranging from 1 to board size respecting the calculation
       hint.
    2. The tiny numbers and math signs in the corner of an area give you the
       hint about what's happening inside that area.
    3. For example a '3+' means that the sum of the numbers inside that area
       equals 3. In that case you would have to write the numbers 1 and 2
       there.
    4. Another example: '12*' means that the multiplication of the numbers
       in that area gives 12, so it could be 3 and 4 or even 3, 4 and 1,
       depending on the area size.
    5. Even where the order of the operands matter (in subtraction and division)
       they can appear in any order inside the area (ie.e. 2/ could be done
       with 4 and 2 or 2 and 4.
    6. All the numbers appear just one time in each row and column, but they
       could be repeated in non-straight areas, like the L-shaped one.
*/

namespace puzzles::Calcudoku{

constexpr auto PUZ_SPACE = 0;
constexpr auto PUZ_ADD = '+';
constexpr auto PUZ_SUB = '-';
constexpr auto PUZ_MUL = '*';
constexpr auto PUZ_DIV = '/';

struct puz_area_info
{
    vector<Position> m_range;
    char m_operator;
    int m_result;
    // Dimension 1: all possible combinations
    // Dimension 2: numbers to be placed in this area
    vector<vector<int>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_area_info> m_area_info;
    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            auto s = str.substr(c * 4, 4);
            int n = s[0] - 'a';

            if (n >= m_area_info.size())
                m_area_info.resize(n + 1);
            auto& info = m_area_info[n];
            info.m_range.push_back(p);
            if (s.substr(1) == "   ") continue;
            info.m_operator = s[3];
            info.m_result = stoi(s.substr(1, 2));
        }
    }

    auto f = [](const vector<Position>& rng) {
        vector<vector<int>> groups;
        map<int, vector<int>> grp_rows, grp_cols;
        for (int i = 0; i < rng.size(); ++i) {
            auto& p = rng[i];
            grp_rows[p.first].push_back(i);
            grp_cols[p.second].push_back(i);
        }
        for (auto& m : {grp_rows, grp_cols})
            for (auto& kv : m)
                if (kv.second.size() > 1)
                    groups.push_back(kv.second);
        return groups;
    };

    for (auto& info : m_area_info) {
        int cnt = info.m_range.size();

        vector<vector<int>> perms;
        vector<int> perm(cnt, 1);
        for (int i = 0; i < cnt;) {
            auto nums = perm;
            boost::sort(nums, greater<>());
            if (accumulate(next(nums.begin()), nums.end(),
                nums.front(), [&](int acc, int v) {
                switch(info.m_operator) {
                case PUZ_ADD:
                    return acc + v;
                case PUZ_SUB:
                    return acc - v;
                case PUZ_MUL:
                    return acc * v;
                case PUZ_DIV:
                    return acc % v == 0 ? acc / v : 0;
                default:
                    return 0;
                }
            }) == info.m_result)
                perms.push_back(perm);
            for (i = 0; i < cnt && ++perm[i] == m_sidelen + 1; ++i)
                perm[i] = 1;
        }

        auto groups = f(info.m_range);
        for (const auto& perm : perms)
            if (boost::algorithm::all_of(groups, [&](const vector<int>& grp) {
                set<int> nums;
                for (int i : grp)
                    nums.insert(perm[i]);
                return nums.size() == grp.size();
            }))
                info.m_perms.push_back(perm);
    }
}

struct puz_state : vector<int>
{
    puz_state() {}
    puz_state(const puz_game& g);

    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i, int j);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, vector<int>> m_matches;
};

puz_state::puz_state(const puz_game& g)
    : vector<int>(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
{
    for (int i = 0; i < g.m_area_info.size(); ++i) {
        auto& info = g.m_area_info[i];
        auto& perm_ids = m_matches[i];
        perm_ids.resize(info.m_perms.size());
        boost::iota(perm_ids, 0);
    }
}

bool puz_state::make_move(int i, int j)
{
    auto& info = m_game->m_area_info[i];
    auto area = info.m_range;
    auto perm = info.m_perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area[k]) = perm[k];

    m_matches.erase(i);

    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;
        auto& info2 = m_game->m_area_info[area_id];
        auto area2 = info2.m_range;

        for (int k = 0; k < area.size(); ++k) {
            auto& p = area[k];
            for (int k2 = 0; k2 < area2.size(); ++k2) {
                auto& p2 = area2[k2];
                if (p.first == p2.first || p.second == p2.second)
                    boost::remove_erase_if(perm_ids, [&](int id) {
                        auto& perm2 = info2.m_perms[id];
                        return perm[k] == perm2[k2];
                    });
            }
        }

        if (perm_ids.empty())
            return false;
    }
    return true;
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
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_Calcudoku()
{
    using namespace puzzles::Calcudoku;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Calcudoku.xml", "Puzzles/Calcudoku.txt", solution_format::GOAL_STATE_ONLY);
}
