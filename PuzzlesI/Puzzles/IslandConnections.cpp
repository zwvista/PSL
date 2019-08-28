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

namespace puzzles{ namespace IslandConnections{

#define PUZ_SPACE             ' '
#define PUZ_ISLAND            'N'
#define PUZ_HORZ              '-'
#define PUZ_VERT              '|'
#define PUZ_BOUNDARY          'B'

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

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_ISLAND);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, const vector<int>& perm);
    bool make_move2(const Position& p, const vector<int>& perm);
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
    // key: the position of the island
    // value.elem: the numbers of the bridges connected to the island
    //             in all four directions
    map<Position, vector<vector<int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    for (auto& kv : g.m_pos2num)
        m_matches[kv.first];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perms = kv.second;
        perms.clear();

        int sum = m_game->m_pos2num.at(p);
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            bool is_horz = i % 2 == 1;
            auto& os = offset[i];
            auto& nums = dir_nums[i];
            // none for the other cases
            nums = {0};
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_ISLAND && m_matches.count(p2) != 0)
                    // one, two bridges or none
                    nums = {0, 1, 2};
                else if ((ch == PUZ_HORZ || ch == PUZ_HORZ) && is_horz)
                    // already connected
                    nums = {ch == PUZ_HORZ ? 1 : 2};
                else if ((ch == PUZ_VERT || ch == PUZ_VERT) && !is_horz)
                    // already connected
                    nums = {ch == PUZ_VERT ? 1 : 2};
                if (ch != PUZ_SPACE)
                    break;
            }
        }

        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum)
                            perms.push_back({n0, n1, n2, n3});

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perms.front()) ? 1 : 0;
            }
    }
    return 2;
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
    int i = s.find(PUZ_ISLAND);
    make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto& os = offset[i];
        bool is_horz = i % 2 == 1;
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if (is_horz && (ch == PUZ_HORZ || ch == PUZ_HORZ) ||
            !is_horz && (ch == PUZ_VERT || ch == PUZ_VERT)) {
            while (m_state->cells(p2) != PUZ_ISLAND)
                p2 += os;
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_connected() const
{
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    return smoves.size() == boost::count(*this, PUZ_ISLAND);
}

bool puz_state::make_move2(const Position& p, const vector<int>& perm)
{
    for (int i = 0; i < 4; ++i) {
        bool is_horz = i % 2 == 1;
        auto& os = offset[i];
        int n = perm[i];
        if (n == 0)
            continue;
        for (auto p2 = p + os; ; p2 += os) {
            char& ch = cells(p2);
            if (ch != PUZ_SPACE)
                break;
            ch = is_horz ? n == 1 ? PUZ_HORZ : PUZ_HORZ
                : n == 1 ? PUZ_VERT : PUZ_VERT;
        }
    }

    ++m_distance;
    m_matches.erase(p);

    return !is_goal_state() || is_connected();
}

bool puz_state::make_move(const Position& p, const vector<int>& perm)
{
    m_distance = 0;
    if (!make_move2(p, perm))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<vector<int>>>& kv1,
        const pair<const Position, vector<vector<int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& perm : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, perm))
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
                out << format("%2d") % m_game->m_pos2num.at(p);
            else
                out << ' ' << ch;
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_IslandConnections()
{
    using namespace puzzles::IslandConnections;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/IslandConnections.xml", "Puzzles/IslandConnections.txt", solution_format::GOAL_STATE_ONLY);
}
