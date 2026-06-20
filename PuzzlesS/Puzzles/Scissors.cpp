#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 1/Scissors

    Summary
    Tailor's puzzle

    Description
    1. Cut the board into patches.
    2. Each patch should contain the numbers 1 to N exactly once (N being the highest number on the board).
    3. Each patch should end on the border.
*/

namespace puzzles::Scissors{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BACK_SLASH = '\\';
constexpr auto PUZ_FRONT_SLASH = '/';
    
constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::Square2x2Offset;
constexpr array<Position, 4> offset3 = {
    Position{-1, -1},
    Position{-1, 1},
    Position{1, -1},
    Position{1, 1},
};

using puz_slash = pair<Position, Position>;

// first: position of the cell/triangle
// second: 15 if the cell contains no slash
// second: 3 or 12 if the cell contains a back slash
// second: 6 or 9 if the cell contains a front slash
using puz_position = pair<Position, int>;

struct puz_move
{
    vector<puz_slash> m_cut;
    vector<pair<Position, char>> m_slash_chars;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    char m_max_num = '1';
    set<puz_slash> m_slashes;
    vector<puz_move> m_moves;
    vector<puz_position> m_positions;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : puz_position
{
    puz_state2(const vector<puz_position>* positions)
        : m_positions(positions) { make_move(positions->front()); }

    void make_move(const puz_position& kv) { static_cast<puz_position&>(*this) = kv; }
    void gen_children(list<puz_state2>& children) const;

    const vector<puz_position>* m_positions;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i)
        if (second & (1 << i)) {
            auto p2 = first + offset[i];
            int j = (i * 2) % 4;
            for (auto& kv : *m_positions)
                if (auto& [p3, n] = kv; p3 == p2 && n & (1 << j)) {
                    children.emplace_back(*this).make_move(kv);
                    break;
                }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            m_cells.push_back(ch);
            m_max_num = max(m_max_num, ch);
            m_positions.emplace_back(p, 15);
            if (ch == PUZ_SPACE) {
                // back slash
                m_slashes.emplace(p + offset2[0], p + offset2[3]);
                // front slash
                m_slashes.emplace(p + offset2[1], p + offset2[2]);
            }
        }
    }

    auto is_border = [&](const Position& p) {
        return p.first == 0 || p.second == 0 || p.first == m_sidelen || p.second == m_sidelen;
    };
    for (int i = 0; i <= m_sidelen; ++i) {
        auto f = [&](int r, int c) {
            Position p0(r, c);
            vector<puz_slash> cut;
            vector<pair<Position, char>> slash_chars;
            auto is_valid_cut = [&]{
                auto positions = m_positions;
                for (auto& [p, ch] : slash_chars) {
                    boost::remove_erase(positions, puz_position(p, 15));
                    if (ch == PUZ_BACK_SLASH)
                        positions.emplace_back(p, 3), positions.emplace_back(p, 12);
                    else
                        positions.emplace_back(p, 6), positions.emplace_back(p, 9);
                }
                auto smoves = puz_move_generator<puz_state2>::gen_moves(&positions);
                vector<char> nums1, nums2;
                for (auto& [p, n] : positions) {
                    if (n != 15) continue;
                    char ch = cells(p);
                    if (ch == PUZ_SPACE) continue;
                    (boost::algorithm::any_of(smoves, [&](const puz_state2& s) {
                        return p == s.first;
                    }) ? nums1 : nums2).push_back(ch);
                }
                auto g = [&](vector<char>& nums) {
                    int sz = nums.size();
                    if (sz != (m_max_num - '0'))
                        return false;
                    boost::sort(nums);
                    for (int i = 0; i < sz; ++i)
                        if (nums[i] != (i + '0'))
                            return false;
                    return true;
                };
                return g(nums1) || g(nums2);
            };
            auto dfs = [&](this const auto& self, const Position& p1) {
                if (p1 != p0 && is_border(p1)) {
                    if (p1 > p0 && is_valid_cut())
                        m_moves.emplace_back(cut, slash_chars);
                    return;
                }
                for (auto& os : offset3) {
                    auto p2 = p1 + os;
                    auto p3 = min(p1, p2), p4 = max(p1, p2);
                    if (!m_slashes.contains({p3, p4})) continue;
                    cut.emplace_back(p1, p2);
                    auto& [r1, c1] = p3;
                    auto& [r2, c2] = p4;
                    int r0 = min(r1, r2), c0 = min(c1, c2);
                    slash_chars.emplace_back(Position(r0, c0), r0 == r1 && c0 == c1 ? PUZ_BACK_SLASH : PUZ_FRONT_SLASH);
                    self(p2);
                    cut.pop_back();
                    slash_chars.pop_back();
                }
            };
        };
        f(0, i), f(0, m_sidelen), f(i, 0), f(i, m_sidelen);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
}


bool puz_state::make_move(const Position& p, int n)
{
    return true;
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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << format("{:<2}", cells(p));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Scissors()
{
    using namespace puzzles::Scissors;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Scissors.xml", "Puzzles/Scissors.txt", solution_format::GOAL_STATE_ONLY);
}
