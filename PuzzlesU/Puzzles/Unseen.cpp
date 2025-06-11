#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 4/Unseen

    Summary
    Round the corner

    Description
    1. Divide the board to include one 'eye' (a number) in each region.
    2. The eye can see in all four directions up to region borders.
    3. The number tells you how many tiles the eye does NOT see in the region.
*/

namespace puzzles::Unseen{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '+';

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
    // 'eye' (a number) in the region
    int m_num;
    // all permutations (forms) of the region
    vector<set<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_region> m_pos2region;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const Position& p, int num)
        : m_game(&game), m_p(p), m_num(num) {
        make_move(p);
    }

    bool is_goal_state() const { return m_invisible == m_num; }
    bool make_move(const Position& p) {
        insert(p);
        int n = 1;
        for (auto& os : offset)
            for (auto p2 = m_p + os; contains(p2); p2 += os)
                ++n;
        return (m_invisible = size() - n) <= m_num;
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    Position m_p;
    int m_num;
    int m_invisible = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            if (contains(p2) || m_game->cells(p2) != PUZ_SPACE) continue;
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
            if (char ch = str[c - 1]; ch != ' ')
                m_start.push_back(ch_r),
                m_pos2region[{r, c}] = {ch_r++, ch - '0', {}};
            else
                m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, region] : m_pos2region) {
        puz_state2 sstart(*this, p, region.m_num);
        list<list<puz_state2>> spaths;
        puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
        for (auto& spath : spaths)
            region.m_perms.push_back(spath.back());
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
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
    // key: the position of the number
    // value.elem: the index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_start)
{
    for (auto& [p, region] : m_game->m_pos2region) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(region.m_perms.size());
        boost::iota(perm_ids, 0);
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    set<Position> rng;
    for (auto& [p, perm_ids] : m_matches) {
        auto& [ch, num, perms] = m_game->m_pos2region.at(p);
        boost::remove_erase_if(perm_ids, [&](int id) {
            return boost::algorithm::any_of(perms[id], [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 != PUZ_SPACE && ch2 != ch;
            });
        });
        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
        for (int id : perm_ids) {
            auto& perm = perms[id];
            rng.insert(perm.begin(), perm.end());
        }
    }
    return rng.size() == boost::count(m_cells, PUZ_SPACE) + m_matches.size() ? 2 : 0;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [ch, num, perms] = m_game->m_pos2region.at(p);
    for (auto& p2 : perms[n])
        cells(p2) = ch;
    m_matches.erase(p), ++m_distance;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, region_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : region_ids) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
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
        // draw horizontal walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            if (auto it = m_game->m_pos2region.find(p); it == m_game->m_pos2region.end())
                out << ' ';
            else
                out << it->second.m_num;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Unseen()
{
    using namespace puzzles::Unseen;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Unseen.xml", "Puzzles/Unseen.txt", solution_format::GOAL_STATE_ONLY);
}
