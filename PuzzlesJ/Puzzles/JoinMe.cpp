#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 2/Join Me!

    Summary
    Communicating Vessels

    Description
    1. Connect the different patches with one stitch (more in later levels).
    2. The numbers on the outside tell you how many stitches you can see from
       there in the row/column.
    3. A cell can contain only one stitch.
    4. Later levels will show you in the top right how many stitches you have
       to put between patches.
*/

namespace puzzles::JoinMe{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_HORZ_STITCH = '=';
constexpr auto PUZ_VERT_STITCH = 'I';
constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_stitch
{
    Position m_p1, m_p2;
    map<int, int> m_area2num;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_num_stitches;
    map<int, int> m_area2num;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    map<pair<int, int>, int> m_patch2area;
    set<Position> m_horz_walls, m_vert_walls;
    vector<puz_stitch> m_stitches;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
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
    , m_num_stitches(level.attribute("Stitches").as_int())
    , m_areas(m_sidelen * 2)
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
            rng.insert(p);
        }
    }

    auto f = [&](int area_id, string_view s) {
        m_area2num[area_id] = s == "  " ? PUZ_UNKNOWN : stoi(string(s));
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1].substr(m_sidelen * 2 + 1, 2));
        f(i + m_sidelen, strs[m_sidelen * 2 + 1].substr(i * 2, 2));
    }

    for (int n = m_areas.size(); !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& rng2 = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng2.push_back(p);
            m_areas[p.first].push_back(p);
            m_areas[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }

    int next_area_id = m_areas.size();
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p1(r, c);
            for (auto& os : {offset[1], offset[2]})
                if (auto p2 = p1 + os; is_valid(p2))
                    if (int n1 = m_pos2area.at(p1), n2 = m_pos2area.at(p2); n1 != n2) {
                        int area_id = [&] {
                            pair patch = {min(n1, n2), max(n1, n2)};
                            if (auto it = m_patch2area.find(patch); it != m_patch2area.end())
                                return it->second;
                            int id = next_area_id++;
                            m_patch2area[patch] = id;
                            m_area2num[id] = m_num_stitches;
                            return id;
                        }();
                        map<int, int> area2num;
                        area2num[area_id] = 1;
                        for (auto& p3 : {p1, p2})
                            area2num[p3.first]++, area2num[p3.second + m_sidelen]++;
                        m_stitches.emplace_back(p1, p2, area2num);
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
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const {return m_distance;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_area2num;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_area2num(g.m_area2num)
{
    for (int i = 0; i < g.m_stitches.size(); ++i)
        for (auto& [area_id, _1] : g.m_stitches[i].m_area2num)
            if (g.m_area2num.at(area_id) != PUZ_UNKNOWN)
                m_matches[area_id].push_back(i);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, stitch_ids] : m_matches) {
        boost::remove_erase_if(stitch_ids, [&](int id) {
            auto& [p1, p2, area2num] = m_game->m_stitches[id];
            return cells(p1) != PUZ_SPACE || cells(p2) != PUZ_SPACE ||
                !boost::algorithm::all_of(area2num, [&](const pair<const int, int>& kv) {
                    auto& [area_id, num] = kv;
                    int num2 = m_area2num[area_id];
                    return num2 == PUZ_UNKNOWN || num <= num2;
                });
        });

        if (!init)
            switch(stitch_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(stitch_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [p1, p2, area2num] = m_game->m_stitches[n];
    char ch = p1.first == p2.first ? PUZ_HORZ_STITCH : PUZ_VERT_STITCH;
    cells(p1) = cells(p2) = ch;
    for (auto& [area_id, num] : area2num)
        if (int& num2 = m_area2num[area_id];
            num2 != PUZ_UNKNOWN && (num2 -= num) == 0)
            ++m_distance, m_matches.erase(area_id);
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
    auto& [_1, stitch_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : stitch_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int area_id) {
        auto& rng = m_game->m_areas[area_id];
        int sum = boost::accumulate(rng, 0, [&](int acc, const Position& p) {
            return acc + (cells(p) == PUZ_SPACE ? 0 : 1);
        });
        out << format("{:2}", sum);
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

void solve_puz_JoinMe()
{
    using namespace puzzles::JoinMe;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/JoinMe.xml", "Puzzles/JoinMe.txt", solution_format::GOAL_STATE_ONLY);
}
