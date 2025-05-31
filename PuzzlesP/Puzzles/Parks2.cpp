#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 1/Parks

    Summary
    Put one Tree in each Park, row and column.(two in bigger levels)

    Description
    1. In Parks, you have many differently coloured areas(Parks) on the board.
    2. The goal is to plant Trees, following these rules:
    3. A Tree can't touch another Tree, not even diagonally.
    4. Each park must have exactly ONE Tree.
    5. There must be exactly ONE Tree in each row and each column.
    6. Remember a Tree CANNOT touch another Tree diagonally,
       but it CAN be on the same diagonal line.
    7. Larger puzzles have TWO Trees in each park, each row and each column.
*/

namespace puzzles::Parks2{

constexpr auto PUZ_TREE = 'T';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';

constexpr Position offset[] = {
    {-1, 0},    // n
    {-1, 1},    // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},    // sw
    {0, -1},    // w
    {-1, -1},    // nw
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_area_info
{
    vector<Position> m_range;
    vector<string> m_perms;
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    int m_tree_count_area;
    int m_tree_total_count;
    string m_start;
    map<Position, int> m_pos2park;
    vector<puz_area_info> m_area_info;
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
        auto p = *this + offset[i * 2];
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
, m_tree_count_area(level.attribute("TreesInEachArea").as_int(1))
, m_tree_total_count(m_tree_count_area * m_sidelen)
, m_area_info(m_sidelen * 3)
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
            m_start.push_back(ch);
            if (ch != PUZ_EMPTY) {
                rng.insert(p);
                m_area_info[r].m_range.push_back(p);
                m_area_info[c + m_sidelen].m_range.push_back(p);
            }
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        for (auto& p : smoves) {
            m_pos2park[p] = n;
            m_area_info[n + m_sidelen * 2].m_range.push_back(p);
            rng.erase(p);
        }
    }

    map<int, vector<string>> sz2perms;
    for (auto& info : m_area_info) {
        int ps_cnt = info.m_range.size();
        auto& perms = sz2perms[ps_cnt];
        if (perms.empty()) {
            auto perm = string(ps_cnt - m_tree_count_area, PUZ_EMPTY)
                + string(m_tree_count_area, PUZ_TREE);
            do
                perms.push_back(perm);
            while (boost::next_permutation(perm));
        }
        for (const auto& perm : perms) {
            vector<Position> ps_tree;
            for (int i = 0; i < perm.size(); ++i)
                if (perm[i] == PUZ_TREE)
                    ps_tree.push_back(info.m_range[i]);

            if ([&]{
                // no touching
                for (const auto& ps1 : ps_tree)
                    for (const auto& ps2 : ps_tree)
                        if (boost::algorithm::any_of_equal(offset, ps1 - ps2))
                            return false;
                return true;
            }())
                info.m_perms.push_back(perm);
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (int i = 0; i < sidelen() * 3; ++i)
        m_matches[i];

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, v] : m_matches) {
        auto& info = m_game->m_area_info[p];
        string area;
        for (auto& p : info.m_range)
            area.push_back(cells(p));

        v.clear();
        for (int i = 0; i < info.m_perms.size(); ++i)
            if (boost::equal(area, info.m_perms[i], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                v.push_back(i);

        if (!init)
            switch(v.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, v.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& info = m_game->m_area_info[i];
    auto& area = info.m_range;
    auto& perm = info.m_perms[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = area[k];
        if ((cells(p) = perm[k]) != PUZ_TREE) continue;

        // no touching
        for (auto& os : offset) {
            auto p2 = p + os;
            if (is_valid(p2))
                cells(p2) = PUZ_EMPTY;
        }
    }

    ++m_distance;
    m_matches.erase(i);
    return true;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if (!make_move2(i, j))
        return false;
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
        if (!children.back().make_move(kv.first, n))
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

void solve_puz_Parks2()
{
    using namespace puzzles::Parks2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Parks.xml", "Puzzles/Parks2.txt", solution_format::GOAL_STATE_ONLY);
}
