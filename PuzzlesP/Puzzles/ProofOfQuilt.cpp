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
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_BLANK = '.';
constexpr auto PUZ_UPPER_LEFT = 'A';
constexpr auto PUZ_UPPER_RIGHT = 'B';
constexpr auto PUZ_LOWER_LEFT = 'C';
constexpr auto PUZ_LOWER_RIGHT = 'D';
constexpr auto PUZ_NON_TRIANGLE = '0';
constexpr auto PUZ_TRIANGLE1 = '1';
constexpr auto PUZ_TRIANGLE2 = '2';

constexpr auto offset = Position::Directions4;

constexpr string triangles = "CDACABBD";

using puz_quilt = map<Position, char>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    vector<puz_quilt> m_quilts;
    map<int, vector<string>> m_num2perms;
    set<Position> m_blanks;

    puz_game(const vector<string>& strs, const xml_node& level);
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
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
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE) {
                m_cells.push_back(PUZ_SPACE);
                m_blanks.insert(p);
            } else {
                m_cells.push_back(PUZ_WHITE);
                if (ch != PUZ_WHITE)
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
            if (char& ch = cells(p + os); ch == PUZ_SPACE)
                ch = PUZ_BLANK;
    
    for (auto& [p, n] : m_pos2num) {
        auto& perms = m_num2perms[n];
        if (!perms.empty()) continue;
        vector<string> permsB;
        string s1(n, ' ');
        for (int i = 0; i < 1 << n; ++i) {
            for (int j = 0; j < n; ++j)
                s1[j] = (i & 1 << j) == 0 ? PUZ_TRIANGLE1 : PUZ_TRIANGLE2;
            permsB.push_back(s1);
        }
        auto perm = string(4 - n, '0') + string(n, '1');
        do {
            for (auto& s1 : permsB) {
                string s2;
                for (int i = 0, j = 0; i <= 4; ++i)
                    if (perm[i] == '0')
                        s2 += PUZ_NON_TRIANGLE;
                    else
                        s2 += s1[j++];
                perms.push_back(s2);
            }
        } while (boost::next_permutation(perm));
    }

    //    (j,k) = 1,4
    //    A B . . .
    //    C . B . .
    //    . C . B .
    //    . . C . B
    //    . . . C D
    //
    //    (j,k) = 2,3
    //    . A B . .
    //    A . . B .
    //    C . . . B
    //    . C . . D
    //    . . C D .
    //
    //    (j,k) = 3,2
    //    . . A B .
    //    . A . . B
    //    A . . . D
    //    C . . D .
    //    . C D . .
    //
    //    (j,k) = 4,1
    //    . . . A B
    //    . . A . D
    //    . A . D .
    //    A . D . .
    //    C D . . .
    //
    // Find all tilted quilts
    // A tilted quilt has a circumscribed square
    for (int i = 2; i <= m_sidelen; ++i)
        for (int j = 1; j <= i - 1; ++j) {
            puz_quilt pos2char;
            for (int k = i - j, dr = 0; dr < i; ++dr) {
                int m1, m2, ch1, ch2;
                if (dr < min(j, k))
                    m1 = j - 1 - dr, m2 = j + dr, ch1 = PUZ_UPPER_LEFT, ch2 = PUZ_UPPER_RIGHT;
                else if (dr < j)
                    m1 = j - 1 - dr, m2 = j + dr, ch1 = PUZ_UPPER_LEFT, ch2 = PUZ_LOWER_RIGHT;
                else if (dr < max(j, k))
                    m1 = dr - j, m2 = j + dr, ch1 = PUZ_LOWER_LEFT, ch2 = PUZ_UPPER_RIGHT;
                else
                    m1 = dr - j, m2 = i - 1 - dr + max(j, k), ch1 = PUZ_LOWER_LEFT, ch2 = PUZ_LOWER_RIGHT;
                for (int dc = 0; dc < i; ++dc) {
                    Position p(dr, dc);
                    if (dc == m1)
                        pos2char[p] = ch1;
                    else if (dc == m2)
                        pos2char[p] = ch2;
                    else if (dc > m1 && dc < m2)
                        pos2char[p] = PUZ_BLANK;
                }
            }
            for (int r = 1; r <= m_sidelen - i + 1; ++r)
                for (int c = 1; c <= m_sidelen - i + 1; ++c)
                    if (Position p(r, c);
                        boost::algorithm::all_of(pos2char, [&](const pair<const Position, char>& kv) {
                        return cells(p + kv.first) == PUZ_SPACE;
                    })) {
                        puz_quilt quilt;
                        for (auto& [dp, ch] : pos2char)
                            quilt[p + dp] = ch;
                        m_quilts.push_back(quilt);
                    }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_pos2perm_ids, m_pos2triangle) < tie(x.m_cells, x.m_pos2perm_ids, x.m_pos2triangle);
    }
    bool make_move_triangle(int quilt_id);
    bool make_move_hint(const Position& p, int perm_id);
    bool make_move_quilt(int quilt_id);
    void make_move_quilt2(int quilt_id);
    bool check_hints();
    void check_quilts();
    bool check_rectangles() const;
    void check_blanks();
    bool check_triangles();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_pos2triangle.size() +
        boost::accumulate(m_pos2perm_ids, 0, [&](int acc, const pair<const Position, vector<int>>& kv) {
            return acc + kv.second.size() + 1;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    set<Position> m_blanks;
    map<Position, vector<int>> m_pos2perm_ids;
    vector<int> m_quilt_ids;
    map<Position, int> m_pos2triangle;
    vector<int> m_triangle_quilt_ids;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_blanks(g.m_blanks)
, m_quilt_ids(g.m_quilts.size())
{
    boost::iota(m_quilt_ids, 0);
    for (auto& [p, n] : m_game->m_pos2num) {
        auto& perms = m_game->m_num2perms.at(n);
        auto& v = m_pos2perm_ids[p];
        v.resize(perms.size());
        boost::iota(v, 0);
    }
    check_hints();
}

bool puz_state::check_hints()
{
    for (auto& [p, perm_ids] : m_pos2perm_ids) {
        auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
        boost::remove_erase_if(perm_ids, [&](int perm_id) {
            auto& perm = perms[perm_id];
            for (int i = 0; i < 4; ++i) {
                char ch2 = perm[i];
                if (char ch = cells(p + offset[i]);
                    !(ch == PUZ_SPACE ||
                      (ch == PUZ_BOUNDARY || ch == PUZ_WHITE) && ch2 == PUZ_NON_TRIANGLE ||
                      ch == (ch2 == PUZ_NON_TRIANGLE ? PUZ_BLANK : triangles[i * 2 + (ch2 - PUZ_TRIANGLE1)])))
                    return true;
            }
            return false;
        });
        if (perm_ids.empty())
            return false;
    }
    return true;
}

void puz_state::check_quilts()
{
    boost::remove_erase_if(m_quilt_ids, [&](int quilt_id) {
        auto& quilt = m_game->m_quilts[quilt_id];
        return !boost::algorithm::all_of(quilt, [&](const pair<const Position, char>& kv) {
            auto& [p, ch2] = kv;
            int ch1 = cells(p);
            return ch1 == PUZ_SPACE || ch1 == ch2;
        });
    });
}

void puz_state::make_move_quilt2(int quilt_id)
{
    auto& quilt = m_game->m_quilts[quilt_id];
    for (auto& [p, ch2] : quilt) {
        if (char& ch = cells(p); ch == PUZ_SPACE)
            ch = ch2, m_blanks.erase(p);
        m_pos2triangle.erase(p);
    }
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p) : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        switch (auto p2 = *this + os; m_state->cells(p2)) {
        case PUZ_BLANK:
        case PUZ_SPACE:
            children.emplace_back(*this).make_move(p2);
            break;
        }
}

bool puz_state::check_rectangles() const
{
    auto blanks = m_blanks;
    while (!blanks.empty()) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves(
            {this, *blanks.begin()});
        if (boost::algorithm::all_of(smoves, [&](const puz_state2& s) {
            return cells(s) == PUZ_BLANK;
        })) {
            int r1 = 99, c1 = 99, r2 = 0, c2 = 0;
            for (auto& s : smoves) {
                auto& [r, c] = static_cast<const Position&>(s);
                r1 = min(r1, r);
                c1 = min(c1, c);
                r2 = max(r2, r);
                c2 = max(c2, c);
            }
            int rs = r2 - r1 + 1, cs = c2 - c1 + 1;
            if (rs * cs != smoves.size())
                return false;
        }
        for (auto& p : smoves)
            blanks.erase(p);
    }
    return true;
}

