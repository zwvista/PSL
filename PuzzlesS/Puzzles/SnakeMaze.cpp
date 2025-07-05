#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Snake Maze

    Summary
    Find the snakes using the given hints.

    Description
    1. A Snake is a path of five tiles, numbered 1-2-3-4-5, where 1 is the head and 5 the tail.
       The snake's body segments are connected horizontally or vertically.
    2. A snake cannot see another snake or it would attack it. A snake sees straight in the
       direction 2-1, that is to say it sees in front of the number 1.
    3. A snake cannot touch another snake horizontally or vertically.
    4. Arrows show you the closest piece of Snake in that direction (before another arrow or the edge).
    5. Arrows with zero mean that there is no Snake in that direction.
    6. arrows block snake sight and also block other arrows hints.
*/

namespace puzzles::SnakeMaze{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_BLOCK = 'B';
constexpr auto PUZ_HINT = 'H';
constexpr auto PUZ_SNAKE = 'S';
constexpr auto PUZ_SNAKE_SIZE = 5;

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const string_view dirs = "^>v<";
const string_view space_str = "     ";
const string_view snake_str = "12345";

using puz_hint = pair<int, char>;

struct puz_move
{
    vector<Position> m_snake;
    set<Position> m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, puz_hint> m_pos2hint;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : map<int, Position>
{
    puz_state2(const puz_game& game, int n, const Position& p, const set<Position>& empties)
        : m_game(&game), m_empties(&empties) { make_move(n, p); }
    bool is_self(const Position& p) const {
        return boost::algorithm::any_of(*this, [&](const pair<const int, Position>& kv) {
            return kv.second == p;
        });
    }

    bool is_goal_state() const { return size() == PUZ_SNAKE_SIZE; }
    void make_move(int n, const Position& p) { emplace(n, p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    const set<Position>* m_empties;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    auto f = [&](int n, const Position& p) {
        for (int i = 0; i < 4; ++i)
            if (auto p2 = p + offset[i]; 
                m_game->cells(p2) == PUZ_SPACE && !m_empties->contains(p2) && !is_self(p2) &&
                boost::algorithm::none_of(offset, [&](const Position& os) {
                auto p3 = p2 + os;
                return p3 != p && is_self(p3);
            })) {
                children.push_back(*this);
                children.back().make_move(n, p2);
            }
    };
    auto& [n, p] = *begin();
    if (n > 1)
        f(n - 1, p);
    else {
        auto& [n, p] = *rbegin();
        f(n + 1, p);
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
            switch (Position p(r, c); char ch = str[c * 2 - 2]) {
            case PUZ_SPACE:
            case PUZ_BLOCK:
                m_cells.push_back(ch);
                break;
            default:
                m_pos2hint[p] = {ch - '0', str[c * 2 - 1]};
                m_cells.push_back(PUZ_HINT);
                break;
            }
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

    for (auto& [p, hint] : m_pos2hint) {
        auto& [n, ch] = hint;
        auto& os = offset[dirs.find(ch)];
        vector<Position> rng;
        for (auto p2 = p + os; cells(p2) == PUZ_SPACE; p2 += os)
            rng.push_back(p2);
        if (n == 0)
            for (auto& p2 : rng)
                cells(p2) = PUZ_EMPTY;
        else
            for (auto it = rng.begin(); it != rng.end(); ++it) {
                set<Position> empties2(rng.begin(), it);
                puz_state2 sstart(*this, n, *it, empties2);
                list<list<puz_state2>> spaths;
                if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
                    // save all goal states as permutations
                    // A goal state is a snake formed from the hint
                    for (auto& spath : spaths) {
                        vector<Position> snake;
                        for (auto& [_1, p2] : spath.back())
                            snake.push_back(p2);
                        if (int n2 = boost::find_if(m_moves, [&](const puz_move& move) {
                            return move.m_snake == snake;
                        }) - m_moves.begin(); n2 != m_moves.size())
                            m_pos2move_ids[p].push_back(n2);
                        else {
                            auto empties = empties2;
                            // 3. A snake cannot touch another snake horizontally or vertically.
                            for (auto& p2 : snake)
                                for (auto& os2 : offset)
                                    if (auto p3 = p2 + os2;
                                        cells(p3) == PUZ_SPACE &&
                                        boost::algorithm::none_of_equal(snake, p3))
                                        empties.insert(p3);
                            // 2. A snake cannot see another snake or it would attack it. A snake sees straight in the
                            // direction 2-1, that is to say it sees in front of the number 1.
                            auto &p0 = snake[0], &p1 = snake[1], os2 = p0 - p1;
                            for (auto p2 = p0 + os2; cells(p2) == PUZ_SPACE; p2 += os2)
                                empties.insert(p2);

                            n2 = m_moves.size();
                            m_moves.emplace_back(snake, empties);
                            m_pos2move_ids[p].push_back(n2);
                        }
                    }
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
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the hint
    // value.elem: the index of the move
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [snake, empties] = m_game->m_moves[id];
            string chars;
            for (auto& p2 : snake)
                chars.push_back(cells(p2));
            return !((chars == space_str || chars == snake_str) &&
                boost::algorithm::all_of(empties, [&](const Position& p2) {
                    char ch = cells(p2);
                    return ch == PUZ_SPACE || ch == PUZ_EMPTY;
                }));
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [snake, empties] = m_game->m_moves[n];
    for (int i = 0; i < snake.size(); ++i)
        cells(snake[i]) = i + '1';
    for (auto& p2 : empties)
        cells(p2) = PUZ_EMPTY;
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
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
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_HINT) {
                auto& [n, ch2] = m_game->m_pos2hint.at(p);
                out << n << ch2;
            } else
                out << ch << (isdigit(ch) ? PUZ_SNAKE : ' ');
            out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_SnakeMaze()
{
    using namespace puzzles::SnakeMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SnakeMaze.xml", "Puzzles/SnakeMaze.txt", solution_format::GOAL_STATE_ONLY);
}
