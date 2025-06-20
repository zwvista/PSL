#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Archipelago

    Summary
    No bridges, just find the water

    Description
    1. Each number represents a rectangular island in the Archipelago.
    2. The number in itself identifies how many squares the island occupies.
    3. Islands can only touch each other diagonally and by touching they
       must form a network where no island is isolated from the others.
    4. In other words, every island must be touching another island diagonally
       and no group of islands must be separated from the others.
    5. Not all the islands you need to find are necessarily marked by numbers.
    6. Finally, no 2*2 square can be occupied by water only (just like Nurikabe).
*/

namespace puzzles::Archipelago{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_ISLAND = '.';
constexpr auto PUZ_WATER = 'W';

constexpr Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},    // nw
};

struct puz_island_info
{
    // the area of the island
    int m_area;
    // top-left and bottom-right
    vector<pair<Position, Position>> m_islands;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_island_info> m_pos2islandinfo;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            if (ch != PUZ_SPACE) {
                int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_pos2islandinfo[{r, c}].m_area = n;
            }
        }
    }

    for (auto& [pn, info] : m_pos2islandinfo) {
        auto& [island_area, islands] = info;

        for (int i = 1; i <= m_sidelen; ++i) {
            int j = island_area / i;
            if (i * j != island_area || j > m_sidelen) continue;
            Position island_sz(i - 1, j - 1);
            auto p2 = pn - island_sz;
            for (int r = p2.first; r <= pn.first; ++r)
                for (int c = p2.second; c <= pn.second; ++c) {
                    Position tl(r, c), br = tl + island_sz;
                    if (tl.first >= 0 && tl.second >= 0 &&
                        br.first < m_sidelen && br.second < m_sidelen &&
                        boost::algorithm::none_of(m_pos2islandinfo, [&](
                        const pair<const Position, puz_island_info>& kv) {
                        auto& p = kv.first;
                        if (p == pn)
                            return false;
                        for (int k = 0; k < 4; ++k) {
                            auto p2 = p + offset[k * 2];
                            if (p2.first >= tl.first && p2.second >= tl.second &&
                                p2.first <= br.first && p2.second <= br.second)
                                return true;
                        }
                        return false;
                    }))
                        islands.emplace_back(tl, br);
                }
        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n) {
        auto& island = m_game->m_pos2islandinfo.at(p).m_islands[n];
        make_move3(p, island);
    }
    void make_move3(const Position& p, const pair<Position, Position>& island);
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0 && is_interconnected(); }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size() + m_2by2waters.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    vector<set<Position>> m_2by2waters;
    vector<pair<Position, Position>> m_islands;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int r = 0; r < g.m_sidelen - 1; ++r)
        for (int c = 0; c < g.m_sidelen - 1; ++c)
            m_2by2waters.push_back({{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}});

    for (auto& [p, info] : g.m_pos2islandinfo) {
        auto& island_ids = m_matches[p];
        island_ids.resize(info.m_islands.size());
        boost::iota(island_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, island_ids] : m_matches) {
        auto& islands = m_game->m_pos2islandinfo.at(p).m_islands;
        boost::remove_erase_if(island_ids, [&](int id) {
            auto& island = islands[id];
            for (int r = island.first.first; r <= island.second.first; ++r)
                for (int c = island.first.second; c <= island.second.second; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            return false;
        });

        if (!init)
            switch(island_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, island_ids.front()), 1;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const Position& p_start) : m_state(&s) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os;
            m_state->is_valid(p2) && m_state->cells(p2) == PUZ_ISLAND) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

// 3. Islands can only touch each other diagonally and by touching they
// must form a network where no island is isolated from the others.
// 4. In other words, every island must be touching another island diagonally
// and no group of islands must be separated from the others.
bool puz_state::is_interconnected() const
{
    int i = this->find(PUZ_ISLAND);
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        {*this, {i / sidelen(), i % sidelen()}});
    return smoves.size() == boost::count(*this, PUZ_ISLAND);
}

void puz_state::make_move3(const Position& p, const pair<Position, Position>& island)
{
    int cnt = m_2by2waters.size();
    auto &tl = island.first, &br = island.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c) {
            Position p2(r, c);
            cells(p2) = PUZ_ISLAND;
            boost::remove_erase_if(m_2by2waters, [&](const set<Position>& rng) {
                return rng.contains(p2);
            });
        }

    auto f = [&](const Position& p2) {
        if (is_valid(p2))
            cells(p2) = PUZ_WATER;
    };
    for (int r = tl.first; r <= br.first; ++r)
        f({r, tl.second - 1}), f({r, br.second + 1});
    for (int c = tl.second; c <= br.second; ++c)
        f({tl.first - 1, c}), f({br.first + 1, c});

    m_distance += cnt - m_2by2waters.size() + 1;
    m_matches.erase(p);
    m_islands.push_back(island);
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
    if (!m_matches.empty()) {
        auto& [p, island_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : island_ids) {
            children.push_back(*this);
            if (!children.back().make_move(p, n))
                children.pop_back();
        }
    } else
        for (int i = 0; i < length(); ++i) {
            if ((*this)[i] != PUZ_SPACE) continue;
            Position p(i / sidelen(), i % sidelen());
            for (int r = p.first; r < sidelen(); ++r)
                for (int c = p.second; c < sidelen(); ++c)
                    if ([&]{
                        for (int r2 = p.first; r2 <= r; ++r2)
                            for (int c2 = p.second; c2 <= c; ++c2)
                                if (cells({r2, c2}) != PUZ_SPACE)
                                    return false;
                        return true;
                    }()) {
                        children.push_back(*this);
                        children.back().make_move3(p, {p, {r, c}});
                    }
        }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2islandinfo.find(p);
                it == m_game->m_pos2islandinfo.end())
                out << cells(p) << ' ';
            else
                out << format("{:<2}", it->second.m_area);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Archipelago()
{
    using namespace puzzles::Archipelago;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Archipelago.xml", "Puzzles/Archipelago.txt", solution_format::GOAL_STATE_ONLY);
}
