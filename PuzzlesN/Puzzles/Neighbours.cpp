#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Neighbours

    Summary
    Neighbours, yes, but not equally sociable

    Description
    1. The board represents a piece of land bought by a bunch of people. They
       decided to split the land in equal parts.
    2. However some people are more social and some are less, so each owner
       wants an exact number of neighbours around him.
    3. Each number on the board represents an owner house and the number of
       neighbours he desires.
    4. Divide the land so that each one has an equal number of squares and
       the requested number of neighbours.
    5. Later on, there will be Question Marks, which represents an owner for
       which you don't know the neighbours preference.
*/

namespace puzzles::Neighbours{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = 'B';
constexpr auto PUZ_QM = '?';
constexpr auto PUZ_UNKNOWN = 0;

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_area_info
{
    puz_area_info(const Position& p, int cnt)
    : m_pHouse(p), m_neighbour_count(cnt) {}
    Position m_pHouse;
    int m_neighbour_count;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<char, puz_area_info> m_id2info;
    int m_cell_count_area;

    puz_game(const vector<string>& strs, const xml_node& level);
    int neighbour_count(char id) const { return m_id2info.at(id).m_neighbour_count; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    for (int r = 1, n = 0; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE) continue;
            char id = n++ + 'a';
            int cnt = ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0';
            Position p(r, c);
            m_pos2num[p] = cnt;
            m_id2info.emplace(id, puz_area_info(p, cnt));
        }
    }
    m_cell_count_area = (m_sidelen - 2) * (m_sidelen - 2) / m_id2info.size();
}

struct puz_area
{
    set<Position> m_inner, m_outer;
    set<char> m_neighbours;
    bool m_ready = false;

    void add_cell(const Position& p, int cnt) {
        m_inner.insert(p);
        m_ready = m_inner.size() == cnt;
    }
    bool add_neighbour(char id, int cnt) {
        m_neighbours.insert(id);
        return cnt == PUZ_UNKNOWN || m_neighbours.size() <= cnt;
    }

    bool operator<(const puz_area& x) const {
        return pair{m_ready, m_outer.size()} < pair{x.m_ready, x.m_outer.size()};
    }
};

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(char id, const Position& p);
    bool make_move2(char id, Position p);
    int adjust_area(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<char, puz_area> m_id2area;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < sidelen(); ++i)
        cells({i, 0}) = cells({i, sidelen() - 1}) =
        cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

    for (auto& [id, info] : g.m_id2info)
        make_move2(id, info.m_pHouse);

    adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
    for (auto& [id, area] : m_id2area) {
        auto& outer = area.m_outer;
        int nb_cnt = m_game->neighbour_count(id);
        if (area.m_ready && outer.empty()) continue;

        outer.clear();
        for (auto& p : area.m_inner)
            for (auto& os : offset) {
                auto p2 = p + os;
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    outer.insert(p2);
            }

        if (!init)
            switch(outer.size()) {
            case 0:
                return !area.m_ready ||
                    nb_cnt != PUZ_UNKNOWN && area.m_neighbours.size() < nb_cnt ? 0 :
                    1;
            case 1:
                if (!area.m_ready)
                    return make_move2(id, *outer.begin()) ? 1 : 0;
                break;
            }
    }
    return 2;
}

bool puz_state::make_move2(char id, Position p)
{
    auto& area = m_id2area[id];
    cells(p) = id;
    area.add_cell(p, m_game->m_cell_count_area);
    ++m_distance;

    for (auto& os : offset) {
        auto p2 = p + os;
        char ch = cells(p2);
        if (ch == PUZ_SPACE || ch == PUZ_BOUNDARY ||
            ch == id || area.m_neighbours.contains(ch))
            continue;

        char id2 = ch;
        auto& area2 = m_id2area.at(id2);
        if (!area.add_neighbour(id2, m_game->neighbour_count(id)) ||
            !area2.add_neighbour(id, m_game->neighbour_count(id2)))
            return false;
    }
    return true;
}

bool puz_state::make_move(char id, const Position& p)
{
    m_distance = 0;
    if (!make_move2(id, p))
        return false;
    int m;
    while ((m = adjust_area(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [id, area] = *boost::min_element(m_id2area, [](
        const pair<const char, puz_area>& kv1,
        const pair<const char, puz_area>& kv2) {
        return kv1.second < kv2.second;
    });
    if (area.m_ready) return;
    for (auto& p : area.m_outer) {
        children.push_back(*this);
        if (!children.back().make_move(id, p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (auto& [id, area] : m_id2area)
        for (auto& p : area.m_inner)
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (!area.m_inner.contains(p2))
                    walls.insert(p_wall);
            }

    for (int r = 1;; ++r) {
        // draw horizontal walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            char id = cells(p);
            auto& info = m_game->m_id2info.at(id);
            if (info.m_pHouse != p)
                out << ' ';
            else
                out << m_id2area.at(id).m_neighbours.size();
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Neighbours()
{
    using namespace puzzles::Neighbours;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Neighbours.xml", "Puzzles/Neighbours.txt", solution_format::GOAL_STATE_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Neighbours2.xml", "Puzzles/Neighbours2.txt", solution_format::GOAL_STATE_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Neighbours3.xml", "Puzzles/Neighbours3.txt", solution_format::GOAL_STATE_ONLY);
}
