#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/Odds Are Even

    Summary
    This ... looks familiar ... I thought you didi't like Sudokus?

    Description
    1. Odds Are Even is a Sudoku variant where the hints are given by
       differently shaded tiles.
    2. Standard Sudoku rules apply, i.e. you need to fill the board with
       numbers 1 to 9 in every row, column and area, without repetitions.
    3. The brighter coloured tiles contain odd numbers (1,3,5,7,9) and
       the dim ones contain even numbers (2,4,6,8).

    Variation
    4. Further levels have irregular areas, instead of the classic 3*3 Sudoku.
       All the other rules are the same.
    5. Even further, you will find levels that have NO areas.
*/

namespace puzzles::OddsAreEven{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ROW_LINE = '|';
constexpr auto PUZ_COL_LINE = '-';
constexpr auto PUZ_ODD = 'O';
constexpr auto PUZ_EVEN = 'E';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_pos_info
{
    int m_area_id;
    string m_nums;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_bNoAreas;
    map<Position, char> m_horz_walls, m_vert_walls;
    vector<vector<Position>> m_areas;
    map<Position, puz_pos_info> m_pos2info;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
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
            // horizontal walls
            string_view str_h = strs[r * 2];
            for (int c = 0; c < m_sidelen; ++c)
                m_horz_walls[{r, c}] = str_h[c * 2 + 1];
            if (r == m_sidelen) break;
            string_view str_v = strs[r * 2 + 1];
            for (int c = 0;; ++c) {
                Position p(r, c);
                // vertical walls
                m_vert_walls[p] = str_v[c * 2];
                if (c == m_sidelen) break;
                rng.insert(p);
                m_areas[p.first].push_back(p);
                m_areas[m_sidelen + p.second].push_back(p);
                m_cells.push_back(str_v[c * 2 + 1]);
            }
        }
        for (int n = 0; !rng.empty(); ++n) {
            auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls,
                *rng.begin()});
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
        m_cells = boost::accumulate(strs, string());
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
            char ch = cells(p);
            if (isdigit(ch))
                m_pos2info[p].m_nums.push_back(ch);
            else
                m_pos2info[p].m_nums = ch == PUZ_ODD ? "13579" : "2468";
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, string> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& [p, info] : g.m_pos2info)
        m_pos2nums[p] = info.m_nums;

    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = g.cells(p);
            if (isdigit(ch))
                make_move(p, ch);
        }
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    auto& areas = m_game->m_areas;
    auto& info = m_game->m_pos2info.at(p);

    auto f = [&](const Position& p2, function<bool(char)> g) {
        if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end())
            boost::remove_erase_if(it->second, g);
    };
    vector<const vector<Position>*> area_ptrs = {&areas[p.first], &areas[sidelen() + p.second]};
    if (!m_game->m_bNoAreas)
        area_ptrs.emplace_back(&areas[info.m_area_id]);
    for (auto* area : area_ptrs)
        for (auto& p2 : *area)
            if (p2 != p)
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
    auto& [p, nums] = *boost::min_element(m_pos2nums, [](
        const pair<const Position, string>& kv1,
        const pair<const Position, string>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : nums)
        if (children.push_back(*this); !children.back().make_move(p, ch))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    if (!m_game->m_bNoAreas)
        for (int r = 0;; ++r) {
            // draw horizontal lines
            for (int c = 0; c < sidelen(); ++c)
                out << ' ' << m_game->m_horz_walls.at({r, c});
            println(out);
            if (r == sidelen()) break;
            for (int c = 0;; ++c) {
                Position p(r, c);
                // draw vertical lines
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

void solve_puz_OddsAreEven()
{
    using namespace puzzles::OddsAreEven;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/OddsAreEven.xml", "Puzzles/OddsAreEven.txt", solution_format::GOAL_STATE_ONLY);
}
