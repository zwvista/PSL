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
    string m_cells;
    set<Position> m_stones;
    map<Position, vector<puz_move>> m_stone2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    m_cells = boost::accumulate(strs, string());
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
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
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

    const puz_game* m_game = nullptr;
    string m_cells;
    set<Position> m_stones;
    map<pair<Position, int>, vector<pair<Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, infos] : m_matches) {
        //boost::remove_erase_if(infos, [&](const pair<Position, int>& info) {
        //    auto& perm = m_game->m_pos2perms.at(info.first)[info.second];
        //    for (int i = 0, sz = perm.size(); i < sz; ++i) {
        //        auto& p2 = perm[i];
        //        int dt = dots(p2);
        //        if (i < sz - 1 && !m_matches.contains({p2, PUZ_FROM}) ||
        //            i > 0 && !m_matches.contains({p2, PUZ_TO}))
        //            return true;
        //    }
        //    return false;
        //});

        if (!init)
            switch(infos.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(infos[0].first, infos[0].second), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    //auto& perm = m_game->m_pos2perms.at(p)[n];
    //for (int i = 0, sz = perm.size(); i < sz; ++i) {
    //    auto& p2 = perm[i];
    //    auto f = [&](const Position& p3, int fromto) {
    //        int dir = boost::find(offset, p3 - p2) - offset;
    //        dots(p2) |= 1 << dir;
    //        dots(p3) |= 1 << (dir + 2) % 4;
    //        m_matches.erase({p2, fromto}), ++m_distance;
    //    };
    //    if (i < sz - 1)
    //        f(perm[i + 1], PUZ_FROM);
    //    if (i > 0)
    //        f(perm[i - 1], PUZ_TO);
    //}
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
    auto& [p, infos] = *boost::min_element(m_matches, [](
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv1,
        const pair<const pair<Position, int>, vector<pair<Position, int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& info : infos)
        if (children.push_back(*this); !children.back().make_move(info.first, info.second))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_STONE)
                out << format("{:2}", 2);
            else
                out << ' ' << ch;
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
