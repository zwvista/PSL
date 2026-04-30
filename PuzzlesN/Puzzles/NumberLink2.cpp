#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/NumberLink

    Summary
    Connect the same numbers without the crossing paths

    Description
    1. Connect the couples of equal numbers (i.e. 2 with 2, 3 with 3 etc)
       with a continuous line.
    2. The line can only go horizontally or vertically and can't cross
       itself or other lines.
    3. Lines must originate on a number and must end in the other equal
       number.
    4. At the end of the puzzle, you must have covered ALL the squares with
       lines and no line can cover a 2*2 area (like a 180 degree turn).
    5. In other words you can't turn right and immediately right again. The
       same happens on the left, obviously. Be careful not to miss this rule.

    Variant
    6. In some levels there will be a note that tells you don't need to cover
       all the squares.
    7. In some levels you will have more than a couple of the same number.
       In these cases, you must connect ALL the same numbers together.
*/

namespace puzzles::NumberLink2{

constexpr auto PUZ_SPACE = ' ';

constexpr array<Position, 4> offset = Position::Directions4;

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;

struct puz_move
{
    vector<Position> m_path;
    set<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2num;
    map<char, vector<Position>> m_num2rng;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    bool m_no_board_fill;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : vector<pair<Position, int>>
{
    puz_state2(const puz_game* game, const Position& p, char num, const vector<Position> rng)
        : m_game(game), m_p(p), m_num(num), m_rng(rng) { make_move(-1, p); }

    bool is_goal_state() const {
        return boost::algorithm::all_of(m_rng, [&](const Position& p) {
            return boost::algorithm::any_of(*this, [&](const pair<Position, int>& kv) {
                return kv.first == p;
            });
        });
    }
    void make_move(int i, const Position& p) { emplace_back(p, i); }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    Position m_p;
    char m_num;
    vector<Position> m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (is_goal_state())
        return;
    auto& t = back();
    for (int i = 0; i < 4; ++i) {
        if (size() > 2) {
            // 5. In other words you can't turn right and immediately right again. The
            //    same happens on the left, obviously. Be careful not to miss this rule.
            if (int d = (t.second + 4 - rbegin()[1].second) % 4;
                (d == 1 || d == 3) && (i + 4 - t.second) % 4 == d)
                continue;
        }
        if (auto p2 = t.first + offset[i];
            m_game->is_valid(p2) && boost::algorithm::none_of(*this, [&](const pair<Position, int>& kv) {
            return kv.first == p2;
        }))
            if (char ch2 = m_game->cells(p2);
                ch2 == PUZ_SPACE || ch2 == m_num)
                children.emplace_back(*this).make_move(i, p2);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_no_board_fill(level.attribute("NoBoardFill").as_int() == 1)
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE) {
                Position p(r, c);
                m_pos2num[p] = ch, m_num2rng[ch].push_back(p);
            }
    }

    for (auto& [p, num] : m_pos2num) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, num, m_num2rng.at(num)});
        for (auto& s : smoves)
            if (s.is_goal_state()) {
                puz_move move;
                for (auto& [p2, _1] : s)
                    move.m_path.push_back(p2), move.m_rng.insert(p2);
                if (boost::algorithm::any_of(m_moves, [&](const puz_move& move2) {
                    return move2.m_rng == move.m_rng;
                })) continue;
                int n = m_moves.size();
                m_moves.push_back(move);
                m_pos2move_ids[p].push_back(n);
                for (auto& [p2, _1] : s)
                    m_pos2move_ids[p2].push_back(n);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    int dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    int& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
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
    vector<int> m_dots;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_dots(g.m_sidelen * g.m_sidelen)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [path, _2] = m_game->m_moves[id];
            return !boost::algorithm::all_of(path, [&](const Position& p2) {
                return dots(p2) == lineseg_off;
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
    auto& [path, _1] = m_game->m_moves[n];
    for (int i = 0, sz = path.size(); i < sz; ++i) {
        auto& p2 = path[i];
        auto f = [&](const Position& p3) {
            int dir = boost::find(offset, p3 - p2) - offset.begin();
            dots(p2) |= 1 << dir;
            dots(p3) |= 1 << (dir + 2) % 4;
            m_matches.erase(p2), ++m_distance;
        };
        if (i < sz - 1)
            f(path[i + 1]);
        if (i > 0)
            f(path[i - 1]);
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
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
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
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << m_game->cells(p)
                << (is_lineseg_on(dots(p), 1) ? '-' : ' ');
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c}), 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_NumberLink2()
{
    using namespace puzzles::NumberLink2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberLink.xml", "Puzzles/NumberLink2.txt", solution_format::GOAL_STATE_ONLY);
}
