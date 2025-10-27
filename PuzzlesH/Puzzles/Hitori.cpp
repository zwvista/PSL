#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 2/Hitori

    Summary
    Darken some tiles so no number appears in a row or column more than once

    Description
    1. The goal is to shade squares so that a number appears only once in a
       row or column.
    2. While doing that, you must take care that shaded squares don't touch
       horizontally or vertically between them.
    3. In the end all the un-shaded squares must form a single continuous area.
*/

namespace puzzles::Hitori{

constexpr auto PUZ_SHADED = 100;
constexpr auto PUZ_BOUNDARY = 0;

constexpr array<Position, 4> offset = Position::Directions4;

// key.first: the index of a row or column where a number appears on the board
//            The index will be added by the side length of the board 
//            if it represents a column.
// key.second: a number that appears on the board
using puz_shaded = map<pair<int, int>, vector<Position>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_cells;
    puz_shaded m_shaded;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            m_cells.push_back(n);
            m_shaded[{r, n}].push_back(p);
            m_shaded[{m_sidelen + c, n}].push_back(p);
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.insert(m_cells.end(), m_sidelen, PUZ_BOUNDARY);

    // All numbers that appear only once in a row or column are removed
    // as they are not the targets to be shaded
    auto it = m_shaded.begin();
    while ((it = find_if(it, m_shaded.end(), [](const puz_shaded::value_type& kv) {
        return kv.second.size() == 1;
    })) != m_shaded.end())
        it = m_shaded.erase(it);
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_game(&g), m_shaded(g.m_shaded)
    {}

    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const pair<int, int>& key, const Position& p);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_shaded, 0, [](int acc, const puz_shaded::value_type& kv) {
            return acc + kv.second.size() - 1;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<int> m_cells;
    puz_shaded m_shaded;
    int m_distance = 0;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

inline bool is_unshaded(char ch) { return ch != PUZ_SHADED && ch != PUZ_BOUNDARY; }

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; is_unshaded(m_state->cells(p2))) {
            children.emplace_back(*this).make_move(p2);
        }
}

// 3. All the un-shaded squares must form a single continuous area.
bool puz_state::is_interconnected() const
{
    int i = boost::find_if(m_cells, is_unshaded) - m_cells.begin();
    auto smoves = puz_move_generator<puz_state2>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return smoves.size() == boost::count_if(m_cells, is_unshaded);
}

bool puz_state::make_move(const pair<int, int>& key, const Position& p)
{
    cells(p) = PUZ_SHADED;

    m_distance = 0;
    auto f = [&](const pair<int, int>& k) {
        if (auto it = m_shaded.find(k); it != m_shaded.end()) {
            ++m_distance;
            boost::remove_erase(it->second, p);
            if (it->second.size() == 1)
                m_shaded.erase(it);
        }
    };
    f(key);
    f({key.first < sidelen() ? sidelen() + p.second : p.first, key.second});

    // 2. Shaded squares don't touch horizontally or vertically between them.
    return (boost::algorithm::none_of(offset, [&](const Position& os) {
        return cells(p + os) == PUZ_SHADED;
    })) && is_interconnected();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [key, v] = *boost::min_element(m_shaded, [](
        const puz_shaded::value_type& kv1,
        const puz_shaded::value_type& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& p : v)
        if (!children.emplace_back(*this).make_move(key, p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            int n = cells(p);
            if (n == PUZ_SHADED)
                out << " .";
            else
                out << format("{:2}", n);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Hitori()
{
    using namespace puzzles::Hitori;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Hitori.xml", "Puzzles/Hitori.txt", solution_format::GOAL_STATE_ONLY);
}
