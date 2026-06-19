#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 1/Scissors

    Summary
    Tailor's puzzle

    Description
    1. Cut the board into patches.
    2. Each patch should contain the numbers 1 to N exactly once (N being the highest number on the board).
    3. Each patch should end on the border.
*/

namespace puzzles::Scissors{

constexpr auto PUZ_SPACE = ' ';
    
constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::Square2x2Offset;
constexpr array<Position, 4> offset3 = {
    Position{-1, -1},
    Position{-1, 1},
    Position{1, -1},
    Position{1, 1},
};

using puz_slash = pair<Position, Position>;
using puz_move = vector<puz_slash>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    char m_max_num = '1';
    set<puz_slash> m_slashes;
    vector<puz_move> m_moves;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            m_cells.push_back(ch);
            m_max_num = max(m_max_num, ch);
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0';
            else {
                // front slash
                m_slashes.emplace(p + offset2[0], p + offset2[3]);
                // back slash
                m_slashes.emplace(p + offset2[1], p + offset2[2]);
            }
        }
    }

    for (int i = 0; i <= m_sidelen; ++i) {
        puz_move move;
        auto f = [&](int r, int c) {
            Position p0(r, c);
            auto dfs = [&](this const auto& self, const Position& p1) {
                for (auto& os : offset3) {
                    auto p2 = p1 + os;
                    if (!m_slashes.contains({min(p1, p2), max(p1, p2)})) continue;
                    move.emplace_back(p1, p2);
                    self(p2);
                    move.pop_back();
                }
            };
        };
        f(0, i), f(0, m_sidelen), f(i, 0), f(i, m_sidelen);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
}


bool puz_state::make_move(const Position& p, int n)
{
    return true;
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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << format("{:<2}", cells(p));
            else
                out << format("{:<2}", it->second);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Scissors()
{
    using namespace puzzles::Scissors;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Scissors.xml", "Puzzles/Scissors.txt", solution_format::GOAL_STATE_ONLY);
}
