#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Proof of Quilt

    Summary
    Quilt the board, following the hints

    Description
     1. The goal is to place triangles in some cells in the end generating a pattern
        similar to a Quilt.
     2. The numbered tiles tell you how many triangles share an edge with it,
        horizontally and vertically
     3, For example, if a tile says 4, it has triangles all around it.
     4. If a tile says 1, it has only one triangle somewhere.
     5. Some tiles will remain blank and will form, along with the triangles, rectangles
        and squares.
     6. These can be tilted by 45 degrees.
     7. Some other tiles are filled but contain no number. These and the hints are
        the only tiles that can be completely filled.
     8. Rectangles or squares can't touch orthogonally, but can touch diagonally
*/

namespace puzzles::ProofOfQuilt{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_WHITE = 'W';
constexpr auto PUZ_BOUNDARY = 99;
constexpr auto PUZ_FILLED = 0;
constexpr auto PUZ_BLANK = 15;
constexpr auto PUZ_UPPER_LEFT = 6;
constexpr auto PUZ_UPPER_RIGHT = 12;
constexpr auto PUZ_LOWER_LEFT = 3;
constexpr auto PUZ_LOWER_RIGHT = 9;
constexpr auto PUZ_UNKNOWN = -1;
constexpr auto PUZ_NON_TRIANGLE = '0';
constexpr auto PUZ_TRIANGLE1 = '1';
constexpr auto PUZ_TRIANGLE2 = '2';

constexpr auto offset = Position::Directions4;

constexpr int triangles[] = {3, 9, 3, 6, 6, 12, 9, 12};

using puz_quilt = map<Position, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    map<Position, int> m_pos2num;
    vector<puz_quilt> m_quilts;
    map<int, vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    int& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    vector<Position> zeros;
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        m_cells.push_back(PUZ_BOUNDARY);
        string_view str = strs[r - 1];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_UNKNOWN);
            else {
                m_cells.push_back(PUZ_FILLED);
                if (ch != PUZ_WHITE) {
                    Position p(r, c);
                    if (int n = ch - '0'; n == 0)
                        zeros.push_back(p);
                    else
                        m_pos2num[p] = n;
                }
            }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_BOUNDARY);

    for (auto& p : zeros)
        for (auto& os : offset)
            if (int& n = cells(p + os); n == PUZ_UNKNOWN)
                n = PUZ_BLANK;
    
    for (auto& [p, n] : m_pos2num) {
        auto& perms = m_num2perms[n];
        if (!perms.empty()) continue;
        vector<string> perms12;
        string s1(n, ' ');
        for (int i = 0; i < 1 << n; ++i) {
            for (int j = 0; j < n; ++j)
                s1[j] = (i & 1 << j) == 0 ? PUZ_TRIANGLE1 : PUZ_TRIANGLE2;
            perms12.push_back(s1);
        }
        auto perm = string(4 - n, '0') + "1";
        do {
            for (auto& s1 : perms12) {
                string s2;
                for (int i = 0; i <= 4 - n; ++i)
                    if (perm[i] == '0')
                        s2 += PUZ_NON_TRIANGLE;
                    else
                        s2 += s1;
                perms.push_back(s2);
            }
        } while (boost::next_permutation(perm));
    }

    //    (j,k) = 1,4
    //    06 12 .. .. ..
    //    03 15 12 .. ..
    //    .. 03 15 12 ..
    //    .. .. 03 15 12
    //    .. .. .. 03 09
    //
    //    (j,k) = 2,3
    //    .. 06 12 .. ..
    //    06 15 15 12 ..
    //    03 15 15 15 12
    //    .. 03 15 15 09
    //    .. .. 03 09 ..
    //
    //    (j,k) = 3,2
    //    .. .. 06 12 ..
    //    .. 06 15 15 12
    //    06 15 15 15 09
    //    03 15 15 09 ..
    //    .. 03 09 .. ..
    //
    //    (j,k) = 4,1
    //    .. .. .. 06 12
    //    .. .. 06 15 09
    //    .. 06 15 09 ..
    //    06 15 09 .. ..
    //    03 09 .. .. ..
    //
    // Find all tilted quilts
    // A tilted quilt has a circumscribed square
    for (int i = 2; i <= m_sidelen; ++i)
        for (int j = 1; j <= i - 1; ++j) {
            puz_quilt pos2num;
            for (int k = i - j, dr = 0; dr < i; ++dr) {
                int m1, m2, n1, n2;
                if (dr < min(j, k))
                    m1 = j - 1 - dr, m2 = j + dr, n1 = PUZ_UPPER_LEFT, n2 = PUZ_UPPER_RIGHT;
                else if (dr < j)
                    m1 = j - 1 - dr, m2 = j + dr, n1 = PUZ_UPPER_LEFT, n2 = PUZ_LOWER_RIGHT;
                else if (dr < max(j, k))
                    m1 = dr - j, m2 = j + dr, n1 = PUZ_LOWER_LEFT, n2 = PUZ_UPPER_RIGHT;
                else
                    m1 = dr - j, m2 = i - 1 - dr + max(j, k), n1 = PUZ_LOWER_LEFT, n2 = PUZ_LOWER_RIGHT;
                for (int dc = 0; dc < i; ++dc) {
                    Position p(dr, dc);
                    if (dc == m1)
                        pos2num[p] = n1;
                    else if (dc == m2)
                        pos2num[p] = n2;
                    else if (dc > m1 && dc < m2)
                        pos2num[p] = PUZ_BLANK;
                }
            }
            for (int r = 1; r <= m_sidelen - i + 1; ++r)
                for (int c = 1; c <= m_sidelen - i + 1; ++c)
                    if (Position p(r, c);
                        boost::algorithm::all_of(pos2num, [&](const pair<const Position, int>& kv) {
                        return cells(p + kv.first) == PUZ_UNKNOWN;
                    })) {
                        puz_quilt quilt;
                        for (auto& [dp, n] : pos2num)
                            quilt[p + dp] = n;
                        m_quilts.push_back(quilt);
                    }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells;
    }
    bool make_move_triangle(int n);
    bool make_move_hint(const Position& p, int n);
    bool make_move_quilt(int n);
    void check_board();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_UNKNOWN) + m_pos2triangle.size() +
        boost::accumulate(m_matches_hint, 0, [&](int acc, const pair<const Position, vector<int>>& kv) {
            return acc + kv.second.size() + 1;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    map<Position, vector<int>> m_matches_hint;
    vector<int> m_matches_quilt;
    map<Position, int> m_pos2triangle;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches_quilt(g.m_quilts.size())
{
    boost::iota(m_matches_quilt, 0);
    for (auto& [p, n] : m_game->m_pos2num) {
        auto& perms = m_game->m_num2perms.at(n);
        auto& v = m_matches_hint[p];
        boost::iota(v, 0);
    }
    check_board();
}

void puz_state::check_board()
{
    
}

bool puz_state::make_move_triangle(int n)
{
    return true;
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    return true;
}

bool puz_state::make_move_quilt(int n)
{
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_pos2triangle.empty()) {
        for (int n : m_matches_quilt)
            if (!children.emplace_back(*this).make_move_triangle(n))
                children.pop_back();
    } else if (m_matches_hint.empty()) {
        auto& [p, v] = *boost::min_element(m_matches_hint, [&](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : v)
            if (!children.emplace_back(*this).make_move_hint(p, n))
                children.pop_back();
    } else {
        for (int n : m_matches_quilt)
            if (!children.emplace_back(*this).make_move_quilt(n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << format("{:<2}", (ch == PUZ_SPACE ? '.' : ch));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ProofOfQuilt()
{
    using namespace puzzles::ProofOfQuilt;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ProofOfQuilt.xml", "Puzzles/ProofOfQuilt.txt", solution_format::GOAL_STATE_ONLY);
}
