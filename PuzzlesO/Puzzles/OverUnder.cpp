#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 15/Over Under

    Summary
    Over and Under regions

    Description
    1. Divide the board in regions following these rules:
    2. Each region must contain two numbers.
    3. The region size must be between the two numbers.
*/

namespace puzzles::OverUnder{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_WALL         'W'
#define PUZ_BOUNDARY     '+'

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

struct puz_region
{
    // character that represents the region. 'a', 'b' and so on
    char m_name;
    // key: number of the tiles occupied by the region
    // value: all permutations (forms) of the region
    vector<set<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    // key: positions of the two numbers (hints)
    map<pair<Position, Position>, puz_region> m_pair2region;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const Position& p, const Position& p2, int num)
        : m_game(&game), m_p2(&p2), m_num(num) {make_move(p);}

    bool is_goal_state() const { return m_distance == m_num; }
    bool make_move(const Position& p) {
        insert(p); ++m_distance;
        return boost::algorithm::any_of(*this, [&](const Position& p2) {
            // cannot go too far away
            return manhattan_distance(p2, *m_p2) <= m_num - m_distance;
        });
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    const Position* m_p2;
    int m_num;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if (ch2 != PUZ_SPACE && p2 != *m_p2 || count(p2) != 0) continue;
            children.push_back(*this);
            if (!children.back().make_move(p2))
                children.pop_back();
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_r = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                int n = ch - '0';
                Position p(r, c);
                m_pos2num[p] = n;
                m_start.push_back(ch_r++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& kv : m_pos2num) {
        auto& p = kv.first;
        int n = kv.second;
        for (auto& kv2 : m_pos2num) {
            auto p2 = kv2.first;
            if (p2 <= p) continue;
            int n2 = kv2.second;
            int n3 = min(n, n2), n4 = max(n, n2);
            // The region size must be between the two numbers.
            // cannot make a pairing with a tile too far away
            if (n4 - n3 < 2 || manhattan_distance(p, p2) + 1 >= n4) continue;
            auto kv3 = make_pair(p, p2);
            auto& region = m_pair2region[kv3];
            region.m_name = cells(p);
            for (int i = n3 + 1; i < n4; ++i) {
                puz_state2 sstart(*this, p, p2, i);
                list<list<puz_state2>> spaths;
                puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
                for (auto& spath : spaths)
                    region.m_perms.push_back(spath.back());
            }
            if (region.m_perms.empty())
                m_pair2region.erase(kv3);
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, const Position& p2, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: position of a tile
    // value.0, value.1: positions of the two numbers (hints) from which
    //                   a region containing that tile can be formed
    // value.2: index into the permutations (forms) of the region
    map<Position, vector<tuple<Position, Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto& kv : g.m_pair2region) {
        auto &p = kv.first.first, &p2 = kv.first.second;
        auto& perms = kv.second.m_perms;
        for (int i = 0; i < perms.size(); i++)
            for (auto& p3 : perms[i])
                m_matches[p3].emplace_back(p, p2, i);
    }
}

bool puz_state::make_move(const Position& p, const Position& p2, int n)
{
    auto& region = m_game->m_pair2region.at({p, p2});
    auto& perm = region.m_perms[n];
    for (auto& p3 : perm) {
        cells(p3) = region.m_name;
        m_matches.erase(p3);
    }
    m_distance = perm.size();
    for (auto& p3 : perm)
        for (auto& kv : m_matches) {
            auto& v = kv.second;
            boost::remove_erase_if(v, [&](auto& t) {
                auto& p = get<0>(t);
                auto& p2 = get<1>(t);
                int id = get<2>(t);
                auto& perm = m_game->m_pair2region.at({p, p2}).m_perms[id];
                return perm.contains(p3);
            });
        }

    return boost::algorithm::none_of(m_matches, [&](auto& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](auto& kv1, auto& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& t : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(get<0>(t), get<1>(t), get<2>(t)))
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
        // draw horz-walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            auto it = m_game->m_pos2num.find(p);
            if (it == m_game->m_pos2num.end())
                out << ' ';
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_OverUnder()
{
    using namespace puzzles::OverUnder;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/OverUnder.xml", "Puzzles/OverUnder.txt", solution_format::GOAL_STATE_ONLY);
}
