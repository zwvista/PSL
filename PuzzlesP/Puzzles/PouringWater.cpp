#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 3/Pouring Water

    Summary
    Communicating Vessels

    Description
    1. The board represents some communicating vessels.
    2. You have to fill some water in it, considering that water pours down
       and levels itself like in reality.
    3. Areas of the same level which are horizontally connected will have
       the same water level.
    4. The numbers on the border show you how many tiles of each row and
       column are filled.
*/

namespace puzzles::PouringWater{

constexpr auto PUZ_WATER = 'X';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_move
{
    set<Position> m_filled, m_emptied;
    set<int> m_rows;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // index: row or column
    // value: number of tiles filled
    vector<int> m_area2num;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls), m_p_start(p_start) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
    Position m_p_start;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall) && p.first >= m_p_start.first)
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 - 1)
    , m_area2num(2 * m_sidelen)
{
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
        }
    }

    auto f = [&](int area_id, char ch) {
        m_area2num[area_id] = ch == ' ' ? PUZ_UNKNOWN : ch - '0';
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1][m_sidelen * 2 + 1]);
        f(i + m_sidelen, strs[m_sidelen * 2 + 1][i * 2 + 1]);
    }

    for (int r = m_sidelen - 1; r >= 0; --r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (boost::algorithm::any_of(m_moves, [&](const puz_move& move) {
                return move.m_filled.contains(p);
            }))
                continue;
            auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, p});
            auto& [filled, _1, rows] = m_moves.emplace_back();
            for (auto& p2 : smoves) {
                filled.insert(p2);
                rows.insert(p2.first);
            }
        }
    
    vector<Position> keys;
    for (auto& [filled, _2, rows] : m_moves)
        if (rows.size() == 1)
            keys.push_back(*filled.begin());
    for (auto& p : keys) {
        auto& ids = m_pos2move_ids[p];
        for (int i = 0; i < m_moves.size(); ++i)
            if (auto& [filled, _1, _2] = m_moves[i]; filled.contains(p))
                ids.push_back(i);
        boost::sort(ids, [&](int i, int j) {
            return m_moves[i].m_rows.size() < m_moves[j].m_rows.size();
        });
        ids.insert(ids.begin(), m_moves.size());
        m_moves.emplace_back();
        set<Position> emptied;
        for (int i = ids.size() - 1; i > 0; --i) {
            auto& [filled, _1, rows] = m_moves[ids[i]];
            int r = *rows.begin();
            for (auto& p2 : filled)
                if (p2.first == r)
                    emptied.insert(p2);
            m_moves[ids[i - 1]].m_emptied = emptied;
        }
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool check_hint() const;

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [filled, emptied, _2] = m_game->m_moves[id];
            return !boost::algorithm::all_of(filled, [&](const Position& p2) {
                return cells(p2) == PUZ_SPACE;
            }) || !boost::algorithm::all_of(emptied, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            });
        });
        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids.front()), 1;
            }
    }
    return check_hint() ? 2 : 0;
}

bool puz_state::check_hint() const
{
    for (int area_id = 0; area_id < 2 * sidelen(); ++area_id) {
        int num = m_game->m_area2num[area_id];
        if (num == PUZ_UNKNOWN) continue;
        bool is_row = area_id < sidelen();
        int max_possible = 0;
        int min_guaranteed = 0;
        for (int i = 0; i < sidelen(); ++i) {
            char ch = cells(is_row ? Position(area_id, i) : Position(i, area_id - sidelen()));
            if (ch == PUZ_WATER) max_possible++, min_guaranteed++;
            if (ch == PUZ_SPACE) max_possible++;
        }
        // Prune if we can't possibly reach the target
        if (max_possible < num) return false;
        // Prune if we have already exceeded the target
        if (min_guaranteed > num) return false;
        // Final check
        if (is_goal_state() && min_guaranteed != num) return false;
    }
    return true;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [filled, emptied, _1] = m_game->m_moves[n];
    for (auto& p2 : filled)
        cells(p2) = PUZ_WATER;
    for (auto& p2 : emptied)
        cells(p2) = PUZ_EMPTY;
    ++m_distance, m_matches.erase(p);
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
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int area_id) {
        int cnt = 0;
        for (int i = 0; i < sidelen(); ++i)
            if (cells(area_id < sidelen() ? Position(area_id, i) : Position(i, area_id - sidelen())) == PUZ_WATER)
                ++cnt;
        out << (area_id < sidelen() ? format("{:<2}", cnt) : format("{:2}", cnt));
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) {
                f(r);
                break;
            }
            out << cells(p);
        }
        println(out);
    }
    for (int c = 0; c < sidelen(); ++c)
        f(c + sidelen());
    println(out);
    return out;
}

}

void solve_puz_PouringWater()
{
    using namespace puzzles::PouringWater;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PouringWater.xml", "Puzzles/PouringWater.txt", solution_format::GOAL_STATE_ONLY);
}
