#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 2/Hitori

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

#define PUZ_SHADED        100
#define PUZ_BOUNDARY    0

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

// key.first: the index of a row or column where a number appears on the board
//            The index will be added by the side length of the board 
//            if it represents a column.
// key.second: a number that appears on the board
typedef map<pair<int, int>, vector<Position>> puz_shaded;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_start;
    puz_shaded m_shaded;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            m_start.push_back(n);
            m_shaded[{r, n}].push_back(p);
            m_shaded[{m_sidelen + c, n}].push_back(p);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);

    // All numbers that appear only once in a row or column are removed
    // as they are not the targets to be shaded
    auto it = m_shaded.begin();
    while ((it = find_if(it, m_shaded.end(), [](const puz_shaded::value_type& kv) {
        return kv.second.size() == 1;
    })) != m_shaded.end())
        it = m_shaded.erase(it);
}

struct puz_state : vector<int>
{
    puz_state(const puz_game& g)
        : vector<int>(g.m_start), m_game(&g), m_shaded(g.m_shaded)
    {}

    int sidelen() const {return m_game->m_sidelen;}
    int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const pair<int, int>& key, const Position& p);
    bool is_continuous() const;

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
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    puz_shaded m_shaded;
    int m_distance = 0;
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a): m_area(&a) { make_move(*a.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->count(p) != 0) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// 3. All the un-shaded squares must form a single continuous area.
bool puz_state::is_continuous() const
{
    set<Position> area;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SHADED)
                area.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(area, smoves);
    return smoves.size() == area.size();
}

bool puz_state::make_move(const pair<int, int>& key, const Position& p)
{
    cells(p) = PUZ_SHADED;

    m_distance = 0;
    auto f = [&](const pair<int, int>& k) {
        auto it = m_shaded.find(k);
        if (it != m_shaded.end()) {
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
    })) && is_continuous();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_shaded, [](
        const puz_shaded::value_type& kv1,
        const puz_shaded::value_type& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& p : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, p))
            children.pop_back();
    }
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
        out << endl;
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
