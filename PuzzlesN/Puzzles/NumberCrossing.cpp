#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 17/Number Crossing

    Summary
    Digital Crosswords

    Description
    1. Find the numbers in the board.
    2. Numbers cannot touch each other, not even diagonally.
    3. On the top and left of the grid, you're given how many numbers are in that
       column or row.
    4. On the bottom and right of the grid, you're given the sum of the numbers
       on that column or row.
*/

namespace puzzles::NumberCrossing{

#define PUZ_EMPTY        0
#define PUZ_UNKNOWN      -1

constexpr Position offset[] = {
    {-1, 0},    // n
    {-1, 1},    // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},    // sw
    {0, -1},    // w
    {-1, -1},    // nw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    // all permutations
    map<int, pair<int, int>> m_area2info;
    map<pair<int, int>, vector<vector<int>>> m_info2perms;
    const vector<vector<int>>& area2perms(int area_id) const {
        return m_info2perms.at(m_area2info.at(area_id));
    }
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

    for (int i = 1; i < m_sidelen - 1; ++i)
        for (int j = 0; j < 2; ++j) {
            int area_id = i + j * m_sidelen;
            auto& area = m_area2range.at(area_id);
            // 3. On the top and left of the grid, you're given how many numbers are in that
            // column or row.
            // 4. On the bottom and right of the grid, you're given the sum of the numbers
            // on that column or row.
            int count = cells(area.front()), sum = cells(area.back());
            if (count == 0 || sum == 0)
                count = sum = 0;
            pair kv(count, sum);
            m_area2info[area_id] = kv;
            m_info2perms[kv];
        }
    for (auto&& [info, perms] : m_info2perms) {
        auto [count, sum] = info;
        if (sum == 0 || sum == PUZ_UNKNOWN)
            perms.push_back(vector<int>(m_sidelen, PUZ_EMPTY));
        if (sum != 0)
            for (int k = 1; k <= (m_sidelen - 1) / 2; ++k) {
                if (count != PUZ_UNKNOWN && count != k) continue;
                vector<int> digits(k, 1);
                set<vector<int>> digits_all;
                for (int i = 0; i < k;) {
                    int sum2 = boost::accumulate(digits, 0);
                    if (sum == PUZ_UNKNOWN || sum == sum2) {
                        auto digits2 = digits;
                        boost::sort(digits2);
                        if (!digits_all.contains(digits2)) {
                            digits_all.insert(digits2);
                            vector<int> perm(m_sidelen - 2 - k, PUZ_EMPTY);
                            perm.insert(perm.end(), digits2.begin(), digits2.end());
                            perm.insert(perm.begin(), k);
                            perm.insert(perm.end(), sum2);
                            auto begin = next(perm.begin()), end = prev(perm.end());
                            do {
                                if ([&]{
                                    // 2. Numbers cannot touch each other, not even diagonally.
                                    for (int m = 2; m < m_sidelen - 2; ++m)
                                        if (perm[m] != PUZ_EMPTY && (perm[m - 1] != PUZ_EMPTY || perm[m + 1] != PUZ_EMPTY))
                                            return false;
                                    return true;
                                }())
                                    perms.push_back(perm);
                            } while(next_permutation(begin, end));
                        }
                    }
                    for (i = 0; i < k && ++digits[i] == 10; ++i)
                        digits[i] = 1;
                }
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
    bool is_valid(const Position& p) const {
        return p.first > 0 && p.first < sidelen() - 1 && p.second > 0 && p.second < sidelen() - 1;
    }
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
: m_game(&g), m_cells(g.m_start)
{
    for (int i = 1; i < sidelen() - 1; ++i)
        for (int j = 0; j < 2; ++j) {
            int area_id = i + j * sidelen();
            vector<int> perm_ids(g.area2perms(area_id).size());
            boost::iota(perm_ids, 0);
            m_matches[area_id] = perm_ids;
        }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;
        auto& perms = m_game->area2perms(area_id);

        vector<int> digits;
        for (auto& p : m_game->m_area2range[kv.first])
            digits.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(digits, perms[id], [](int n1, int n2) {
                return n1 == PUZ_UNKNOWN || n1 == n2;
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
    auto& perm = m_game->area2perms(i)[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = range[k];
        if ((cells(p) = perm[k]) != PUZ_EMPTY && is_valid(p))
            // 2. Numbers cannot touch each other, not even diagonally.
            for (auto& os : offset) {
                auto p2 = p + os;
                if (is_valid(p2))
                    cells(p2) = PUZ_EMPTY;
            }
    }

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(true)) == 1);
    // The program will find another solution,
    // if the parameter is changed to false like the following,
    // while ((m = find_matches(false)) == 1);
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
            if (n == PUZ_UNKNOWN || n == PUZ_EMPTY && is_valid(p))
                out << "  ";
            else
                out << format("{:2}", n);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_NumberCrossing()
{
    using namespace puzzles::NumberCrossing;
    // when line 225 is commented out
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberCrossing.xml", "Puzzles/NumberCrossing.txt", solution_format::GOAL_STATE_ONLY);
    // when line 222 is commented out
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
    //    "Puzzles/NumberCrossing.xml", "Puzzles/NumberCrossing2.txt", solution_format::GOAL_STATE_ONLY);
}
