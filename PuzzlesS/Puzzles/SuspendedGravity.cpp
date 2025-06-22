#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/SuspendedGravity

    Summary
    Falling Blocks

    Description
    1. Each region contains the number of stones, which can be indicated by a number.
    2. Regions without a number contain at least one stone.
    3. Stones inside a region are all connected either vertically or horizontally.
    4. Stones in two adjacent regions cannot touch horizontally or vertically.
    5. Lastly, if we apply gravity to the puzzle and the stones fall down to
       the bottom of the board they fit together exactly and cover the bottom
       half of the board.
    6. Think "Tetris": all the blocks will fall as they are
       (they won't break into single stones)
*/

namespace puzzles::SuspendedGravity{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_STONE = 'O';
constexpr auto PUZ_UNKNOWN = -1;

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

struct puz_area
{
    int m_num = PUZ_UNKNOWN;
    vector<Position> m_range;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    Position m_size;
    map<Position, int> m_pos2num;
    vector<puz_area> m_areas;
    map<Position, int> m_pos2area;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
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
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2, strs[0].length() / 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < cols(); ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == rows()) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == cols()) break;
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        auto& area = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            rng.erase(p);
            area.m_range.push_back(p);
            if (auto it = m_pos2num.find(p); it != m_pos2num.end())
                area.m_num = it->second;
        }
    }

    for (auto& area : m_areas) {
        auto& [num, rng, perms] = area;
        int sz = rng.size();
        if (!perms.empty()) continue;
        for (int i = 1; i <= sz; ++i) {
            // 2. Regions without a number contain at least one stone.
            if (num != PUZ_UNKNOWN && num != i)
                continue;
            // 1. Each region contains the number of stones, which can be indicated by a number.
            auto perm = string(sz - i, PUZ_EMPTY) + string(i, PUZ_STONE);
            do {
                // 3. Stones inside a region are all connected either vertically or horizontally.
                set<Position> rng2;
                for (int j = 0; j < sz; ++j)
                    if (perm[j] == PUZ_STONE)
                        rng2.insert(rng[j]);
                auto smoves = puz_move_generator<puz_state3>::gen_moves(rng2);
                if (smoves.size() != rng2.size())
                    continue;

                // 5. Lastly, if we apply gravity to the puzzle and the stones fall down to
                // the bottom of the board they fit together exactly and cover the bottom
                // half of the board.
                vector<int> v;
                v.resize(cols());
                for (auto& p : rng2)
                    v[p.second]++;
                if (boost::algorithm::any_of(v, [&](int n) {
                    return n > rows() / 2;
                }))
                    continue;

                perms.push_back(perm);
            } while (boost::next_permutation(perm));
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);
    bool check_gravity();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return m_matches.size();}
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the index of the area
    // value.elem: the index of the permutations
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g)
    , m_cells(g.rows() * g.cols(), PUZ_SPACE)
{
    for (int i = 0; i < g.m_areas.size(); ++i) {
        vector<int> perm_ids(g.m_areas[i].m_perms.size());
        boost::iota(perm_ids, 0);
        m_matches[i] = perm_ids;
    }
}

int puz_state::find_matches(bool init)
{
    for (auto& [i, perm_ids] : m_matches) {
        auto& [num, rng, perms] = m_game->m_areas[i];

        string chars;
        for (auto& p : rng)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int j) {
            auto& perm = perms[j];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // Stones in two adjacent regions cannot touch horizontally or vertically.
            for (int k = 0; k < perm.size(); ++k) {
                auto& p = rng[k];
                char ch = perm[k];
                if (ch == PUZ_EMPTY) continue;
                for (auto& os : offset)
                    if (auto p2 = p + os; is_valid(p2) && m_game->m_pos2area.at(p2) != i && cells(p2) == ch)
                        return true;
            }
            return false;
        });

        if (!init)
            switch (perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(i, perm_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& [num, rng, perms] = m_game->m_areas[i];
    auto& perm = perms[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(rng[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
    return check_gravity();
}

// 5. Lastly, if we apply gravity to the puzzle and the stones fall down to
// the bottom of the board they fit together exactly and cover the bottom
// half of the board.
bool puz_state::check_gravity()
{
    auto cellsTemp = m_cells;

    // key: index of the area
    // value.elem: position of the stone
    map<int, set<Position>> area2stones;
    // key: index of the area where stones should fall later
    // value.elem: index of the area where stones should fall sooner
    map<int, set<int>> area2areas;
    for (int c = 0; c < cols(); ++c) {
        int n1 = -1;
        for (int r = 0; r < rows(); ++r) {
            Position p(r, c);
            if (cells(p) != PUZ_STONE) continue;
            int n2 = m_game->m_pos2area.at(p);
            area2stones[n2].insert(p);
            if (n1 == -1)
                n1 = n2;
            else if (n1 != n2) {
                area2areas[n1].insert(n2);
                n1 = n2;
            }
        }
    }

    // make the stones fall down
    while (!area2stones.empty()) {
        for (auto& [i, stones] : area2stones) {
            if (area2areas.contains(i)) continue;

            int j = 0;
            for (;; ++j) {
                for (auto& [r, c] : stones) {
                    Position p2(r + j + 1, c);
                    if (!stones.contains(p2) && (!is_valid(p2) || cells(p2) == PUZ_STONE))
                        goto cantfall;
                }
            }
        cantfall:
            if (j > 0) {
                for (auto& p : stones)
                    cells(p) = PUZ_EMPTY;
                for (auto& [r, c] : stones)
                    cells({r + j, c}) = PUZ_STONE;
            }

            for (auto it = area2areas.begin(); it != area2areas.end();) {
                auto& v = it->second;
                v.erase(i);
                if (v.empty())
                    it = area2areas.erase(it);
                else
                    it++;
            }
            area2stones.erase(i);
            break;
        }
    }

    // After falling down, they fit together exactly and
    // cover the bottom half of the board.
    for (int c = 0; c < cols(); ++c) {
        int r = 0;
        for (; r < rows() / 2; ++r)
            if (cells({r, c}) == PUZ_STONE)
                return false;
        bool hasStone = false;
        for (; r < rows(); ++r)
            if (cells({r, c}) == PUZ_STONE)
                hasStone = true;
            else if (hasStone)
                return false;
    }
    m_cells = cellsTemp;
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
    auto& [i, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(i, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == rows()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_SuspendedGravity()
{
    using namespace puzzles::SuspendedGravity;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/SuspendedGravity.xml", "Puzzles/SuspendedGravity.txt", solution_format::GOAL_STATE_ONLY);
}
