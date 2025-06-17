#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Pond camping

    Summary
    Splash!

    Description
    1. The numbers are Ponds. From each Pond you can have a hike of that many
       tiles as the number marked on it.
*/

namespace puzzles::PondCamping{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_POND = '=';
constexpr auto PUZ_CAMPER = 'C';
constexpr auto PUZ_BOUNDARY = '`';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_hike
{
    set<Position> m_empties;
    set<Position> m_ponds;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    // key: position of the number (hint)
    map<Position, vector<puz_hike>> m_pos2hikes;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const Position& p, int num)
        : m_game(&game), m_num(num) {
        make_move(p);
    }

    bool is_goal_state() const { return m_distance == m_num + 1; }
    void make_move(const Position& p) { insert(p); ++m_distance; }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_num;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            if (!contains(p2) && m_game->cells(p2) == PUZ_SPACE) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                m_start.push_back(PUZ_CAMPER);
                m_pos2num[{r, c}] = ch - '0';
            }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, num] : m_pos2num) {
        puz_state2 sstart(*this, p, num);
        list<list<puz_state2>> spaths;
        puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
        // save all goal states as permutations
        auto& hikes = m_pos2hikes[p];
        for (auto& spath : spaths) {
            auto& [empties, ponds] = hikes.emplace_back();
            empties = spath.back();
            for (auto& p2 : empties)
                for (auto& os : offset) {
                    auto p3 = p2 + os;
                    if (!empties.contains(p3) && cells(p3) == PUZ_SPACE)
                        ponds.insert(p3);
                }
            empties.erase(p);
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
    bool make_move_hike(const Position& p, int n);
    void make_move_hike2(const Position& p, int n);
    void make_move_pond(const Position& p) { cells(p) = PUZ_POND; }
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for (auto& [p, hikes] : g.m_pos2hikes) {
        auto& hike_ids = m_matches[p];
        hike_ids.resize(hikes.size());
        boost::iota(hike_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, hike_ids] : m_matches) {
        auto& hikes = m_game->m_pos2hikes.at(p);
        boost::remove_erase_if(hike_ids, [&](int id) {
            auto& [empties, ponds] = hikes[id];
            return boost::algorithm::any_of(empties, [&](const Position& p2) {
                char ch = cells(p2);
                return !(ch == PUZ_SPACE || ch == PUZ_EMPTY);
            }) || boost::algorithm::any_of(ponds, [&](const Position& p2) {
                char ch = cells(p2);
                return !(ch == PUZ_SPACE || ch == PUZ_POND);
            });
        });

        if (!init)
            switch(hike_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hike2(p, hike_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move_hike2(const Position& p, int n)
{
    auto& [empties, ponds] = m_game->m_pos2hikes.at(p)[n];
    for (auto& p2 : empties) {
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = PUZ_EMPTY, ++m_distance;
    }
    for (auto& p2 : ponds) {
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = PUZ_POND, ++m_distance;
    }
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move_hike(const Position& p, int n)
{
    m_distance = 0;
    make_move_hike2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, hike_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : hike_ids) {
            children.push_back(*this);
            if (!children.back().make_move_hike(p, n))
                children.pop_back();
        }
    } else {
        int i = m_cells.find(PUZ_SPACE);
        Position p(i / sidelen(), i % sidelen());
        children.push_back(*this);
        children.back().make_move_pond(p);
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it != m_game->m_pos2num.end())
                out << it->second;
            else
                out << cells(p);
            out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PondCamping()
{
    using namespace puzzles::PondCamping;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PondCamping.xml", "Puzzles/PondCamping.txt", solution_format::GOAL_STATE_ONLY);
}
