#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 3/Zen Gardens

    Summary
    Many Zen Masters

    Description
    1. Put a leaf on every Zen Garden (area).
    2. A Leaf can only be on a Rock.
    3. Three Rocks in a row (horizontally, vertically or diagonally) can't
       have all the leaves or no leaves.
*/

namespace puzzles::ZenGardens{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_STONE = 'O';
constexpr auto PUZ_LEAF = 'L';
constexpr auto PUZ_NOLEAF = 'S';

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

constexpr Position offset3[] = {
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    // 1st dimension : area id
    // 2nd dimension : three contiguous tiles
    vector<vector<Position>> m_lines;
    // all permutations
    // L L S
    // L S L
    // ...
    // S S L
    vector<string> m_lineperms;
    // 1st dimension : garden id
    // 2nd dimension : all stone tiles in the garden
    vector<vector<Position>> m_gardens;
    map<Position, int> m_pos2garden;
    set<Position> m_horz_walls, m_vert_walls;
    map<int, vector<string>> m_num2gardenperms;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    int garden_count() const { return m_gardens.size(); }
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.emplace_back(*this).make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            m_cells.push_back(ch);
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        vector<Position> garden(smoves.begin(), smoves.end());
        boost::remove_erase_if(garden, [&](const Position& p){
            return cells(p) != PUZ_STONE;
        });
        m_gardens.push_back(garden);
        m_num2gardenperms[garden.size()];
        for (auto& p : smoves) {
            m_pos2garden[p] = n;
            rng.erase(p);
        }
    }
    
    for (auto& [i, perms] : m_num2gardenperms) {
        auto perm = PUZ_LEAF + string(i - 1, PUZ_NOLEAF);
        do
            perms.push_back(perm);
        while(boost::next_permutation(perm));
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_STONE) continue;
            for (auto& os : offset3) {
                auto p2 = p + os, p3 = p2 + os;
                if (is_valid(p2) && cells(p2) == PUZ_STONE && is_valid(p3) && cells(p3) == PUZ_STONE)
                    m_lines.push_back({p, p2, p3});
            }
        }
    
    for (int i = 1; i <= 2; i++) {
        auto perm = string(i, PUZ_LEAF) + string(3 - i, PUZ_NOLEAF);
        do
            m_lineperms.push_back(perm);
        while(boost::next_permutation(perm));
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
    bool make_move(int i, int j);
    void make_move2(int i, int j);
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
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (int i = 0; i < g.m_gardens.size(); ++i) {
        auto& garden = g.m_gardens[i];
        auto& perms = g.m_num2gardenperms.at(garden.size());
        vector<int> perm_ids(perms.size());
        boost::iota(perm_ids, 0);
        m_matches[i] = perm_ids;
    }
    vector<int> perm_ids(g.m_lineperms.size());
    boost::iota(perm_ids, 0);
    for (int i = 0; i < g.m_lines.size(); ++i)
        m_matches[i + g.garden_count()] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_lineperms;
    for (auto& [area_id, perm_ids] : m_matches) {
        if (area_id < m_game->garden_count()) {
            auto& garden = m_game->m_gardens[area_id];
            auto& perms = m_game->m_num2gardenperms.at(garden.size());
            boost::remove_erase_if(perm_ids, [&](int id) {
                return !boost::equal(garden, perms[id], [&](const Position& p, char ch2) {
                    char ch1 = cells(p);
                    return ch1 == PUZ_STONE || ch1 == ch2;
                });
            });
        } else {
            string chars;
            for (auto& p : m_game->m_lines[area_id - m_game->garden_count()])
                chars.push_back(cells(p));
            
            boost::remove_erase_if(perm_ids, [&](int id) {
                return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                    return ch1 == PUZ_STONE || ch1 == ch2;
                });
            });
        }

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    if (i < m_game->garden_count()) {
        auto& range = m_game->m_gardens[i];
        auto& perm = m_game->m_num2gardenperms.at(range.size())[j];
        
        for (int k = 0; k < perm.size(); ++k)
            cells(range[k]) = perm[k];
    } else {
        auto& range = m_game->m_lines[i - m_game->garden_count()];
        auto& perm = m_game->m_lineperms[j];
        
        for (int k = 0; k < perm.size(); ++k)
            cells(range[k]) = perm[k];
    }

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (!children.emplace_back(*this).make_move(area_id, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ZenGardens()
{
    using namespace puzzles::ZenGardens;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ZenGardens.xml", "Puzzles/ZenGardens.txt", solution_format::GOAL_STATE_ONLY);
}