void puz_state::check_blanks()
{
    set<Position> rng;
    for (int quilt_id : m_quilt_ids) {
        auto& quilt = m_game->m_quilts[quilt_id];
        for (auto& [p, ch] : quilt)
            rng.insert(p);
    }
    for (auto& p : m_blanks)
        if (char& ch = cells(p); ch == PUZ_SPACE && !rng.contains(p))
            ch = PUZ_BLANK;
}

bool puz_state::check_triangles()
{
    if (m_pos2triangle.empty()) {
        m_triangle_quilt_ids.clear();
        return true;
    }
    m_triangle_quilt_ids = m_quilt_ids;
    boost::remove_erase_if(m_triangle_quilt_ids, [&](int quilt_id) {
        auto& quilt = m_game->m_quilts[quilt_id];
        return boost::algorithm::none_of(quilt, [&](const pair<const Position, char>& kv) {
            auto& [p, ch] = kv;
            auto it = m_pos2triangle.find(p);
            return it != m_pos2triangle.end() && it->second == ch;
        }) || !boost::algorithm::all_of(quilt, [&](const pair<const Position, char>& kv) {
            auto& [p, ch] = kv;
            auto it = m_pos2triangle.find(p);
            return it == m_pos2triangle.end() || it->second == ch;
        });
    });
    return boost::algorithm::all_of(m_pos2triangle, [&](const pair<const Position, char>& kv) {
        auto& [p, ch] = kv;
        return boost::algorithm::any_of(m_triangle_quilt_ids, [&](int quilt_id) {
            auto& quilt = m_game->m_quilts[quilt_id];
            return boost::algorithm::any_of(quilt, [&](const pair<const Position, char>& kv) {
                auto& [p2, ch2] = kv;
                return p == p2 && ch == ch2;
            });
        });
    });
}

