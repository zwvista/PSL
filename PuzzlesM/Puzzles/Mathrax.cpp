#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 6/Mathrax

    Summary
    Diagonal Math Wiz

    Description
    1. The goal is to input numbers 1 to N, where N is the board size, following
       the hints in the intersections.
    2. A number must appear once for every row and column.
    3. The tiny numbers and sign in the intersections tell you the result of
       the operation between the two opposite diagonal tiles. This is valid
       for both pairs of numbers surrounding the hint.
    4. In some puzzles, there will be 'E' or 'O' as hint. This means that all
       four tiles are either (E)ven or (O)dd numbers.
*/

namespace puzzles::Mathrax{

constexpr auto PUZ_SPACE = 0;
constexpr auto PUZ_WRONG = -1;
constexpr auto PUZ_ADD = '+';
constexpr auto PUZ_SUB = '-';
constexpr auto PUZ_MUL = '*';
constexpr auto PUZ_DIV = '/';
constexpr auto PUZ_EVEN = 'E';
constexpr auto PUZ_ODD = 'O';

constexpr Position offset[] = {
    {0, 0},    // nw
    {0, 1},    // ne
    {1, 1},    // se
    {1, 0},    // sw
};

struct puz_area_diag_info
{
    vector<Position> m_range;
    char m_operator;
    int m_result;
    vector<vector<int>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area_rc2range;
    vector<vector<int>> m_perms_rc;
    vector<puz_area_diag_info> m_area_diag_info;
    int m_area_count;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs[0].size())
, m_area_rc2range(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_cells.push_back(ch == ' ' ? PUZ_SPACE : ch - '0');
            Position p(r, c);
            m_area_rc2range[r].push_back(p);
            m_area_rc2range[m_sidelen + c].push_back(p);
        }
    }

    vector<int> perm(m_sidelen);
    boost::iota(perm, 1);
    do
        m_perms_rc.push_back(perm);
    while (boost::next_permutation(perm));

    for (int r = 0; r < m_sidelen - 1; ++r) {
        string_view str = strs[r + m_sidelen];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            auto s = str.substr(c * 3, 3);
            if (s == "   ") continue;
            Position p(r, c);
            m_area_diag_info.emplace_back();
            auto& info = m_area_diag_info.back();
            for (auto& os : offset)
                info.m_range.push_back(p + os);
            info.m_operator = s[2];
            info.m_result = stoi(string(s.substr(0, 2)));
        }
    }
    m_area_count = m_sidelen * 2 + m_area_diag_info.size();
    
    auto f = [](char ch_op, int n1, int n2) {
        if (n1 < n2)
            swap(n1, n2);
        switch(ch_op) {
        case PUZ_ADD:
            return n1 + n2;
        case PUZ_SUB:
            return n1 - n2;
        case PUZ_MUL:
            return n1 * n2;
        case PUZ_DIV:
            return n1 % n2 == 0 ? n1 / n2 : PUZ_WRONG;
        case PUZ_EVEN:
            return n1 % 2 == 0 && n2 % 2 == 0 ? PUZ_SPACE : PUZ_WRONG;
        case PUZ_ODD:
            return n1 % 2 == 1 && n2 % 2 == 1 ? PUZ_SPACE : PUZ_WRONG;
        default:
            return PUZ_WRONG;
        }
    };

    for (auto& info : m_area_diag_info)
        // n0 n1
        // n3 n2
        for (int n0 = 1; n0 <= m_sidelen; ++n0)
            for (int n1 = 1; n1 <= m_sidelen; ++n1) {
                if (n0 == n1) continue;
                for (int n2 = 1; n2 <= m_sidelen; ++n2) {
                    if (n1 == n2) continue;
                    for (int n3 = 1; n3 <= m_sidelen; ++n3)
                    if (n0 != n3 && n2 != n3 &&
                        f(info.m_operator, n0, n2) == info.m_result &&
                        f(info.m_operator, n1, n3) == info.m_result)
                        info.m_perms.push_back({n0, n1, n2, n3});
                }
            }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
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
: m_cells(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_area_count; ++i) {
        auto& perm_ids = m_matches[i];
        perm_ids.resize(i < sidelen() * 2 ? g.m_perms_rc.size() :
            g.m_area_diag_info[i - sidelen() * 2].m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [index, perm_ids] : m_matches) {
        auto f = [&](const vector<Position>& rng, const vector<vector<int>>& perms) {
            vector<int> nums;
            for (auto& p : rng)
                nums.push_back(cells(p));

            boost::remove_erase_if(perm_ids, [&](int id) {
                return !boost::equal(nums, perms[id], [](int n1, int n2) {
                    return n1 == PUZ_SPACE || n1 == n2;
                });
            });
        };

        if (index < sidelen() * 2)
            f(m_game->m_area_rc2range[index], m_game->m_perms_rc);
        else {
            auto& info = m_game->m_area_diag_info[index - sidelen() * 2];
            f(info.m_range, info.m_perms);
        }

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(index, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto f = [&](const vector<Position>& rng, const vector<int>& perm) {
        for (int k = 0; k < perm.size(); ++k)
            cells(rng[k]) = perm[k];
    };

    if (i < sidelen() * 2)
        f(m_game->m_area_rc2range[i], m_game->m_perms_rc[j]);
    else {
        auto& info = m_game->m_area_diag_info[i - sidelen() * 2];
        f(info.m_range, info.m_perms[j]);
    }
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
    auto& [index, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(index, n))
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

void solve_puz_Mathrax()
{
    using namespace puzzles::Mathrax;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Mathrax.xml", "Puzzles/Mathrax.txt", solution_format::GOAL_STATE_ONLY);
}
