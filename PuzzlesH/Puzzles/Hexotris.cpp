#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 4/Hexotris

    Summary
    Longer Tetris

    Description
    1. Divide the board in Hexominos or regions of 6 tiles, of any shape.
    2. Each Hexomino contains two numbers. Each number tells you how many
       tiles of the region are orthogonally adjacent to that tile.
*/

namespace puzzles::Hexotris{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};
constexpr Position offset2[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

struct puz_region
{
    // character that represents the region. 'a', 'b' and so on
    char m_name;
    // position of the hints
    Position m_p1, m_p2;
    // all permutations (forms) of the region
    set<Position> m_perm;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_region> m_regions;
    map<Position, vector<int>> m_pos2region_ids;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, int num, const Position& p, const Position& p2)
        : m_game(&game), m_num(num), m_p(&p), m_p2(&p2) { make_move(p); }

    bool is_goal_state() const { return m_distance == m_num; }
    bool make_move(const Position& p);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_num;
    const Position* m_p;
    const Position* m_p2;
    int m_distance = 0;
};

bool puz_state2::make_move(const Position& p)
{
    insert(p); ++m_distance;
    // cannot go too far away
    return boost::algorithm::any_of(*this, [&](const Position& p2) {
        return manhattan_distance(p2, *m_p2) <= m_num - m_distance;
    }) && (!is_goal_state() || boost::algorithm::all_of(vector{m_p, m_p2},
        [&](const Position* ptr) {
        // 2. Each number tells you how many tiles of the region
        // are orthogonally adjacent to that tile.
        return boost::count_if(offset, [&](const Position& os) {
            return contains(*ptr + os);
        }) == m_game->m_pos2num.at(*ptr);
    }));
}

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // Regions extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile cannot be occupied by the region
            // if it is a space tile and has not been occupied by the region and
            if ((ch2 == PUZ_SPACE || p2 == *m_p2) && !contains(p2)) {
                children.push_back(*this);
                if (!children.back().make_move(p2))
                    children.pop_back();
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_g = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                int n = ch - '0';
                Position p(r, c);
                m_pos2num[p] = n;
                m_start.push_back(ch_g++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, n] : m_pos2num)
        for (auto& [p2, n2] : m_pos2num) {
            // only make a pairing with a tile greater than itself
            if (p2 <= p) continue;
            int n3 = 6;
            // cannot make a pairing with a tile too far away
            if (manhattan_distance(p, p2) + 1 > n3) continue;
            auto kv3 = pair{p, p2};
            puz_state2 sstart(*this, n3, p, p2);
            list<list<puz_state2>> spaths;
            // Regions can have any form.
            puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
            // save all goal states as permutations
            // A goal state is a region formed from two numbers
            for (auto& spath : spaths) {
                int n = m_regions.size();
                set<Position> perm = spath.back();
                m_regions.push_back({cells(p), p, p2, perm});
                for (auto& p : perm)
                    m_pos2region_ids[p].push_back(n);
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

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: position of the board
    // value.elem: index of the region
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
, m_matches(g.m_pos2region_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, v] : m_matches) {
        // remove any path if it contains a tile which belongs to another region
        boost::remove_erase_if(v, [&](int region_id) {
            auto& [_1, p, p2, perm] = m_game->m_regions[region_id];
            return boost::algorithm::any_of(perm, [&](const Position& p3) {
                return p3 != p && p3 != p2 && cells(p3) != PUZ_SPACE;
            });
        });

        if (!init)
            switch (v.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(v[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [name, _1, _2, perm] = m_game->m_regions[n];
    for (auto& p : perm) {
        cells(p) = name;
        ++m_distance;
        m_matches.erase(p);
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
    auto& [p, v] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& n : v) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (cells(p) != cells(p2))
                    walls.insert(p_wall);
            }
        }

    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (vert_walls.contains({r, c}) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            //if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
            //    out << " .";
            //else
            //    out << format("{:2}", it->second);
            out << cells(p) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Hexotris()
{
    using namespace puzzles::Hexotris;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Hexotris.xml", "Puzzles/Hexotris.txt", solution_format::GOAL_STATE_ONLY);
}
