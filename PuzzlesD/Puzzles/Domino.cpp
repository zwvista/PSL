#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 5/Domino

    Summary
    Find all the Domino tiles

    Description
    1. On the board there is a complete Domino set. The Domino tiles borders
       however aren't marked, it's up to you to identify them.
    2. In early levels the board contains a smaller Domino set, of numbers
       ranging from 0 to 3.
    3. This means you will be looking for a Domino set composed of these
       combinations.
       0-0, 0-1, 0-2, 0-3
       1-1, 1-2, 1-3
       2-2, 2-3
       3-3
    4. In harder levels, the Domino set will also include fours, fives,
       sixes, etc.
*/

namespace puzzles::Domino{

#define PUZ_SPACE        ' '

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_domino_kind
{
    string m_str;
    Position m_offset;
};
const vector<puz_domino_kind> domino_kinds = {
    {"h-", {0, 1}},    // horizontal domino
    {"v|", {1, 0}},    // vertical domino
};

struct puz_domino
{
    puz_domino(int k, const Position& p1)
    : m_kind(k), m_p1(p1) {}

    Position p2() const { return m_p1 + domino_kinds[m_kind].m_offset; }
    const string& str() const { return domino_kinds[m_kind].m_str; }

    // the kind of the domino
    int m_kind;
    // tile one and tile two
    Position m_p1;
};

struct puz_game
{
    string m_id;
    Position m_size;
    vector<int> m_start;
    map<vector<int>, int> m_comb2id;
    map<int, vector<puz_domino>> m_combid2dominoes;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
    int cells(const Position& p) const { return m_start[p.first * cols() + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_size(strs.size(), strs[0].size())
{
    for (int r = 0; r < rows(); ++r) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); ++c)
            m_start.push_back(str[c] - '0');
    }

    for (int i = 0, n = 0; i < rows(); ++i)
        for (int j = i; j < rows(); ++j)
            m_comb2id[{i, j}] = n++;

    auto f = [&](int kind, Position p1) {
        puz_domino d(kind, p1);
        int n1 = cells(p1), n2 = cells(d.p2());
        if (n1 > n2)
            swap(n1, n2);
        m_combid2dominoes[m_comb2id.at({n1, n2})].push_back(d);
    };
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            if (c < cols() - 1)
                f(0, p);
            if (r < rows() - 1)
                f(1, p);
        }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * cols() + p.second]; }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
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
    map<int, vector<int>> m_matches;
    set<Position> m_horz_walls, m_vert_walls;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.rows() * g.cols(), PUZ_SPACE), m_game(&g)
{
    for (auto& kv : g.m_combid2dominoes) {
        auto& domino_ids = m_matches[kv.first];
        domino_ids.resize(kv.second.size());
        boost::iota(domino_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int comb_id = kv.first;
        auto& domino_ids = kv.second;

        auto& dominos = m_game->m_combid2dominoes.at(comb_id);
        boost::remove_erase_if(domino_ids, [&](int id) {
            auto& d = dominos[id];
            return cells(d.m_p1) != PUZ_SPACE ||
                cells(d.p2()) != PUZ_SPACE;
        });

        if (!init)
            switch(domino_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(comb_id, domino_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& d = m_game->m_combid2dominoes.at(i)[j];
    auto& str = d.str();
    cells(d.m_p1) = str[0];
    cells(d.p2()) = str[1];

    set<Position> rng{d.m_p1, d.p2()};
    for (auto& p : rng)
        for (int i = 0; i < 4; ++i) {
            auto p3 = p + offset[i];
            auto p_wall = p + offset2[i];
            auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
            if (!rng.contains(p3))
                walls.insert(p_wall);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
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
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < cols(); ++c)
            out << (m_horz_walls.contains({r, c}) ? " -" : "  ");
        out << endl;
        if (r == rows()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            out << m_game->cells(p);
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Domino()
{
    using namespace puzzles::Domino;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Domino.xml", "Puzzles/Domino.txt", solution_format::GOAL_STATE_ONLY);
}
