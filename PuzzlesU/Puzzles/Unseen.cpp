#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 4/Unseen

    Summary
    Round the corner

    Description
    1. Divide the board to include one 'eye' (a number) in each region.
    2. The eye can see in all four directions up to region borders.
    3. The number tells you how many tiles the eye does NOT see in the region.
*/

namespace puzzles::Unseen{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_region
{
    // character that represents the region. 'a', 'b' and so on
    char m_name;
    // 'eye' (a number) in the region
    int m_num;
};

struct puz_move
{
    char m_name;
    Position m_p_hint;
    set<Position> m_region;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    map<Position, puz_region> m_pos2region;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p, int num)
        : m_game(game), m_p(p), m_num(num) { make_move(p); }

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

    const puz_game* m_game;
    Position m_p;
    int m_num;
    int m_invisible = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            if (contains(p2) || m_game->cells(p2) != PUZ_SPACE) continue;
            if (!children.emplace_back(*this).make_move(p2))
                children.pop_back();
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_r = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch != PUZ_SPACE)
                m_cells.push_back(ch_r),
                m_pos2region[p] = {ch_r++, ch - '0'};
            else
                m_cells.push_back(PUZ_SPACE);
            m_pos2move_ids[p];
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, region] : m_pos2region) {
        auto& [name, num] = region;
        puz_state2 sstart(this, p, num);
        list<list<puz_state2>> spaths;
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, num});
        for (auto& s : smoves) {
            if (s.m_invisible != num)
                continue;
            int n = m_moves.size();
            auto& [name2, p_hint, area] = m_moves.emplace_back();
            name2 = name, p_hint = p, area = s;
            for (auto& p2 : s)
                m_pos2move_ids[p2].push_back(n);
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

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_1, p_hint, area] = m_game->m_moves[id];
            return !boost::algorithm::all_of(area, [&](const Position& p2) {
                return p_hint == p2 || cells(p2) == PUZ_SPACE;
            });
        });
        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [name, _1, area] = m_game->m_moves[n];
    for (auto& p2 : area)
        cells(p2) = name, m_matches.erase(p2), ++m_distance;
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
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return cells(p1) != cells(p2);
    };
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
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
