#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 14/The Odd Brick

    Summary
    Even Bricks are strange, sometimes

    Description
    1. On the board there is a wall, made of 2*1 and 1*1 bricks.
    2. Each 2*1 brick contains and odd and an even number, while 1*1 bricks
       can contain any number.
    3. Each row and column contains numbers 1 to N, where N is the side of
       the board.
*/

namespace puzzles::TheOddBrick{

#define PUZ_SPACE        ' '

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

struct puz_numbers : vector<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        for (int i = 0; i < num; ++i)
            push_back(i + '1');
    }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_brick_count;
    vector<vector<Position>> m_area_pos;
    puz_numbers m_numbers;
    map<Position, char> m_pos2num;
    map<Position, int> m_pos2brick;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2)
    , m_area_pos(m_sidelen * 2)
    , m_numbers(m_sidelen)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        m_area_pos.emplace_back();
        for (auto& p : smoves) {
            m_pos2brick[p] = n;
            m_area_pos.back().push_back(p);
            m_area_pos[p.first].push_back(p);
            m_area_pos[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }
    m_brick_count = m_area_pos.size() - m_sidelen * 2;
}

// first: the index of the area
// second : all char numbers used to fill a position
typedef pair<int, puz_numbers> puz_area;

struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int index, int sz, const puz_numbers& numbers) {
        for (int i = 0; i < sz; i++)
            emplace_back(index++, numbers);
    }
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, char ch);
    void remove_pair(const Position& p, char ch) {
        auto i = m_pos2nums.find(p);
        if (i != m_pos2nums.end())
            boost::remove_erase(i->second, ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::count(*this, PUZ_SPACE);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    puz_group m_grp_bricks;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
    map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
    , m_grp_bricks(g.m_sidelen * 2, g.m_brick_count, g.m_numbers)
    , m_grp_rows(0, g.m_sidelen, g.m_numbers)
    , m_grp_cols(g.m_sidelen, g.m_sidelen, g.m_numbers)
{
    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c)
            m_pos2nums[{r, c}] = g.m_numbers;

    for (auto& kv : g.m_pos2num)
        make_move(kv.first, kv.second);
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2nums.erase(p);

    auto f = [&](puz_area* a, char ch2) {
        for (auto& p2 : m_game->m_area_pos[a->first])
            remove_pair(p2, ch2);
    };
    array<puz_area*, 3> areas = {
        &m_grp_bricks[m_game->m_pos2brick.at(p)],
        &m_grp_rows[p.first],
        &m_grp_cols[p.second]
    };
    for (auto a : areas)
        f(a, ch);
    // Each 2*1 brick contains and odd and an even number
    auto a = areas[0];
    for (char ch2 : a->second)
        if ((ch - '0') % 2 == (ch2 - '0') % 2)
            f(a, ch2);

    return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, puz_numbers>& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_pos2nums, [](
        const pair<const Position, puz_numbers>& kv1,
        const pair<const Position, puz_numbers>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TheOddBrick()
{
    using namespace puzzles::TheOddBrick;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheOddBrick.xml", "Puzzles/TheOddBrick.txt", solution_format::GOAL_STATE_ONLY);
}
