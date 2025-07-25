#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::SquareMergePuzzle{

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // 2*2 nw
    {0, 1},        // 2*2 ne
    {1, 0},        // 2*2 sw
    {1, 1},        // 2*2 se
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells, m_goal;
    map<char, vector<Position>> m_ch2rng;

    puz_game(const vector<string>& strs, const xml_node& level);
    char goal(const Position& p) const { return m_goal[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2)
{
    m_goal = accumulate(strs.begin(), strs.begin() + m_sidelen, string());
    m_cells = accumulate(strs.begin() + m_sidelen, strs.end(), string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_ch2rng[goal(p)].push_back(p);
        }
}

using puz_move = pair<Position, bool>;

ostream& operator<<(ostream& out, const puz_move& a)
{
    out << format("({},{}){}", a.first.first, a.first.second, a.second ? 'r' : 'l');
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_game(&g), m_cells(g.m_cells) {}
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_move) < tie(x.m_cells, x.m_move);
    }
    void make_move(const Position& p, bool clockwise);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    //unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    optional<puz_move> m_move;
    unsigned int m_distance = 0;
};

unsigned int puz_state::get_heuristic() const
{
    unsigned int h = 0;
    map<char, vector<Position>> ch2rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            ch2rng[cells(p)].push_back(p);
        }
    for (auto& [ch, rng] : m_game->m_ch2rng) {
        auto& rng2 = ch2rng[ch];
        for (int i = 0; i < rng.size(); ++i)
            h += manhattan_distance(rng[i], rng2[i]);
    }
    return h;
}

void puz_state::make_move(const Position& p, bool clockwise)
{
    // 0 1
    // 2 3
    // counter-clockwise
    // 1 3
    // 0 2
    // clockwise
    // 2 0
    // 3 1
    auto d1 = get_heuristic();
    auto p1 = p + offset2[1], p2 = p + offset2[2], p3 = p + offset2[3];
    if (!clockwise)
        cells(p1) = exchange(cells(p3), exchange(cells(p2), exchange(cells(p), cells(p1))));
    else
        cells(p2) = exchange(cells(p3), exchange(cells(p1), exchange(cells(p), cells(p2))));
    m_move = puz_move(p, clockwise);
    m_distance = d1 - get_heuristic();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            for (int i = 0; i < 2; ++i) {
                children.push_back(*this);
                children.back().make_move(p, i == 1);
            }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c});
        println(out);
    }
    return out;
}

}

void solve_puz_SquareMergePuzzle()
{
    using namespace puzzles::SquareMergePuzzle;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/SquareMergePuzzle.xml", "Puzzles/SquareMergePuzzle.txt");
}
