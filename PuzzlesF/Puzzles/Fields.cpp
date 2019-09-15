#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Fields

    Summary
    Twice of the blessings of a Nurikabe

    Description
    1. Fill the board with either meadows or soil, creating a path of soil
       and a path of meadows, with the same rules for each of them.
    2. The path is continuous and connected horizontally or vertically, but
       cannot touch diagonally.
    3. The path can't form 2x2 squares.
    4. These type of paths are called Nurikabe.
*/

namespace puzzles::Fields{

#define PUZ_SPACE        ' '
#define PUZ_SOIL         'S'
#define PUZ_MEADOW       'M'
#define PUZ_BOUNDARY     '+'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
const Position offset2[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
};
    
struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            m_start.push_back(ch);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);
    int adjust_area();
    int check_2x2();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    int n1, n2;
    do {
        n1 = adjust_area();
        n2 = check_2x2();
    } while (n1 != 2 || n2 != 2);
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng, const Position& starting) : m_rng(&rng) { make_move(starting); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->count(p2) != 0) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

int puz_state::adjust_area()
{
    map<Position, vector<char>> pos2kinds;
    for (char chPath : {PUZ_SOIL, PUZ_MEADOW}) {
        // The path is continuous and connected horizontally or vertically.
        set<Position> a;
        Position starting;
        int n1 = 0;
        for (int r = 1; r < sidelen() - 1; ++r)
            for (int c = 1; c < sidelen() - 1; ++c) {
                Position p(r, c);
                if (char ch = cells(p); ch == PUZ_SPACE || ch == chPath) {
                    a.insert(p);
                    if (ch == PUZ_SPACE)
                        pos2kinds[p];
                    else if (n1++ == 0)
                        starting = p;
                }
            }
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({a, starting}, smoves);
        
        int n2 = 0;
        for (auto& p : smoves)
            if (char ch = cells(p); ch == chPath)
                ++n2;
            else
                pos2kinds[p].push_back(chPath);
        
        if (n1 != n2)
            return 0;
    }
    int n = 2;
    for (auto&& [p, kinds] : pos2kinds) {
        if (kinds.empty())
            return 0;
        if (kinds.size() == 1)
            cells(p) = kinds[0], n = 1;
    }
    return n;
}

// The path can't form 2x2 squares.
int puz_state::check_2x2()
{
    int n = 2;
    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c) {
            Position p(r, c);
            vector<Position> rngSpace, rngSoil, rngMeadow;
            for (auto& os : offset2) {
                auto p2 = p + os;
                switch(cells(p2)) {
                case PUZ_SOIL: rngSoil.push_back(p2); break;
                case PUZ_MEADOW: rngMeadow.push_back(p2); break;
                case PUZ_SPACE: rngSpace.push_back(p2); break;
                }
            }
            if (rngSoil.size() == 4 || rngMeadow.size() == 4) return 0;
            if (rngSoil.size() == 3 && rngSpace.size() == 1)
                cells(rngSpace[0]) = PUZ_MEADOW, ++m_distance, n = 1;
            if (rngMeadow.size() == 3 && rngSpace.size() == 1)
                cells(rngSpace[0]) = PUZ_SOIL, ++m_distance, n = 1;
        }
    return n;
}

bool puz_state::make_move(const Position& p, char ch)
{
    m_distance = 0;
    cells(p) = ch, ++m_distance;
    int n1, n2;
    do {
        n1 = adjust_area();
        if (n1 == 0) return false;
        n2 = check_2x2();
        if (n2 == 0) return false;
    } while (n1 != 2 || n2 != 2);
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int i = m_cells.find(PUZ_SPACE);
    Position p(i / sidelen(), i % sidelen());
    for (char ch : {PUZ_SOIL, PUZ_MEADOW}) {
        children.push_back(*this);
        if (!children.back().make_move(p, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
        out << endl;
    }
    return out;
}

}

void solve_puz_Fields()
{
    using namespace puzzles::Fields;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Fields.xml", "Puzzles/Fields.txt", solution_format::GOAL_STATE_ONLY);
}
