#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 14/North Pole Fishing

    Summary
    Fishing Neighbours

    Description
    1. The board represents a piece of antarctic, which some fisherman want
       to split in equal parts.
    2. They decide each one should have a piece of land of exactly 4 squares,
       including one fishing hole.
    3. Divide the land accordingly.
    4. Ice blocks are just obstacles and don't count as piece of land.
*/

namespace puzzles::NorthPoleFishing{

#define PUZ_SPACE        ' '
#define PUZ_BOUNDARY    '+'
#define PUZ_BLOCK        'B'
#define PUZ_HOLE        'H'
#define PUZ_PIECE_SIZE    4

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, Position> m_id2hole;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 2)
{
    char id = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_HOLE) {
                m_start.push_back(PUZ_SPACE);
                m_id2hole[id++] = {r, c};
            } else
                m_start.push_back(ch);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_area
{
    set<Position> m_inner, m_outer;
    bool m_ready = false;

    void add_cell(const Position& p) {
        m_inner.insert(p);
        m_ready = m_inner.size() == PUZ_PIECE_SIZE;
    }
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(char id, const Position& p);
    void make_move2(char id, Position p);
    int adjust_area(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<char, puz_area> m_id2area;
    unsigned int m_distance = 0;
    string m_next;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_id2hole)
        make_move2(kv.first, kv.second);

    adjust_area(true);
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const Position& p, int d);

    void make_move(const Position& p) { 
        static_cast<Position&>(*this) = p;
        --m_distance;
    }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    int m_distance;
};

puz_state2::puz_state2(const puz_state& s, const Position& p, int d)
    : m_state(&s), m_distance(d)
{
    make_move(p);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (m_distance == 0) return;
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_state->cells(p2) == PUZ_SPACE) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

int puz_state::adjust_area(bool init)
{
    for (auto& kv : m_id2area) {
        char id = kv.first;
        auto& area = kv.second;
        auto& outer = area.m_outer;
        outer.clear();
        if (area.m_ready) continue;

        for (auto& p : area.m_inner)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (cells(p2) == PUZ_SPACE)
                    outer.insert(p2);
            }

        if (!init)
            switch(outer.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(id, *outer.begin()), 1;
            }
    }

    // pruning
    map<Position, set<char>> space2ids;
    for (const auto& kv : m_id2area) {
        auto& area = kv.second;
        int d = PUZ_PIECE_SIZE - area.m_inner.size();
        for (auto& p : area.m_outer) {
            list<puz_state2> smoves;
            puz_move_generator<puz_state2>::gen_moves({*this, p, d}, smoves);
            for (auto& p2 : smoves)
                space2ids[p2].insert(kv.first);
        }
    }
    if (space2ids.size() != boost::count(*this, PUZ_SPACE))
        return 0;

    if (space2ids.empty())
        m_next = "";
    else {
        // find the tile reachable from the fewest number of fishing holes
        const auto& ids = boost::min_element(space2ids, [](
            const pair<const Position, set<char>>& kv1,
            const pair<const Position, set<char>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        })->second;
        m_next = string(ids.begin(), ids.end());
    }
    return 2;
}

void puz_state::make_move2(char id, Position p)
{
    auto& area = m_id2area[id];
    cells(p) = id;
    area.add_cell(p);
    ++m_distance;
}

bool puz_state::make_move(char id, const Position& p)
{
    m_distance = 0;
    make_move2(id, p);
    int m;
    while ((m = adjust_area(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (char ch : m_next)
        for (auto& p : m_id2area.at(ch).m_outer) {
            children.push_back(*this);
            if (!children.back().make_move(ch, p))
                children.pop_back();
        }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) == PUZ_BLOCK) {
                horz_walls.insert(p);
                horz_walls.insert(p + offset2[2]);
                vert_walls.insert(p);
                vert_walls.insert(p + offset2[1]);
            }
        }
    for (auto& kv : m_id2area) {
        auto& area = kv.second;
        for (auto& p : area.m_inner)
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (area.m_inner.count(p2) == 0)
                    walls.insert(p_wall);
            }
    }

    for (int r = 1;; ++r) {
        // draw horz-walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen() - 1) break;
            char ch = cells(p);
            out << (ch == PUZ_BLOCK ? ch :
                m_game->m_id2hole.at(ch) == p ? PUZ_HOLE : PUZ_SPACE
            );
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_NorthPoleFishing()
{
    using namespace puzzles::NorthPoleFishing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NorthPoleFishing.xml", "Puzzles/NorthPoleFishing.txt", solution_format::GOAL_STATE_ONLY);
}
