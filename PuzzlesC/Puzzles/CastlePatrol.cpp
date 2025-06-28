#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 4/Castle Patrol

    Summary
    Don't fall down the wall

    Description
    1. Divide the grid into walls and empty areas. Every area contains one number.
    2. The number indicates the size of the area. Numbers in wall tiles are part
       of wall areas; numbers in empty tiles are part of empty areas.
    3. Areas of the same type cannot share an edge.
*/

namespace puzzles::CastlePatrol{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_WALL = 'W';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_area
{
    // number of the tiles occupied by the area
    int m_num;
    // all permutations (forms) of the area
    vector<set<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    // key: position of the number (hint)
    // value: the area
    map<Position, puz_area> m_pos2area;
    map<Position, vector<pair<Position, int>>> m_pos2perminfos;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const puz_area& area, const Position& p)
        : m_game(&game), m_area(&area), m_ch(game.cells(p)) { make_move(p); }

    bool is_goal_state() const { return m_distance == m_area->m_num; }
    void make_move(const Position& p) { insert(p); ++m_distance; }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    const puz_area* m_area;
    char m_ch = PUZ_SPACE;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // Areas extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile can be occupied by the area
            // if it is a space tile and has not been occupied by the area and
            if (ch2 == PUZ_SPACE && !contains(p2) && boost::algorithm::none_of(offset, [&](const Position& os2) {
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                // no adjacent tile to it is of the same type as the area, because
                // 3. Areas of the same type cannot share an edge.
                return !contains(p3) && ch3 == m_ch;
            })) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch1 = str[c * 2 - 2], ch2 = str[c * 2 - 1];
            m_cells.push_back(ch2);
            if (ch1 != ' ') {
                Position p(r, c);
                auto& area = m_pos2area[p];
                area.m_num = isdigit(ch1) ? ch1 - '0' : ch1 - 'A' + 10;
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    
    for (auto& [p, area] : m_pos2area) {
        puz_state2 sstart(*this, area, p);
        list<list<puz_state2>> spaths;
        // Areas can have any form.
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            // save all goal states as permutations
            // A goal state is a area formed from the number
            for (auto& spath : spaths) {
                auto& rng = spath.back();
                int n = area.m_perms.size();
                area.m_perms.push_back(rng);
                for (auto& p2 : rng)
                    m_pos2perminfos[p2].emplace_back(p, n);
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
    bool make_move(const pair<Position, int>& kv);
    void make_move2(const pair<Position, int>& kv);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position occupied by an area
    // value.key : the hint position of the area
    // value.value: index into the permutations (forms) of the area
    map<Position, vector<pair<Position, int>>> m_matches;
    set<Position> m_hints;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2perminfos)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p2, infos] : m_matches) {
        boost::remove_erase_if(infos, [&](const pair<Position, int>& kv2) {
            auto& p = kv2.first;
            int n = kv2.second;
            char ch = cells(p);
            auto& perm = m_game->m_pos2area.at(p).m_perms[n];
            // remove any path if it contains a tile which belongs to another area
            return boost::algorithm::any_of(perm, [&](const Position& p2) {
                char ch2 = cells(p2);
                return p != p2 && (ch2 != PUZ_SPACE ||
                    boost::algorithm::any_of(offset, [&](const Position& os) {
                    // no adjacent tile to it is of the same type as the area, because
                    // 3. Areas of the same type cannot share an edge.
                    auto p3 = p2 + os;
                    return p != p3 && cells(p3) == ch;
                }));
            });
        });
        if (!init)
            switch(infos.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(infos[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const pair<Position, int>& kv)
{
    auto& [p, n] = kv;
    auto& perm = m_game->m_pos2area.at(p).m_perms[n];

    char ch = cells(p);
    for (auto& p2 : perm)
        cells(p2) = ch, ++m_distance, m_matches.erase(p2);
}

bool puz_state::make_move(const pair<Position, int>& kv)
{
    m_distance = 0;
    make_move2(kv);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, infos] = *boost::min_element(m_matches, [](auto& kv1, auto& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& kv : infos) {
        children.push_back(*this);
        if (!children.back().make_move(kv))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (auto it = m_game->m_pos2area.find(p); it == m_game->m_pos2area.end())
                out << "  ";
            else
                out << format("{:2}", it->second.m_num);
            out << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CastlePatrol()
{
    using namespace puzzles::CastlePatrol;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CastlePatrol.xml", "Puzzles/CastlePatrol.txt", solution_format::GOAL_STATE_ONLY);
}