bool puz_state::make_move_triangle(int quilt_id)
{
    auto h = get_heuristic();
    make_move_quilt2(quilt_id);
    boost::remove_erase(m_quilt_ids, quilt_id);
    check_quilts();
    check_blanks();
    m_distance = h - get_heuristic();
    return check_triangles() && check_rectangles();
}

bool puz_state::make_move_hint(const Position& p, int perm_id)
{
    auto h = get_heuristic();
    auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[perm_id];
    for (int i = 0; i < 4; ++i) {
        auto p2 = p + offset[i];
        char ch2 = perm[i];
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            m_pos2triangle[p2] = ch = ch2 == PUZ_NON_TRIANGLE ? PUZ_BLANK : triangles[i * 2 + (ch2 - PUZ_TRIANGLE1)], m_blanks.erase(p2);
    }
    m_pos2perm_ids.erase(p);
    if (!check_hints())
        return false;
    check_quilts();
    check_blanks();
    m_distance = h - get_heuristic();
    return check_triangles() && check_rectangles();
}

bool puz_state::make_move_quilt(int quilt_id)
{
    auto h = get_heuristic();
    make_move_quilt2(quilt_id);
    boost::remove_erase_if(m_quilt_ids, [&](int quilt_id2) {
        return quilt_id2 <= quilt_id;
    });
    check_quilts();
    check_blanks();
    m_distance = h - get_heuristic();
    return check_rectangles();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_pos2triangle.empty()) {
        for (int quilt_id : m_triangle_quilt_ids)
            if (!children.emplace_back(*this).make_move_triangle(quilt_id))
                children.pop_back();
    } else if (!m_pos2perm_ids.empty()) {
        auto& [p, perm_ids] = *boost::min_element(m_pos2perm_ids, [&](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int perm_id : perm_ids)
            if (!children.emplace_back(*this).make_move_hint(p, perm_id))
                children.pop_back();
    } else {
        for (int quilt_id : m_quilt_ids)
            if (!children.emplace_back(*this).make_move_quilt(quilt_id))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
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
