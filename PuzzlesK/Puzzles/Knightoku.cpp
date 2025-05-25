#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 8/Knightoku

    Summary
    I believe you two already met ??

    Description
    1. Knightoku is a Sudoku variant which plays mostly like Sudoku with
       one glaring exception.
    2. Standard Sudoku rules apply, i.e. you need to fill the board with
       numbers 1 to 9 in every row, column and area, without repetitions.
    3. Two tiles in a chess-like Knight jump configuration (i.e. L-shaped)
       can't contain the same number.

    Variation
    4. Further levels have irregular areas, instead of the classic 3*3 Sudoku.
       All the other rules are the same.
    5. Even further, you will find levels that have NO areas.
*/

namespace puzzles::Knightoku{

#define PUZ_SPACE        ' '
#define PUZ_ROW_LINE    '|'
#define PUZ_COL_LINE    '-'

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

const Position offset_knight[] = {
    {-2, -1},    // nw
    {-2, 1},    // ne
    {-1, 2},    // en
    {1, 2},        // es
    {2, -1},    // sw
    {2, 1},        // se
    {-1, -2},    // wn
    {1, -2},    // ws
};

struct puz_pos_info
{
    int m_area_id;
    vector<Position> m_knight_jumps;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_bNoAreas;
    map<Position, char> m_horz_walls, m_vert_walls;
    vector<vector<Position>> m_areas;
    map<Position, puz_pos_info> m_pos2info;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const map<Position, char>& horz_walls, const map<Position, char>& vert_walls,
        const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const map<Position, char> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        char ch = walls.at(p_wall);
        if (ch == PUZ_SPACE) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(9)
    , m_bNoAreas(level.attribute("NoAreas").as_int() == 1)
    , m_areas(m_bNoAreas ? 18 : 27)
{
    if (!m_bNoAreas) {
        set<Position> rng;
        for (int r = 0;; ++r) {
            // horz-walls
            auto& str_h = strs[r * 2];
            for (int c = 0; c < m_sidelen; ++c)
                m_horz_walls[{r, c}] = str_h[c * 2 + 1];
            if (r == m_sidelen) break;
            auto& str_v = strs[r * 2 + 1];
            for (int c = 0;; ++c) {
                Position p(r, c);
                // vert-walls
                m_vert_walls[p] = str_v[c * 2];
                if (c == m_sidelen) break;
                rng.insert(p);
                m_areas[p.first].push_back(p);
                m_areas[m_sidelen + p.second].push_back(p);
                m_start.push_back(str_v[c * 2 + 1]);
            }
        }
        for (int n = 0; !rng.empty(); ++n) {
            list<puz_state2> smoves;
            puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls,
                *rng.begin()}, smoves);
            int id = m_sidelen * 2 + n;
            auto& area = m_areas[id];
            for (auto& p : smoves) {
                area.push_back(p);
                rng.erase(p);
                m_pos2info[p].m_area_id = id;
            }
            boost::sort(area);
        }
    } else {
        m_start = boost::accumulate(strs, string());
        for (int r = 0; r < m_sidelen; ++r)
            for (int c = 0; c < m_sidelen; ++c) {
                Position p(r, c);
                m_areas[p.first].push_back(p);
                m_areas[m_sidelen + p.second].push_back(p);
                m_pos2info[p].m_area_id = -1;
            }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            auto& info = m_pos2info[p];
            for (auto& os : offset_knight) {
                auto p2 = p + os;
                if (is_valid(p2))
                    info.m_knight_jumps.push_back(p2);
            }
        }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, char ch);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, string> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& kv : g.m_pos2info)
        m_pos2nums[kv.first] = "123456789";

    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c) {
            Position p(r, c);
            char ch = g.cells(p);
            if (ch != PUZ_SPACE)
                make_move(p, ch);
        }
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    auto& areas = m_game->m_areas;
    auto& info = m_game->m_pos2info.at(p);

    auto f = [&](const Position& p2, function<bool(char)> g) {
        auto it = m_pos2nums.find(p2);
        if (it != m_pos2nums.end())
            boost::remove_erase_if(it->second, g);
    };
    vector<const vector<Position>*> area_ptrs = {&areas[p.first], &areas[sidelen() + p.second]};
    if (!m_game->m_bNoAreas)
        area_ptrs.emplace_back(&areas[info.m_area_id]);
    for (auto* area : area_ptrs)
        for (auto& p2 : *area)
            if (p2 != p)
                f(p2, [ch](char ch2) { return ch2 == ch; });
    for (auto& p2 : info.m_knight_jumps)
        f(p2, [ch](char ch2) { return ch2 == ch; });

    m_pos2nums.erase(p);
    return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, string>& kv) {
        return kv.second.empty();
    }) && boost::algorithm::all_of(areas, [&](const vector<Position>& area) {
        set<char> nums;
        for (auto& p2 : area) {
            char ch2 = cells(p2);
            if (ch2 == PUZ_SPACE)
                for (char ch3 : m_pos2nums.at(p2))
                    nums.insert(ch3);
            else
                nums.insert(ch2);
        }
        return nums.size() == 9;
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_pos2nums, [](
        const pair<const Position, string>& kv1,
        const pair<const Position, string>& kv2) {
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
    if (!m_game->m_bNoAreas)
        for (int r = 0;; ++r) {
            // draw horz-walls
            for (int c = 0; c < sidelen(); ++c)
                out << ' ' << m_game->m_horz_walls.at({r, c});
            println(out);
            if (r == sidelen()) break;
            for (int c = 0;; ++c) {
                Position p(r, c);
                // draw vert-walls
                out << m_game->m_vert_walls.at(p);
                if (c == sidelen()) break;
                out << cells(p);
            }
            println(out);
        } else
        for (int r = 0; r < sidelen(); ++r) {
            for (int c = 0; c < sidelen(); ++c)
                out << cells({r, c}) << ' ';
            println(out);
        }
    return out;
}

}

void solve_puz_Knightoku()
{
    using namespace puzzles::Knightoku;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Knightoku.xml", "Puzzles/Knightoku.txt", solution_format::GOAL_STATE_ONLY);
}
