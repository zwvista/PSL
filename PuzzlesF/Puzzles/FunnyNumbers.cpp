#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 2/Funny Numbers

    Summary
    Hahaha ... haha ... ehm ...

    Description
    1. Fill each region with numbers 1 to X where the X is the region area.
    2. Same numbers can't touch each other horizontally or vertically across regions.
    3. The numbers outside tell you the sum of the row or column.
*/

namespace puzzles::FunnyNumbers{

constexpr auto PUZ_WATER = 'X';
constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},         // n
    {0, 1},         // e
    {1, 0},         // s
    {0, 0},         // w
};

struct puz_numbers : set<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        for (int i = 0; i < num; ++i)
            insert(i + '1');
    }
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2num;
    map<int, int> m_area2num;
    vector<vector<Position>> m_area_pos;
    map<Position, int> m_pos2area;
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
        if (!walls.contains(p_wall) && p.first >= m_p_start.first) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 - 1)
    , m_area_pos(m_sidelen * 2)
{
    set<Position> rng;
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
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch;
            rng.insert(p);
        }
    }

    auto f = [&](int area_id, string s) {
        if (s != "  ")
            m_area2num[area_id] = stoi(s);
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1].substr(m_sidelen * 2 + 1, 2));
        f(i + m_sidelen, strs[m_sidelen * 2 + 1].substr(i * 2, 2));
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_area_pos.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            m_area_pos.back().push_back(p);
            m_area_pos[p.first].push_back(p);
            m_area_pos[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_area2num, 0,
            [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, int> m_area2num;
    // value.elem: index of the water
    vector<int> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_area2num(g.m_area2num)
{
    boost::iota(m_matches, 0);
}

bool puz_state::make_move(int n)
{
    m_distance = 0;

    return is_goal_state() || !m_matches.empty();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int n : m_matches) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
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

void solve_puz_FunnyNumbers()
{
    using namespace puzzles::FunnyNumbers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FunnyNumbers.xml", "Puzzles/FunnyNumbers.txt", solution_format::GOAL_STATE_ONLY);
}
