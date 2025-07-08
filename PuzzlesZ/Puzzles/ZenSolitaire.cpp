#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 2/Zen Solitaire

    Summary
    Pick up stones

    Description
     1. A favorite Zen Master pastime, scattering stones on the sand and picking them up.
     2. You can start at any stone and pick it up (just to click on it and it will be numbered
        in the order you pick it up).
     3. From a stone, you can move horizontally or vertically to the next stone. You can't
        jump over stones, if you encounter it, you have to pick it up.
     4. When moving from a stone to another, you can change direction, but you cannot reverse it.
     5. when a stone has been picked up, you can pass away it if you encounter it again
        (it's not there anymore).
     6. The goal is to pick up every stone.
*/

namespace puzzles::ZenSolitaire{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_STONE = 'O';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},       // w
};

struct puz_move
{
    Position m_to;
    set<Position> m_on_path; // stones on the path
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    set<Position> m_stones;
    map<Position, vector<puz_move>> m_stone2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    int stone_count() const { return m_stones.size(); }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_stones.emplace(r, c);
    }

    for (auto& p1 : m_stones)
        for (auto& [r1, c1] = p1; auto& p2 : m_stones)
            if (p1 < p2)
                if (auto& [r2, c2] = p2; r1 == r2 || c1 == c2) {
                    set<Position> on_path;
                    if (r1 == r2)
                        for (int c = c1 + 1; c < c2; ++c)
                            if (Position p3(r1, c); m_stones.contains(p3))
                                on_path.insert(p3);
                    else
                        for (int r = r1 + 1; r < r2; ++c)
                            if (Position p3(r, c1); m_stones.contains(p3))
                                on_path.insert(p3);
                    m_stone2moves[p1].emplace_back(p2, on_path);
                    m_stone2moves[p2].emplace_back(p1, on_path);
                }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool operator<(const puz_state& x) const {
        return tie(m_stone2move_ids, m_path) < tie(x.m_stone2move_ids, x.m_path);
    }
    bool make_move(const Position& p, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_stone2move_ids.size(); }
    unsigned int get_distance(const puz_state& child) const {
        return m_stone2move_ids.empty() ? 2 : 1;
    }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_stone2move_ids;
    vector<Position> m_path; // path from the first stone to the current stone
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
    for (auto& [p, moves] : g.m_stone2moves) {
        auto& ids = m_stone2move_ids[p];
        for (int i = 0; i < moves.size(); ++i)
            if (auto& [_1, on_path] = moves[i]; on_path.empty())
                ids.push_back(i);
    }
}

bool puz_state::make_move(const Position& p, int n)
{
    if (m_path.empty())
        m_path.push_back(p); // first stone picked up
    m_stone2move_ids.erase(p);
    auto& [to, _1] = m_game->m_stone2moves.at(p)[n];
    m_path.push_back(to);

    for (auto& [p2, move_ids] : m_stone2move_ids) {
        auto& moves = m_game->m_stone2moves.at(p2);
        for (int i = 0; i < moves.size(); ++i)
            if (auto& [to2, on_path2] = moves[i]; !m_stone2move_ids.contains(to2))
                boost::remove_erase(move_ids, i);
            else if (!on_path2.empty() && boost::algorithm::none_of(on_path2, [&](const Position& p2) {
                return m_stone2move_ids.contains(p2);
            }))
                move_ids.push_back(i);
    }
    if (m_stone2move_ids.size() == 1)
        m_stone2move_ids.clear(); // only one stone left, no more moves available
    else if (boost::algorithm::any_of(m_stone2move_ids,
        [](const pair<const Position, vector<int>>& kv) {
        return kv.second.empty();
    }))
        return false; // no more moves available for some stones
    return true; // move successful
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto f = [&](const Position& p) {
        for (int n : m_stone2move_ids.at(p))
            if (children.push_back(*this); !children.back().make_move(p, n))
                children.pop_back();
    };
    if (m_path.empty())
        for (auto& p : m_game->m_stones)
            f(p);
    else
        f(m_path.back());
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            if (Position p(r, c); !m_game->m_stones.contains(p))
                out << ".. ";
            else {
                int n = boost::find(m_path, p) - m_path.begin() + 1;
                out << format("{:2}", n) << ' ';
            }
        println(out);
    }
    return out;
}

}

void solve_puz_ZenSolitaire()
{
    using namespace puzzles::ZenSolitaire;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ZenSolitaire.xml", "Puzzles/ZenSolitaire.txt", solution_format::GOAL_STATE_ONLY);
}
