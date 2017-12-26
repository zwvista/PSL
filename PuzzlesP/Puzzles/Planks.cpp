#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Planks

    Summary
    Planks and Nails

    Description
    1. On the board there are a few nails. Each one nails a plank to
       the board.
    2. Planks are 3 tiles long and can be oriented vertically or
       horizontally. The Nail can be in any of the 3 tiles.
    3. Each Plank touches orthogonally exactly two other Planks.
    4. All the Planks form a ring, or a closed loop.
*/

namespace puzzles{ namespace Planks{

#define PUZ_SPACE       ' '
#define PUZ_NAIL        'N'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

const Position planks_offset[][3] = {
    {{-2, 0}, {-1, 0}, {0, 0}},
    {{-1, 0}, {0, 0}, {1, 0}},
    {{0, 0}, {1, 0}, {2, 0}},
    {{0, -2}, {0, -1}, {0, 0}},
    {{0, -1}, {0, 0}, {0, 1}},
    {{0, 0}, {0, 1}, {0, 2}},
};


struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, vector<vector<Position>>> m_pos2planks;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            m_start.push_back(ch);
            if (ch == PUZ_NAIL)
                m_pos2planks[{r, c}];
        }
    }
    for (auto& kv : m_pos2planks) {
        const auto& p = kv.first;
        auto& vs = kv.second;
        for (auto& po : planks_offset) {
            vector<Position> v;
            for (auto& os : po)
                v.push_back(p + os);
            if (boost::algorithm::all_of(v, [&](const Position& p2) {return p == p2 || is_valid(p2) && cells(p2) != PUZ_NAIL;}))
                vs.push_back(v);
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const { 
        return m_cells < x.m_cells;
    }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto& kv : g.m_pos2planks) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(kv.second.size());
        boost::iota(perm_ids, 0);
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& range = m_game->m_pos2planks.at(p)[id];
            return boost::algorithm::any_of(range, [&](const Position& p2) {
                char ch = cells(p2);
                return ch != PUZ_SPACE && ch != PUZ_NAIL;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a) : m_area(&a) { make_move(*a.begin()); }

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

bool puz_state::make_move2(const Position& p, int n)
{
    auto& range = m_game->m_pos2planks.at(p)[n];
    for (auto& p : range)
        cells(p) = m_ch;
    ++m_ch, ++m_distance;
    m_matches.erase(p);

    set<Position> rngMatches;
    for (auto& kv : m_matches) {
        auto& rng = m_game->m_pos2planks.at(kv.first);
        for (int n : kv.second)
            for (auto& p : rng[n])
                rngMatches.insert(p);
    }

    set<Position> area;
    map<char, vector<Position>> ch2plank;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_SPACE || ch == PUZ_NAIL) continue;
            area.insert(p);
            ch2plank[ch].push_back(p);
        }
    for (auto& kv : ch2plank) {
        char ch = kv.first;
        string strNeighbors;
        vector<Position> rngNeighbors;
        for (auto& p : kv.second)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (!is_valid(p2)) continue;
                char ch2 = cells(p2);
                if (ch2 == PUZ_SPACE)
                    rngNeighbors.push_back(p2);
                if (ch2 == PUZ_NAIL || ch2 != PUZ_SPACE && ch2 != ch && strNeighbors.find(ch2) == -1)
                    strNeighbors += ch2;
            }
        int sz = strNeighbors.size();
        if (sz > 2 || sz < 2 && (is_goal_state() || boost::algorithm::all_of(rngNeighbors, [&](const Position& p) {
            return rngMatches.count(p) == 0;
        })))
            return false;
    }

    if (!is_goal_state()) return true;

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(area, smoves);
    return smoves.size() == area.size();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << cells(p) << m_game->cells(p);
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Planks()
{
    using namespace puzzles::Planks;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Planks.xml", "Puzzles/Planks.txt", solution_format::GOAL_STATE_ONLY);
}
