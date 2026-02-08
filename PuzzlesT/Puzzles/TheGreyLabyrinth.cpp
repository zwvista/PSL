#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/The Grey Labyrinth

    Summary
    Maze Curator

    Description
    1. Find the walls that divide the board in a Labyrinth.
    2. The Labyrinth must have these rules:
    3. Walls can't touch each other orthogonally.
    4. From any location, there must only be one route to the treasure.
    5. You must follow the arrows, where present.
*/

namespace puzzles::TheGreyLabyrinth{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TRESURE = 'T';
constexpr auto PUZ_WALL = 'W';

constexpr array<Position, 4> offset = Position::Directions4;

const string_view dirs = "^>v<";

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    Position m_treasure;
    map<Position, vector<Position>> m_pos2rng;
    map<Position, Position> m_pos2pos;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            switch (Position p(r, c); char ch = cells(p)) {
            case PUZ_TRESURE:
                m_treasure = p;
                m_pos2rng[p].push_back(p);
                break;
            case PUZ_SPACE:
                break;
            default:
                {
                    auto& os = offset[dirs.find(ch)];
                    auto p2 = p + os;
                    m_pos2pos[p] = p2;
                    m_pos2rng[p].push_back(p2);
                }
                break;
            }
    for (auto& [p, rng] : m_pos2rng) {
        if (!rng.empty())
            continue;
        for (auto& os : offset) {
            auto p2 = p + os;
            if (auto it = m_pos2pos.find(p2); it == m_pos2pos.end())
                rng.push_back(p2);
            else if (it->second != p)
                rng.push_back(p2);
        }
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
    void make_move2(const Position& p, int n);
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
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    // key: the position of the arrow that resides outside the board
    // value.elem: the possible direction of the arrow
    map<Position, set<int>> m_arrow_dirs;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    //for (auto& [p, TheGreyLabyrinth] : g.m_pos2TheGreyLabyrinth) {
    //    auto& perm_ids = m_matches[p];
    //    perm_ids.resize(TheGreyLabyrinth.m_perms.size());
    //    boost::iota(perm_ids, 0);
    //}

    // compute the possible directions of the TheGreyLabyrinth
    // that reside outside the board
    auto f = [&](int r, int c) {
        Position p(r, c);
        auto& dirs = m_arrow_dirs[p];
        for (int i = 0; i < 8; ++i) {
            auto p2 = p + offset[i];
            // the arrow must point to a tile inside the board
            if (p2.first > 0 && p2.second > 0 &&
                p2.first < sidelen() - 1 && p2.second < sidelen() - 1)
                dirs.insert(i);
        }
    };
    for (int i = 1; i < sidelen() - 1; ++i)
        f(0, i), f(sidelen() - 1, i), f(i, 0), f(i, sidelen() - 1);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        //auto& arrow = m_game->m_pos2TheGreyLabyrinth.at(p);
        //vector<set<int>> arrow_dirs;
        //for (auto& p2 : arrow.m_range)
        //    arrow_dirs.push_back(m_arrow_dirs.at(p2));

        //boost::remove_erase_if(perm_ids, [&](int id) {
        //    return !boost::equal(arrow_dirs, arrow.m_perms[id], [](const set<int>& dirs, int n2) {
        //        // a positive or 0 direction means the arrow is supposed to
        //        // point to the number, so it should be contained
        //        return n2 >= 0 && dirs.contains(n2)
        //            // a negative direction means the arrow is not supposed
        //            // to point to the number, so it should not be the only
        //            // possible direction
        //            || n2 < 0 && (dirs.size() > 1 || !dirs.contains(~n2));
        //    });
        //});

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    //auto& arrow = m_game->m_pos2TheGreyLabyrinth.at(p);
    //auto& perm = arrow.m_perms[n];

    //for (int k = 0; k < perm.size(); ++k) {
    //    const auto& p2 = arrow.m_range[k];
    //    int& n1 = cells(p2);
    //    int n2 = perm[k];
    //    if (n2 >= 0)
    //        m_arrow_dirs[p2] = {n1 = n2};
    //    else
    //        m_arrow_dirs[p2].erase(~n2);
    //}

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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_TheGreyLabyrinth()
{
    using namespace puzzles::TheGreyLabyrinth;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheGreyLabyrinth.xml", "Puzzles/TheGreyLabyrinth.txt", solution_format::GOAL_STATE_ONLY);
}
