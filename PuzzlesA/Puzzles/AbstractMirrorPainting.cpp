#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 4/Abstract Mirror Painting

    Summary
    Aliens, move over, the Next Trend is here!

    Description
    1. Diagonal mirrors are out, the new trend is orthogonal mirror abstract painting!
    2. You should paint areas that span two adjacent regions. The area is symmetrical with respect
       to the regions border.
    3. Numbers tell you how many tiles in that region are painted.
    4. Areas can't touch orthogonally.
*/

namespace puzzles::AbstractMirrorPainting{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';

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

constexpr string_view dirs = "^>v<";

struct puz_move
{
    int m_area_id1, m_area_id2;
    Position m_entrance1, m_entrance2;
    char m_dir1, m_dir2;
    set<Position> m_empties;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : the index of the area
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    map<Position, char> m_pos2dir;
    set<Position> m_horz_walls, m_vert_walls;
    vector<puz_move> m_moves;
    set<pair<int, int>> m_area_pairs;
    map<int, vector<int>> m_area2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_horz_walls, * m_vert_walls;
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
            m_cells.push_back(ch);
            if (ch != PUZ_SPACE)
                m_pos2dir[p] = ch;
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_areas.push_back({smoves.begin(), smoves.end()});
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
        }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            int area_id1 = m_pos2area.at(p);
            for (auto& os : {offset[1], offset[2]})
                if (auto p2 = p + os; is_valid(p2))
                    if (int area_id2 = m_pos2area.at(p2); area_id1 != area_id2)
                        m_area_pairs.insert({min(area_id1, area_id2), max(area_id1, area_id2)});
        }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            // 1. Each neighbourhood contains one entrance to the AbstractMirrorPainting.
            int area_id1 = m_pos2area.at(p);
            for (int i : {1, 2}) {
                auto& os = offset[i];
                vector<Position> rng;
                for (auto p2 = p + os; is_valid(p2); p2 += os)
                    rng.push_back(p2);
                if (rng.size() < 2) continue;
                for (auto it = rng.begin() + 1; it != rng.end(); ++it)
                    // 2. For each entrance there is a corresponding entrance in a different neighbourhood.
                    // 5. Two corresponding entrances cannot be in adjacent neighbourhood, i.e.
                    // there must be at least one neighbourhood between them.
                    if (int area_id2 = m_pos2area.at(*it);
                        area_id1 != area_id2 &&
                        !m_area_pairs.contains({min(area_id1, area_id2), max(area_id1, area_id2)})) {
                        int n = m_moves.size();
                        auto& [a1, a2, e1, e2, d1, d2, empties] = m_moves.emplace_back();
                        a1 = area_id1, a2 = area_id2;
                        e1 = p, e2 = *it;
                        // 3. The arrows of two corresponding entrances must point to each other.
                        d1 = dirs[i], d2 = dirs[(i + 2) % 4];
                        // 4. Between two corresponding entrances there cannot be any other entrance.
                        empties.insert(rng.begin(), it);
                        empties.insert_range(m_areas[area_id1]);
                        empties.insert_range(m_areas[area_id2]);
                        empties.erase(e1), empties.erase(e2);
                        m_area2move_ids[area_id1].push_back(n);
                        m_area2move_ids[area_id2].push_back(n);
                    }
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
    bool make_move(int move_id);
    void make_move2(int move_id);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
, m_matches(g.m_area2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_2, _3, e1, e2, d1, d2, empties] = m_game->m_moves[id];
            char ch1 = cells(e1), ch2 = cells(e2);
            return ch1 != PUZ_SPACE && ch1 != d1 ||
                ch2 != PUZ_SPACE && ch2 != d2 ||
                boost::algorithm::any_of(empties, [&](const Position& p) {
                    char ch3 = cells(p);
                    return ch3 != PUZ_SPACE && ch3 != PUZ_EMPTY;
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

void puz_state::make_move2(int move_id)
{
    auto& [a1, a2, e1, e2, d1, d2, empties] = m_game->m_moves[move_id];
    cells(e1) = d1, cells(e2) = d2;
    for (auto& p : empties)
        cells(p) = PUZ_EMPTY;
    m_distance += 2, m_matches.erase(a1), m_matches.erase(a2);
}

bool puz_state::make_move(int move_id)
{
    m_distance = 0;
    make_move2(move_id);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& move_id : move_ids)
        if (children.push_back(*this); !children.back().make_move(move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
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
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_AbstractMirrorPainting()
{
    using namespace puzzles::AbstractMirrorPainting;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/AbstractMirrorPainting.xml", "Puzzles/AbstractMirrorPainting.txt", solution_format::GOAL_STATE_ONLY);
}
