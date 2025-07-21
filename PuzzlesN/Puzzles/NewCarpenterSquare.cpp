#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 3/New Carpenter Square

    Summary
    The old one was cooked

    Description
    1. Divide the board in 'L'-shaped figures, with one cell wide 'legs'.
    2. Every symbol on the board represents the corner of an L.
       there are no hidden L's.
    3. A = symbol tells you that the legs have equal length.
    4. A ÅÇ symbol tells you that the legs have different lengths.
    5. A ? symbol tells you that the legs could have different lengths
       or equal length.
*/

namespace puzzles::NewCarpenterSquare{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_NOT_EQUAL = '/';
constexpr auto PUZ_EQUAL = '=';
constexpr auto PUZ_QM = '?';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

const vector<vector<int>> tool_dirs2 = {
    {0, 1}, {0, 3}, {1, 2}, {2, 3}
};

struct puz_hint
{
    char m_symbol, m_name;
};

struct puz_perm
{
    Position m_p_hint;
    vector<Position> m_rng;
    char m_name;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_hint> m_pos2hint;
    string m_cells;
    vector<puz_perm> m_perms;
    map<Position, vector<int>> m_pos2perm_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_t = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                Position p(r, c);
                m_pos2hint[p] = {ch, ch_t};
                m_cells.push_back(ch_t++);
            }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, hint] : m_pos2hint) {
        auto [symbol, name] = hint;

        // 2. Every symbol on the board represents the corner of an L.
        // there are no hidden L's.
        vector<vector<Position>> arms(4);
        vector<vector<int>> arm_lens(4);
        Position p2;
        char ch2;
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            int j = 1;
            for (p2 = p + os; (ch2 = cells(p2)) == PUZ_SPACE; p2 += os) {
                arms[i].push_back(p2);
                arm_lens[i].push_back(j++);
            }
        }
        vector<vector<Position>> range2D;
        for (auto& dirs : tool_dirs2) {
            auto& a0 = arms[dirs[0]], & a1 = arms[dirs[1]];
            auto& lens0 = arm_lens[dirs[0]], & lens1 = arm_lens[dirs[1]];
            if (a0.empty() || a1.empty()) continue;
            for (int i : lens0)
                for (int j : lens1)
                    // 3. A = symbol tells you that the legs have equal length.
                    // 4. A ÅÇ symbol tells you that the legs have different lengths.
                    // 5. A ? symbol tells you that the legs could have different lengths
                    // or equal length.
                    if (symbol == PUZ_QM ||
                        (symbol == PUZ_EQUAL) == (i == j)) {
                        vector<Position> rng;
                        for (int k = 0; k < i; ++k)
                            rng.push_back(a0[k]);
                        for (int k = 0; k < j; ++k)
                            rng.push_back(a1[k]);
                        rng.push_back(p);
                        boost::sort(rng);
                        int n = m_perms.size();
                        m_perms.emplace_back(p, rng, name);
                        for (auto& p2 : rng)
                            m_pos2perm_ids[p2].push_back(n);
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i);
    void make_move2(int i);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    set<Position> m_used_hints;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
, m_matches(g.m_pos2perm_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, perm_ids] : m_matches) {
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& [p, rng, _2] = m_game->m_perms[id];
            return m_used_hints.contains(p) ||
                !boost::algorithm::all_of(rng, [&](const Position& p2) {
                    return cells(p2) == PUZ_SPACE || p == p2;
                });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i)
{
    auto& [p, rng, ch] = m_game->m_perms[i];
    for (auto& p2 : rng) {
        cells(p2) = ch;
        if (m_matches.erase(p2))
            ++m_distance;
    }
    m_used_hints.insert(p);
}

bool puz_state::make_move(int i)
{
    m_distance = 0;
    make_move2(i);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(n))
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
            //out << cells(p);
            if (auto it = m_game->m_pos2hint.find(p); it == m_game->m_pos2hint.end())
                out << ".";
            else
                out << it->second.m_symbol;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_NewCarpenterSquare()
{
    using namespace puzzles::NewCarpenterSquare;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NewCarpenterSquare.xml", "Puzzles/NewCarpenterSquare.txt", solution_format::GOAL_STATE_ONLY);
}
