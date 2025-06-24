#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 2/Banquet

    Summary
    A table here, please

    Description
    1. Join the tables in order to form "banquets" of at least two tables.
    2. The number on the table tells you how many tiles it must be moved.
       Tables without numbers must stay put.
    3. Tables can't cross other tables, nor cross other tables paths after
       they moved.
    4. Banquets cannot touch each other horizontally or vertically
       (they can touch diagonally).
    5. Banquets can't be L-shaped but can be more than one table wide.
*/

namespace puzzles::Banquet{

constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TABLE = 'O';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const string_view dirs = "^>v<";

struct puz_path
{
    char dir;
    vector<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    map<Position, vector<puz_path>> m_pos2paths;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch != ' ')
                m_pos2num[p] = ch - '0';
            m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, n] : m_pos2num) {
        auto& paths = m_pos2paths[p];
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            vector<Position> rng(1, p);
            if ([&] {
                auto p2 = p;
                for (int j = 0; j < n; ++j)
                    if (rng.push_back(p2 += os); cells(p2) != PUZ_SPACE)
                        return false;
                return true;
            }())
                paths.emplace_back(dirs[i], rng);
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
    bool make_move(Position p, int n);
    bool make_move2(Position p, int n);
    int find_matches(bool init);
    bool check_tables() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto& [p, paths] : g.m_pos2paths) {
        auto& path_ids = m_matches[p];
        path_ids.resize(paths.size());
        boost::iota(path_ids, 0);
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, path_ids] : m_matches) {
        auto& paths = m_game->m_pos2paths.at(p);
        boost::remove_erase_if(path_ids, [&](int id) {
            auto& [ch, rng] = paths[id];
            return !boost::algorithm::all_of(rng, [&](const Position& p2) {
                return cells(p2) == PUZ_SPACE || p2 == p;
            });
        });

        if (!init)
            switch(path_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, path_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::check_tables() const
{
    return true;
}

bool puz_state::make_move2(Position p, int n)
{
    auto& [ch, rng] = m_game->m_pos2paths.at(p)[n];
    for (auto& p2 : rng)
        cells(p2) = ch;
    cells(rng.back()) = PUZ_TABLE;
    ++m_distance;
    m_matches.erase(p);
    return check_tables();
}

bool puz_state::make_move(Position p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p_basket, path_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : path_ids) {
        children.push_back(*this);
        if (!children.back().make_move(p_basket, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << PUZ_EMPTY << ' ';
            else
                out << it->second << cells(p);
        }
        println(out);
    }
    println(out);
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            char ch = cells({ r, c });
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Banquet()
{
    using namespace puzzles::Banquet;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Banquet.xml", "Puzzles/Banquet.txt", solution_format::GOAL_STATE_ONLY);
}
