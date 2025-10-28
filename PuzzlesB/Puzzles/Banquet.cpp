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
constexpr auto PUZ_QM = '?';
constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;

const string_view dirs = "^>v<";

struct puz_path
{
    char dir;
    vector<Position> m_rng;
    Position m_p_goal;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;
    map<Position, vector<puz_path>> m_pos2paths;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_TABLE);
                m_pos2num[p] = ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, n] : m_pos2num) {
        if (n == 0) continue;
        auto& paths = m_pos2paths[p];
        if (n == PUZ_UNKNOWN)
            paths.push_back({PUZ_TABLE, {}, p});
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            vector<Position> rng(1, p);
            auto p2 = p + os;
            for (int j = 1;; ++j, p2 += os) {
                if (cells(p2) != PUZ_SPACE)
                    break;
                if (n == PUZ_UNKNOWN || j == n) {
                    paths.emplace_back(dirs[i], rng, p2);
                    if (n != PUZ_UNKNOWN)
                        break;
                }
                rng.push_back(p2);
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
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(Position p, int n);
    void make_move2(Position p, int n);
    int find_matches(bool init);
    bool check_tables() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
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
            auto& [_1, rng, p_goal] = paths[id];
            return !rng.empty() && (!boost::algorithm::all_of(rng, [&](const Position& p2) {
                return cells(p2) == PUZ_SPACE || p2 == p;
            }) || cells(p_goal) != PUZ_SPACE);
        });

        if (!init)
            switch(path_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, path_ids[0]), 1;
            }
    }
    return check_tables() ? 2 : 0;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p = *this + os;
            m_state->cells(p) == PUZ_TABLE && !m_state->m_matches.contains(p))
            children.emplace_back(*this).make_move(p);
}

// 4. Banquets cannot touch each other horizontally or vertically
// (they can touch diagonally).
// 5. Banquets can't be L-shaped but can be more than one table wide.
bool puz_state::check_tables() const
{
    auto f = [&](const Position& p) {
        return boost::algorithm::any_of(m_matches, [&](const pair<const Position, vector<int>>& kv) {
            auto& [p2, path_ids] = kv;
            auto& paths = m_game->m_pos2paths.at(p2);
            return boost::algorithm::any_of(path_ids, [&](int id) {
                return paths[id].m_p_goal == p;
            });
        });
    };

    set<Position> rng;
    for (int r = 1; r < sidelen(); ++r)
        for (int c = 1; c < sidelen(); ++c)
            if (Position p(r, c);
                cells(p) == PUZ_TABLE && !m_matches.contains(p))
                rng.insert(p);

    while (!rng.empty()) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        if (smoves.size() == 1) {
            if (auto& p = smoves.front();
                !boost::algorithm::any_of(offset, [&](const Position& os) {
                auto p2 = p + os;
                char ch = cells(p2);
                return (ch == PUZ_SPACE || ch == PUZ_TABLE) && f(p2);
            }))
                return false;
        } else {
            int r1 = boost::min_element(smoves, [&](const puz_state2& p1, const puz_state2& p2) {
                return p1.first < p2.first;
            })->first;
            int c1 = boost::min_element(smoves, [&](const puz_state2& p1, const puz_state2& p2) {
                return p1.second < p2.second;
            })->second;
            int r2 = boost::max_element(smoves, [&](const puz_state2& p1, const puz_state2& p2) {
                return p1.first < p2.first;
            })->first;
            int c2 = boost::max_element(smoves, [&](const puz_state2& p1, const puz_state2& p2) {
                return p1.second < p2.second;
            })->second;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c)
                    if (Position p(r, c);
                        !boost::algorithm::any_of_equal(smoves, p) && !f(p))
                        return false;
        }
        for (auto& p : smoves)
            rng.erase(p);
    }
    return true;
}

void puz_state::make_move2(Position p, int n)
{
    auto& [ch, rng, p_goal] = m_game->m_pos2paths.at(p)[n];
    for (auto& p2 : rng)
        cells(p2) = ch;
    cells(p_goal) = PUZ_TABLE;
    ++m_distance;
    m_matches.erase(p);
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
    auto& [p, path_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : path_ids)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ' ';
            else if (int num = it->second; num == PUZ_UNKNOWN)
                out << PUZ_QM;
            else
                out << num;
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
