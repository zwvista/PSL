#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 4/Castle Patrol

    Summary
    Don't fall down the wall

    Description
    1. Divide the grid into walls and empty areas. Every area contains one number.
    2. The number indicates the size of the area. Numbers in wall tiles are part
       of wall areas; numbers in empty tiles are part of empty areas.
    3. Areas of the same type cannot share an edge.
*/

namespace puzzles::CastlePatrol{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_area
{
    // key: position of the number (hint)
    Position m_start;
    // empty or wall area
    char m_ch;
    // number of the tiles occupied by the area
    int m_num;
    set<Position> m_must_reach_rng;
};

struct puz_move 
{
    // key: position of the number (hint)
    Position m_start;
    // empty or wall area
    char m_ch;
    set<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_area> m_pos2area;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const puz_area* area)
        : m_game(game), m_area(area) { make_move(area->m_start); }
    bool make_move(const Position& p);

    bool is_goal_state() const { return get_heuristic() == 0; }
    unsigned int get_heuristic() const { return m_area->m_num - size(); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const puz_area* m_area;
};

struct puz_state3 : pair<Position, unsigned int>
{
    puz_state3(const puz_state2* s, const puz_area* area)
        : m_state(s), m_area(area), m_can_share_edge(s->m_area->m_ch != m_area->m_ch) {
        second = area->m_num;
        make_move(area->m_start);
    }
    const puz_game& game() const { return *m_state->m_game; }
    void make_move(const Position& p) { first = p, --second; }

    bool is_goal_state() const { return get_heuristic() == 0; }
    unsigned int get_heuristic() const { return second; }
    void gen_children(list<puz_state3>& children) const;
    unsigned int get_distance(const puz_state3& child) const { return 1; }

    const puz_state2* m_state;
    const puz_area* m_area;
    bool m_can_share_edge;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        if (auto p2 = first + os;
            game().cells(p2) == PUZ_SPACE && !m_state->contains(p2) &&
            boost::algorithm::none_of(offset, [&](const Position& os2) {
            auto p3 = p2 + os2;
            return !m_can_share_edge && m_state->contains(p3) ||
                p3 != m_area->m_start && game().cells(p3) == m_area->m_ch;
        })) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

bool puz_state2::make_move(const Position& p)
{
    if (!empty())
        for (auto& [p2, area] : m_game->m_pos2area)
            if (m_area != &area) {
                puz_state3 sstart(this, &area);
                list<list<puz_state3>> spaths;
                if (auto [found, _3] = puz_solver_astar<puz_state3>::find_solution(sstart, spaths); !found)
                    return false;
            }
    insert(p);
    if (size() > 1 && !m_area->m_must_reach_rng.empty()) {
        auto h = get_heuristic();
        if (!boost::algorithm::all_of(m_area->m_must_reach_rng, [&](const Position& p2) {
            if (contains(p2))
                return true;
            auto it = boost::min_element(*this, [&](const Position& p3, const Position& p4) {
                return manhattan_distance(p3, p2) < manhattan_distance(p4, p2);
            });
            return manhattan_distance(*it, p2) <= h;
        }))
            return false;
    }
    return true;
}

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // Areas extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile can be occupied by the area
            // if it is a space tile and has not been occupied by the area and
            if (ch2 == PUZ_SPACE && !contains(p2) && boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                // no adjacent tile to it is of the same type as the area, because
                // 3. Areas of the same type cannot share an edge.
                return !contains(p3) && ch3 == m_area->m_ch;
            }))
                if (!children.emplace_back(*this).make_move(p2))
                    children.pop_back();
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch1 = str[c * 2 - 2], ch2 = str[c * 2 - 1];
            m_cells.push_back(ch2);
            if (ch1 != ' ') {
                Position p(r, c);
                auto& [start, ch, num, _1] = m_pos2area[p];
                start = p, ch = ch2;
                num = isdigit(ch1) ? ch1 - '0' : ch1 - 'A' + 10;
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (int r = 1; r < m_sidelen - 1; ++r)
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (Position p(r, c); !m_pos2area.contains(p)) {
                set<Position> rng;
                for (auto& [p2, area] : m_pos2area)
                    if (manhattan_distance(p, p2) < area.m_num)
                        rng.insert(p2);
                if (rng.size() == 1)
                    m_pos2area.at(*rng.begin()).m_must_reach_rng.insert(p);
            }

    for (auto& [p, area] : m_pos2area) {
        puz_state2 sstart(this, &area);
        list<list<puz_state2>> spaths;
        // Areas can have any form.
        if (auto [found, _1] = puz_solver_astar<puz_state2, false, false>::find_solution(sstart, spaths); found)
            // save all goal states as permutations
            // A goal state is a area formed from the number
            for (auto& spath : spaths) {
                auto& rng = spath.back();
                int n = m_moves.size();
                m_moves.emplace_back(p, area.m_ch, rng);
                for (auto& p2 : rng)
                    m_pos2move_ids[p2].push_back(n);
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
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number (hint)
    // value : the index of the move
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [start, ch, rng] = m_game->m_moves[id];
            return !boost::algorithm::all_of(rng, [&](const Position& p2) {
                return cells(p2) == PUZ_SPACE &&
                    boost::algorithm::none_of(offset, [&](const Position& os) {
                    // no adjacent tile to it is of the same type as the area, because
                    // 3. Areas of the same type cannot share an edge.
                    auto p3 = p2 + os;
                    return start != p3 && cells(p3) == ch;
                }) || start == p2;
            });
        });
        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [_1, ch, rng] = m_game->m_moves[n];
    for (auto& p2 : rng)
        cells(p2) = ch, ++m_distance, m_matches.erase(p2);
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
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (auto it = m_game->m_pos2area.find(p); it == m_game->m_pos2area.end())
                out << "  ";
            else
                out << format("{:2}", it->second.m_num);
            out << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CastlePatrol()
{
    using namespace puzzles::CastlePatrol;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CastlePatrol.xml", "Puzzles/CastlePatrol.txt", solution_format::GOAL_STATE_ONLY);
}
