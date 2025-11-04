#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/Bridges

    Summary
    Enough Sudoku, let's build!

    Description
    1. The board represents a Sea with some islands on it.
    2. You must connect all the islands with Bridges, making sure every
       island is connected to each other with a Bridges path.
    3. The number on each island tells you how many Bridges are touching
       that island.
    4. Bridges can only run horizontally or vertically and can't cross
       each other.
    5. Lastly, you can connect two islands with either one or two Bridges
       (or none, of course)
*/

namespace puzzles::Bridges{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ISLAND = 'N';
constexpr auto PUZ_HORZ_1 = '-';
constexpr auto PUZ_VERT_1 = '|';
constexpr auto PUZ_HORZ_2 = '=';
constexpr auto PUZ_VERT_2 = 'H';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;
    // key: the position of the island
    // value.elem: the numbers of the bridges connected to the island
    //             in all four directions
    map<Position, vector<vector<int>>> m_pos2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(ch);
            else {
                m_cells.push_back(PUZ_ISLAND);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, sum] : m_pos2num) {
        auto& perms = m_pos2perms[p];

        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            auto& nums = dir_nums[i];
            // none for the other cases
            nums = {0};
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_ISLAND)
                    // one, two bridges or none
                    nums = {0, 1, 2};
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
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    for (auto& [p, perms] : g.m_pos2perms) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(perms.size());
        boost::iota(perm_ids, 0);
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& perms = m_game->m_pos2perms.at(p);

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& nums = perms[id];
            for (int i = 0; i < 4; ++i) {
                int n = nums[i];
                auto& os = offset[i];
                bool is_horz = i % 2 == 1;
                auto p2 = p + os;
                if (n == 0) {
                    if (char ch = cells(p2); is_horz && (ch == PUZ_HORZ_1 || ch == PUZ_HORZ_2) ||
                        !is_horz && (ch == PUZ_VERT_1 || ch == PUZ_VERT_2))
                        return true;
                } else
                    for (; ; p2 += os) {
                        char ch = cells(p2);
                        if (ch == PUZ_ISLAND && m_matches.contains(p2) ||
                            is_horz && (ch == PUZ_HORZ_1 && n == 1 || ch == PUZ_HORZ_2 && n == 2) ||
                            !is_horz && (ch == PUZ_VERT_1 && n == 1 || ch == PUZ_VERT_2 && n == 2))
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
    return is_interconnected() ? 2 : 0;
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
        if (is_horz && (ch == PUZ_HORZ_1 || ch == PUZ_HORZ_2) ||
            !is_horz && (ch == PUZ_VERT_1 || ch == PUZ_VERT_2)) {
            while (m_state->cells(p2) != PUZ_ISLAND)
                p2 += os;
            children.emplace_back(*this).make_move(p2);
        } else if (m_state->m_matches.contains(*this) && ch == PUZ_SPACE) {
            while (m_state->cells(p2) == PUZ_SPACE)
                p2 += os;
            if (m_state->cells(p2) == PUZ_ISLAND)
                children.emplace_back(*this).make_move(p2);
        }
    }
}

bool puz_state::is_interconnected() const
{
    auto smoves = puz_move_generator<puz_state2>::gen_moves(*this);
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
                ch = is_horz ? n2 == 1 ? PUZ_HORZ_1 : PUZ_HORZ_2
                    : n2 == 1 ? PUZ_VERT_1 : PUZ_VERT_2;
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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_ISLAND)
                out << format("{:2}", m_game->m_pos2num.at(p));
            else
                out << ' ' << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Bridges()
{
    using namespace puzzles::Bridges;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Bridges/Bridges.xml", "Puzzles/Bridges/Bridges.txt", solution_format::GOAL_STATE_ONLY);
}

void solve_puz_BridgesTest()
{
    using namespace puzzles::Bridges;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, false, true>>(
        "../Test.xml", "../Test.txt", solution_format::GOAL_STATE_ONLY);
}
