#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/Fillomino

    Summary
    Detect areas marked by their extension

    Description
    1. The goal is to detect areas marked with the tile count of the area
       itself.
    2. So for example, areas marked '1', will always consist of one single
       tile. Areas marked with '2' will consist of two (horizontally or
       vertically) adjacent tiles. Tiles numbered '3' will appear in a group
       of three and so on.
    3. Some areas can also be totally hidden at the start.
    
    Variation
    4. Fillomino has several variants.
    5. No Rectangles: Areas can't form Rectangles.
    6. Only Rectangles: Areas can ONLY form Rectangles.
    7. Non Consecutive: Areas can't touch another area which has +1 or -1
       as number (orthogonally).
    8. Consecutive: Areas MUST touch another area which has +1 or -1
       as number (orthogonally).
    9. All Odds: There are only odd numbers on the board.
    10.All Evens: There are only even numbers on the board.
*/

namespace puzzles::Fillomino{

constexpr auto PUZ_UNKNOWN = 0;

constexpr array<Position, 4> offset = Position::Directions4;

enum class puz_game_type
{
    NORMAL,
    NO_RECTANGLES,
    ONLY_RECTANGLES,
    NON_CONSECUTIVE,
    CONSECUTIVE,
    ALL_ODDS,
    ALL_EVENS,
};

using puz_move = set<Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    puz_game_type m_game_type;
    vector<int> m_cells;
    int m_num_max = 1;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    int cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p, int num)
        : m_game(game), m_num(num) { make_move(p); }

    void make_move(const Position& p);
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    int m_num;
    bool m_is_valid;
};

void puz_state2::make_move(const Position &p)
{
    insert(p);
    if (m_num == PUZ_UNKNOWN)
        m_num = m_game->cells(p);
    int sz = size();
    m_is_valid = m_num == PUZ_UNKNOWN || sz == m_num;
    if (m_game->m_game_type == puz_game_type::ALL_ODDS)
        m_is_valid = m_is_valid && sz % 2 == 1;
    else if(m_game->m_game_type == puz_game_type::ALL_EVENS)
        m_is_valid = m_is_valid && sz % 2 == 0;
    m_is_valid = m_is_valid && boost::algorithm::none_of(*this, [&](const Position& p2) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p3 = p2 + os;
            return m_game->is_valid(p3) && !contains(p3) &&
            m_game->cells(p3) == sz;
        });
    });
    auto is_rect = [&] {
        int r1 = m_game->m_sidelen, c1 = m_game->m_sidelen, r2 = 0, c2 = 0;
        for (auto& p2 : *this) {
            auto& [r, c] = p2;
            if (r1 > r) r1 = r;
            if (c1 > c) c1 = c;
            if (r2 < r) r2 = r;
            if (c2 < c) c2 = c;
        }
        return (r2 - r1 + 1) * (c2 - c1 + 1) == size();
    };
    if (m_game->m_game_type == puz_game_type::ONLY_RECTANGLES)
        m_is_valid = m_is_valid && is_rect();
    else if(m_game->m_game_type == puz_game_type::NO_RECTANGLES)
        m_is_valid = m_is_valid && !is_rect();
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    int sz = size();
    if (m_num == PUZ_UNKNOWN && sz == m_game->m_num_max || sz == m_num)
        return;
    for (auto& p : *this)
        for (auto& os : offset)
            if (auto p2 = p + os; !contains(p2) && m_game->is_valid(p2))
                if (int n = m_game->cells(p2);
                    n == PUZ_UNKNOWN || sz < n && (m_num == PUZ_UNKNOWN || n == m_num))
                    children.emplace_back(*this).make_move(p2);
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    string game_type = level.attribute("GameType").as_string("Fillomino");
    m_game_type =
        game_type == "No Rectangles" ? puz_game_type::NO_RECTANGLES :
        game_type == "Only Rectangles" ? puz_game_type::ONLY_RECTANGLES :
        game_type == "Non Consecutive" ? puz_game_type::NON_CONSECUTIVE :
        game_type == "Consecutive" ? puz_game_type::CONSECUTIVE :
        game_type == "All Odds" ? puz_game_type::ALL_ODDS :
        game_type == "All Evens" ? puz_game_type::ALL_EVENS :
        puz_game_type::NORMAL;

    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            int n = ch == ' ' ? PUZ_UNKNOWN : ch - '0';
            if (m_num_max < n) m_num_max = n;
            m_cells.push_back(n);
        }
    }
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, cells(p)});
            for (auto& s : smoves)
                if (s.m_is_valid && boost::algorithm::none_of_equal(m_moves, s)) {
                    int n = m_moves.size();
                    m_moves.push_back(s);
                    for (auto& p : s)
                        m_pos2move_ids[p].push_back(n);
                }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches(g.m_pos2move_ids)
{
    find_matches(false);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& move = m_game->m_moves[id];
            return boost::algorithm::any_of(move, [&](const Position& p2) {
                int n = cells(p2), num = move.size();
                return n != PUZ_UNKNOWN && n != num ||
                boost::algorithm::any_of(offset, [&](const Position& os) {
                    auto p3 = p2 + os;
                    return boost::algorithm::none_of_equal(move, p3) && is_valid(p3) && cells(p3) == num;
                });
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& move = m_game->m_moves[n];
    int m = move.size();
    for (auto& p : move) {
        cells(p) = m;
        ++m_distance, m_matches.erase(p);
    }
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Fillomino()
{
    using namespace puzzles::Fillomino;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Fillomino.xml", "Puzzles/Fillomino.txt", solution_format::GOAL_STATE_ONLY);
}
