#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Finger Pointing

    Summary
    Blame is in the air

    Description
    1. Fill the board with fingers. Two fingers pointing in the same direction
       can't be orthogonally adjacent.
    2. the number tells you how many fingers and finger 'trails' point to that tile.
*/

namespace puzzles::FingerPointing{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FIXED = 'F';
constexpr auto PUZ_ADDED = 'f';
constexpr auto PUZ_BLOCK = 'B';

bool is_flower(char ch) { return ch == PUZ_FIXED || ch == PUZ_ADDED; }

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position invalid_pos = {-1, -1};

using puz_move = vector<vector<Position>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    map<Position, vector<puz_move>> m_pos2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p, int i, int n)
        : m_game(&game), m_last_dir(i), m_num(n) { make_move(i, p); }

    bool is_goal_state() const { return m_distance == m_num; }
    void make_move(int i, Position p2) {
        push_back(p2), m_last_dir = i, ++m_distance;
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_last_dir;
    int m_num;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& p = back();
    for (int i = 0; i < 4; ++i) {
        if (m_last_dir == (i + 2) % 4 || m_last_dir == i) continue;
        if (auto p2 = p + offset[i];
            boost::algorithm::none_of_equal(*this, p2))
            if (m_game->cells(p2) == PUZ_SPACE) {
                children.push_back(*this);
                children.back().make_move(i, p2);
            }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BLOCK);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            m_cells.push_back(ch);
            if (isdigit(ch))
                m_pos2num[{r, c}] = ch - '0';
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

    for (auto& [p, sum] : m_pos2num) {
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i)
            if (cells(p + offset[i]) != PUZ_SPACE)
                dir_nums[i] = {0};
            else {
                dir_nums[i].resize(sum + 1);
                boost::iota(dir_nums[i], 0);
            }
        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum) {
                            vector v = {n0, n1, n2, n3};
                            vector<puz_move> moves(4);
                            for (int i = 0; i < 4; ++i) {
                                int n = v[i];
                                if (n == 0)
                                    moves[i].push_back({invalid_pos});
                                else {
                                    puz_state2 sstart(*this, p + offset[i], i, n);
                                    list<list<puz_state2>> spaths;
                                    if (auto [found, _1] = puz_solver_bfs<puz_state2, true, false, false>::find_solution(sstart, spaths); found)
                                        for (auto& spath : spaths)
                                            moves[i].push_back(spath.back());
                                }
                            }
                            for (auto& m0 : moves[0])
                                for (auto& m1 : moves[1])
                                    for (auto& m2 : moves[2])
                                        for (auto& m3 : moves[3]) {
                                            puz_move move = {m0, m1, m2, m3};
                                            vector<Position> v2;
                                            for (auto& m : move)
                                                v2.append_range(m);
                                            boost::remove_erase(v2, invalid_pos);
                                            set<Position> s(v2.begin(), v2.end());
                                            if (s.size() == v2.size()) {
                                                for (auto& m : move)
                                                    boost::remove_erase(m, invalid_pos);
                                                m_pos2moves[p].push_back(move);
                                            }
                                        }
                        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(Position p);
    void check_field();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_num2outer.size() - 1; }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    // key: the index of the flowerbed
    // value: the neighboring tiles of the flowerbed
    map<int, set<Position>> m_num2outer;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_cells), m_game(&g)
{
    set<Position> rng;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (is_flower(cells(p)))
                rng.insert(p);
        }

    check_field();
}

void puz_state::check_field()
{
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SPACE)
                continue;

            vector<int> counts;
            for (auto& os : offset) {
                int n = 0;
                for (auto p2 = p + os; is_flower(cells(p2)); p2 += os)
                    ++n;
                counts.push_back(n);
            }
            if (counts[0] + counts[2] < 3 && counts[1] + counts[3] < 3)
                continue;
            // if a flower is put in this tile
            // more than 3 flowers will line up horizontally and/or vertically
            // so this tile must be empty.
            cells(p) = PUZ_EMPTY;
            for (auto& [n, outer] : m_num2outer)
                outer.erase(p);
        }
}

bool puz_state::make_move(Position p)
{
    vector<int> nums;
    for (auto& [n, outer] : m_num2outer)
        if (outer.contains(p))
            nums.push_back(n);

    m_distance = 0;
    // merge all the flowerbeds adjacent to p
    while (nums.size() > 1) {
        int n2 = nums.back(), n1 = nums.rbegin()[1];
        auto &outer2 = m_num2outer.at(n2), &outer1 = m_num2outer.at(n1);
        outer1.insert(outer2.begin(), outer2.end());
        m_num2outer.erase(n2);
        ++m_distance;
        nums.pop_back();
    }

    cells(p) = PUZ_ADDED;
    auto& outer = m_num2outer.at(nums[0]);
    outer.erase(p);
    for (auto& os : offset) {
        auto p2 = p + os;
        if (cells(p2) == PUZ_SPACE)
            outer.insert(p2);
    }

    return is_goal_state() || (
        check_field(),
        boost::algorithm::all_of(m_num2outer,
            [](const pair<const int, set<Position>>& kv) {
            return kv.second.size() > 0;
        })
    );
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [n, outer] = *boost::min_element(m_num2outer, [](
        const pair<const int, set<Position>>& kv1,
        const pair<const int, set<Position>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& p : outer) {
        children.push_back(*this);
        if (!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            char ch = cells({r, c});
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FingerPointing()
{
    using namespace puzzles::FingerPointing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FingerPointing.xml", "Puzzles/FingerPointing.txt", solution_format::GOAL_STATE_ONLY);
}
