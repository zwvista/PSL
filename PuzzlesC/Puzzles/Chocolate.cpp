#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 6/Chocolate

    Summary
    Yummy!

    Description
    1. Find some chocolate bars following these rules:
    2. A Chocolate bar is a rectangular or a square.
    3. Chocolate tiles form bars independently of the area borders.
    4. Chocolate bars must not be orthogonally adjacent.
    5. A tile with a number indicates how many tiles in the area must
       be chocolate.
    6. An area without number can have any number of tiles of chocolate.
*/

namespace puzzles::Chocolate{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_CHOCOLATE    'C'
#define PUZ_UNKNOWN      -1

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},         // n
    {0, 1},         // e
    {1, 0},         // s
    {0, 0},         // w
};

struct puz_area
{
    int m_num = PUZ_UNKNOWN;
    vector<Position> m_range;
    vector<int> m_boxids;
};
    
struct puz_box
{
    // top-left and bottom-right
    pair<Position, Position> m_box;
    map<int, int> m_area2num;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    vector<puz_box> m_boxes;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) { make_move(p_start); }

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
        if (walls.count(p_wall) == 0) {
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
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
            if (c == m_sidelen) break;
            rng.insert(p);
        }
    }
    
    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        auto& area = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            area.m_range.push_back(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                area.m_num = it->second;
        }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (int h = 1; h <= m_sidelen - r; ++h)
                for (int w = 1; w <= m_sidelen - c; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r, c), br = tl + box_sz;
                    map<int, int> area2num;
                    for (int r2 = tl.first; r2 <= br.first; ++r2)
                        for (int c2 = tl.second; c2 <= br.second; ++c2)
                            ++area2num[m_pos2area.at({r2, c2})];
                    if (boost::algorithm::all_of(area2num, [&](const pair<const int, int>& kv){
                        int num = m_areas[kv.first].m_num;
                        return num == PUZ_UNKNOWN || kv.second <= num;
                    })) {
                        int n = m_boxes.size();
                        auto& o = m_boxes.emplace_back();
                        o.m_box = {tl, br};
                        o.m_area2num = area2num;
                        for (auto&& [i, j] : area2num)
                            m_areas[i].m_boxids.push_back(n);
                    }
                }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
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

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the index of the area
    // value.elem: the index of the box
    map<int, vector<int>> m_matches;
    vector<int> m_area2num;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_area2num(g.m_areas.size(), PUZ_UNKNOWN)
{
    for (int i = 0; i < m_area2num.size(); ++i) {
        auto& area = g.m_areas[i];
        m_area2num[i] = area.m_num;
        if (area.m_num != 0 && area.m_num != PUZ_UNKNOWN)
            m_matches[i] = area.m_boxids;
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto f = [&](const Position& p) {
        if (!is_valid(p)) return false;
        char ch = cells(p);
        return ch != PUZ_SPACE && ch != PUZ_EMPTY;
    };
    
    for (auto& kv : m_matches) {
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            auto& o = m_game->m_boxes[id];
            auto& box = o.m_box;
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            for (int r = box.first.first; r <= box.second.first; ++r) {
                Position p1(r, box.first.second - 1), p2(r, box.second.second + 1);
                if (f(p1) || f(p2))
                    return true;
            }
            for (int c = box.first.second; c <= box.second.second; ++c) {
                Position p1(box.first.first - 1, c), p2(box.second.first + 1, c);
                if (f(p1) || f(p2))
                    return true;
            }
            return boost::algorithm::any_of(o.m_area2num, [&](const pair<const int, int>& kv){
                return kv.second > m_area2num[kv.first];
            });
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(box_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& o = m_game->m_boxes[n];
    auto& box = o.m_box;
    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c)
            cells({r, c}) = PUZ_CHOCOLATE;
    auto f = [&](const Position& p) {
        if (is_valid(p))
            cells(p) = PUZ_EMPTY;
    };
    for (int r = box.first.first; r <= box.second.first; ++r)
        f({r, box.first.second - 1}), f({r, box.second.second + 1});
    for (int c = box.first.second; c <= box.second.second; ++c)
        f({box.first.first - 1, c}), f({box.second.first + 1, c});
    for(auto&& [i, j] : o.m_area2num)
        if (int& n = m_area2num[i]; n != PUZ_UNKNOWN && (n -= j) == 0)
            m_matches.erase(i), ++m_distance;
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.count({r, c}) == 1 ? " --" : "   ");
        out << endl;
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ' ';
            else
                out << it->second;
            out << cells({r, c});
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Chocolate()
{
    using namespace puzzles::Chocolate;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Chocolate.xml", "Puzzles/Chocolate.txt", solution_format::GOAL_STATE_ONLY);
}
