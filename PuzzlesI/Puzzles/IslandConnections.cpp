#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 5/Island Connections

    Summary
    Simpler Bridges

    Description
    1. Connect the islands with bridges.
    2. All the islands must be connected between them, forming a single path.
    3. Bridges are horizontal or vertical straight lines and cannot cross each other.
    4. Islands with numbers tell you how many bridges depart from it.
    5. Islands without a number can have any number of bridges.
    6. Shaded islands cannot be connected and should be avoided.
*/

namespace puzzles::IslandConnections{

#define PUZ_SPACE             ' '
#define PUZ_ISLAND            'O'
#define PUZ_SHADED            'S'
#define PUZ_HORZ              '-'
#define PUZ_VERT              '|'
#define PUZ_BOUNDARY          'B'
#define PUZ_UNKNOWN           -1

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    // key: the position of the island
    // value.elem: the numbers of the bridges connected to the island
    //             in all the four directions
    map<Position, vector<vector<int>>> m_pos2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; ; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            switch (char ch = str[c - 1]) {
            case PUZ_SPACE:
            case PUZ_SHADED:
                m_start.push_back(ch);
                break;
            case PUZ_ISLAND:
                m_start.push_back(ch);
                m_pos2num[{r * 2 - 1, c * 2 - 1}] = PUZ_UNKNOWN;
                break;
            default:
                m_start.push_back(PUZ_ISLAND);
                m_pos2num[{r * 2 - 1, c * 2 - 1}] = ch - '0';
                break;
            }
            if (c == m_sidelen / 2) break;
            m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_start.push_back(PUZ_BOUNDARY);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto&& [p, sum] : m_pos2num) {
        auto& perms = m_pos2perms[p];
        
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            bool is_horz = i % 2 == 1;
            auto& os = offset[i];
            auto& nums = dir_nums[i];
            // none for the other cases
            nums = {0};
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_ISLAND)
                    // one bridge or none
                    nums = {0, 1};
                if (ch != PUZ_SPACE)
                    break;
            }
        }
        
        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3]) {
                        int sum2 = n0 + n1 + n2 + n3;
                        if (sum == PUZ_UNKNOWN && sum2 != 0 || sum == sum2)
                            perms.push_back({n0, n1, n2, n3});
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_connected() const;

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
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto&& [p, perms] : g.m_pos2perms) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;
        auto& perms = m_game->m_pos2perms.at(p);

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& nums = perms[id];
            for (int i = 0; i < 4; ++i) {
                int n = nums[i];
                auto& os = offset[i];
                bool is_horz = i % 2 == 1;
                auto p2 = p + os;
                if (n == 0) {
                    if (char ch = cells(p2); is_horz && ch == PUZ_HORZ || !is_horz && ch == PUZ_VERT)
                        return true;
                } else
                    for (; ; p2 += os) {
                        char ch = cells(p2);
                        if (ch == PUZ_ISLAND && m_matches.contains(p2) ||
                            is_horz && ch == PUZ_HORZ || !is_horz && ch == PUZ_VERT)
                            break;
                        if (ch != PUZ_SPACE)
                            return true;
                    }
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids[0]), 1;
            }
    }
    return is_connected() ? 2 : 0;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s);

    int sidelen() const { return m_state->sidelen(); }
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
    int i = s.m_cells.find(PUZ_ISLAND);
    make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto& os = offset[i];
        bool is_horz = i % 2 == 1;
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (is_horz && ch == PUZ_HORZ || !is_horz && ch == PUZ_VERT) {
            while (m_state->cells(p2) != PUZ_ISLAND)
                p2 += os;
            children.push_back(*this);
            children.back().make_move(p2);
        } else if (m_state->m_matches.contains(*this) && ch == PUZ_SPACE) {
            while (m_state->cells(p2) == PUZ_SPACE)
                p2 += os;
            if (m_state->cells(p2) == PUZ_ISLAND) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
    }
}

bool puz_state::is_connected() const
{
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    return smoves.size() == boost::count(m_cells, PUZ_ISLAND);
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_pos2perms.at(p)[n];
    for (int i = 0; i < 4; ++i) {
        bool is_horz = i % 2 == 1;
        auto& os = offset[i];
        if (int n2 = perm[i]; n2 != 0)
            for (auto p2 = p + os; ; p2 += os) {
                char& ch = cells(p2);
                if (ch != PUZ_SPACE) break;
                ch = is_horz ? PUZ_HORZ : PUZ_VERT;
            }
    }

    ++m_distance;
    m_matches.erase(p);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
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
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_ISLAND)
                out << format("{:2}", [&]{
                    int n = 0;
                    for (auto& os : offset) {
                        char ch = cells(p + os);
                        if (ch == PUZ_HORZ || ch == PUZ_VERT)
                            ++n;
                    }
                    return n;
                }());
            else
                out << ' ' << ch;
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_IslandConnections()
{
    using namespace puzzles::IslandConnections;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/IslandConnections.xml", "Puzzles/IslandConnections.txt", solution_format::GOAL_STATE_ONLY);
}
