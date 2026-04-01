#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 16/Snake

    Summary
    Still lives inside your pocket-sized computer

    Description
    1. Complete the Snake, head to tail, inside the board.
    2. The two tiles given at the start are the head and the tail of the snake
       (it is irrelevant which is which).
    3. Numbers on the border tell you how many tiles the snake occupies in that
       row or column.
    4. The snake can't touch itself, not even diagonally.
*/

namespace puzzles::Snake{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SNAKE = 'S';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;   
    map<int, int> m_area2piece_count;
    Position m_head_tail[2];
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_area2range;
    map<int, vector<string>> m_num2perms;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_area2range(m_sidelen * 2)
    , m_cells(m_sidelen * m_sidelen, PUZ_SPACE)
{
    set<int> nums;
    for (int r = 0, n = 0; r <= m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c <= m_sidelen; c++) {
            Position p(r, c);
            if (char ch = str[c]; ch == PUZ_SNAKE)
                cells(m_head_tail[n++] = p) = ch;
            else if (c == m_sidelen || r == m_sidelen) {
                if (c == m_sidelen && r == m_sidelen) continue;
                if (!isdigit(ch)) continue;
                int n = ch - '0';
                nums.insert(n);
                m_area2piece_count[c == m_sidelen ? r : c + m_sidelen] = n;
            }
        }
    }
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }

    for (int i = 0; i <= m_sidelen; ++i) {
        if (!nums.contains(i)) continue;
        auto& perms = m_num2perms[i];
        auto perm = string(m_sidelen - i, PUZ_EMPTY) + string(i, PUZ_SNAKE);
        do {
            perms.push_back(perm);
        } while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move_hint(int i, int j);
    bool make_move_hint2(int i, int j);
    int find_matches(bool init);
    bool check_snake();
    bool make_move_space(const Position& p, char ch);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g)
{
    for (auto& [i, n] : g.m_area2piece_count) {
        auto& perm_ids = m_matches[i];
        perm_ids.resize(g.m_num2perms.at(n).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& range = m_game->m_area2range[area_id];
        auto& perms = m_game->m_num2perms.at(m_game->m_area2piece_count.at(area_id));
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(range, perms[id], [&](const Position& p, char ch2) {
                char ch1 = cells(p);
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(area_id, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* state)
        : m_state(state) { make_move_hint(state->m_game->m_head_tail[0]); }

    void make_move_hint(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; m_state->is_valid(p2))
            if (char ch = m_state->cells(p2); ch != PUZ_EMPTY)
                children.emplace_back(*this).make_move_hint(p2);
}

bool puz_state::check_snake()
{
    bool b1 = is_goal_state();
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SNAKE) continue;
            bool b2 = p == m_game->m_head_tail[0] || p == m_game->m_head_tail[1];
            int pieces = b2 ? 1 : 2;
            int num_snake = 0, num_empty = 0;
            vector<Position> rng;
            for (auto& os : offset) {
                auto p2 = p + os;
                if (!is_valid(p2)) continue;
                switch (cells(p2)) {
                case PUZ_SNAKE: ++num_snake; break;
                case PUZ_EMPTY: ++num_empty; break;
                case PUZ_SPACE: rng.push_back(p2); break;
                }
            }
            if (is_goal_state() && num_snake != pieces || num_snake > pieces || num_snake + rng.size() < pieces)
                return false;
            if (num_snake == pieces)
                for (auto& p2 : rng)
                    cells(p2) = PUZ_EMPTY, ++m_distance;
            if (num_snake < pieces && num_snake + rng.size() == pieces)
                for (auto& p2 : rng)
                    cells(p2) = PUZ_SNAKE, ++m_distance;
        }

    auto smoves = puz_move_generator<puz_state2>::gen_moves({this});
    return boost::count_if(smoves, [&](const Position& p) {
        return cells(p) == PUZ_SNAKE;
    }) == boost::count(m_cells, PUZ_SNAKE);
}

bool puz_state::make_move_hint2(int i, int j)
{
    auto& range = m_game->m_area2range[i];
    auto& perm = m_game->m_num2perms.at(m_game->m_area2piece_count.at(i))[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = range[k];
        if (char& ch = cells(p); ch == PUZ_SPACE && (ch = perm[k]) == PUZ_SNAKE)
            ++m_distance;
    }
    m_matches.erase(i); ++m_distance;
    return check_snake();
}

bool puz_state::make_move_hint(int i, int j)
{
    m_distance = 0;
    if (!make_move_hint2(i, j))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_space(const Position &p, char ch)
{
    cells(p) = ch;
    m_distance = 1;
    return check_snake();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const int, vector<int>>& kv1,
            const pair<const int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : perm_ids)
            if (!children.emplace_back(*this).make_move_hint(area_id, n))
                children.pop_back();
    } else {
        int i = m_cells.find(PUZ_SPACE);
        Position p(i / sidelen(), i % sidelen());
        for (char ch : {PUZ_EMPTY, PUZ_SNAKE})
            if (!children.emplace_back(*this).make_move_space(p, ch))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int i) {
        return boost::count_if(m_game->m_area2range[i], [&](const Position& p) {
            return cells(p) == PUZ_SNAKE;
        });
    };
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen())
                out << format("{:<2}", f(r));
            else if (r == sidelen())
                out << format("{:<2}", f(c + sidelen()));
            else
                out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_Snake()
{
    using namespace puzzles::Snake;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Snake.xml", "Puzzles/Snake.txt", solution_format::GOAL_STATE_ONLY);
}
