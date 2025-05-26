#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 4/Galaxies

    Summary
    Fill the Symmetric Spiral Galaxies

    Description
    1. In the board there are marked centers of a few 'Spiral' Galaxies.
    2. These Galaxies are symmetrical to a rotation of 180 degrees. This
       means that rotating the shape of the Galaxy by 180 degrees (half a
       full turn) around the center, will result in an identical shape.
    3. In the end, all the space must be included in Galaxies and Galaxies
       can't overlap.
    4. There can be single tile Galaxies (with the center inside it) and
       some Galaxy center will be cross two or four tiles.
*/

namespace puzzles::Galaxies{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '+';
constexpr auto PUZ_GALAXY = 'o';
constexpr auto PUZ_GALAXY_R = '>';
constexpr auto PUZ_GALAXY_C = 'v';
constexpr auto PUZ_GALAXY_RC = 'x';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<Position> m_galaxies;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * (m_sidelen - 2) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start = boost::accumulate(strs, string());
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        for (int c = 1; c < m_sidelen - 1; ++c)
            switch(str[c - 1]) {
            case PUZ_GALAXY:
                m_galaxies.emplace_back(r * 2, c * 2);
                break;
            case PUZ_GALAXY_R:
                m_galaxies.emplace_back(r * 2, c * 2 + 1);
                break;
            case PUZ_GALAXY_C:
                m_galaxies.emplace_back(r * 2 + 1, c * 2);
                break;
            case PUZ_GALAXY_RC:
                m_galaxies.emplace_back(r * 2 + 1, c * 2 + 1);
                break;
            }
    }
}

struct puz_galaxy
{
    // m_inner only holds half of the tiles in the galaxy,
    // as galaxies are symmetrical.
    set<Position> m_inner, m_outer;
    // position the galaxy is symmetrical to, which means
    // if position p is in the galaxy, 
    // the position (m_center - p) must also be in the galaxy.
    Position m_center;
    bool m_center_in_cell = false;

    puz_galaxy() {}
    puz_galaxy(const Position& p) : m_center(p) {
        Position p2(p.first / 2, p.second / 2);
        m_inner.insert(p2);
        // cross 4 tiles
        if (p.first % 2 == 1 && p.second % 2 == 1)
            m_inner.insert(p2 + Position(0, 1));
        // inside a tile
        else if (p.first % 2 == 0 && p.second % 2 == 0)
            m_center_in_cell = true;
    }
};

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(char ch, const Position& p);
    bool adjust_galaxies();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return 2; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<char, puz_galaxy> m_galaxies;
    string m_next;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < sidelen(); ++i)
        cells({i, 0}) = cells({i, sidelen() - 1}) =
        cells({0, i}) = cells({sidelen() - 1, i}) =
        PUZ_BOUNDARY;

    char ch = 'a';
    for (auto& p : g.m_galaxies) {
        puz_galaxy glx(p);
        for (auto& p2 : glx.m_inner)
            cells(p2) = cells(glx.m_center - p2) = ch;
        m_galaxies[ch++] = glx;
    }

    adjust_galaxies();
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const Position& starting) : m_state(&s) {
        make_move(starting);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (m_state->cells(*this) != PUZ_SPACE)
        return;
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_state->cells(p2) != PUZ_BOUNDARY) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::adjust_galaxies()
{
    for (auto it = m_galaxies.begin(); it != m_galaxies.end();) {
        auto& glx = it->second;
        glx.m_outer.clear();
        for (auto& p : glx.m_inner)
            for (int i = 0; i < 4; ++i) {
                if (glx.m_center_in_cell && glx.m_inner.size() == 1 && i > 1) continue;
                auto p2 = p + offset[i];
                auto p3 = glx.m_center - p2;
                if (cells(p2) == PUZ_SPACE && cells(p3) == PUZ_SPACE)
                    glx.m_outer.insert(min(p2, p3));
            }
        if (glx.m_outer.empty())
            it = m_galaxies.erase(it);
        else
            ++it;
    }

    set<Position> rng;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) == PUZ_SPACE)
                rng.insert(p);
        }

    set<set<char>> idss;
    while (!rng.empty()) {
        list<puz_state2> smoves;
        // find all tiles reachable from the first space tile
        puz_move_generator<puz_state2>::gen_moves({*this, *rng.begin()}, smoves);
        vector<Position> rng2;
        set<char> ids1;
        for (auto& p : smoves) {
            char ch = cells(p);
            if (ch == PUZ_SPACE)
                rng2.push_back(p); // space tiles
            else if (m_galaxies.contains(ch))
                ids1.insert(ch); // galaxies
        }
        // For each space tile, there should exist at least one galaxy
        // from which the space tile is reachable
        for (auto& p : rng2) {
            set<char> ids2;
            for (char ch : ids1) {
                // The galaxies are symmetrical
                auto p2 = m_galaxies.at(ch).m_center - p;
                if (p2.first > 0 && p2.second > 0 &&
                    p2.first < sidelen() - 1 && p2.second < sidelen() - 1 &&
                    cells(p2) == PUZ_SPACE)
                    ids2.insert(ch);
            }
            if (ids2.empty())
                return false;
            idss.insert(ids2);
            rng.erase(p);
        }
    }
    if (idss.empty())
        m_next = "";
    else {
        // Find the space tile reachable from the fewest number of galaxies
        auto& ids3 = *boost::min_element(idss,
            [](const set<char>& ids1, const set<char>& ids2) {
            return ids1.size() < ids2.size();
        });
        m_next = string(ids3.begin(), ids3.end());
    }
    return true;
}

bool puz_state::make_move(char ch, const Position& p)
{
    auto& glx = m_galaxies.at(ch);
    glx.m_inner.insert(p);
    cells(p) = cells(glx.m_center - p) = ch;
    return adjust_galaxies();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (char ch : m_next)
        for (auto& p : m_galaxies.at(ch).m_outer) {
            children.push_back(*this);
            if (!children.back().make_move(ch, p))
                children.pop_back();
        }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horz-walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (r > 1 && c > 1 && m_game->cells({r - 2, c - 2}) == PUZ_GALAXY_RC ? PUZ_GALAXY : ' ')
            << (cells({r - 1, c}) != cells({r, c}) ? '-' :
            r > 1 && m_game->cells({r - 2, c - 1}) == PUZ_GALAXY_C ? PUZ_GALAXY : ' ');
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (cells({r, c - 1}) != cells({r, c}) ? '|' :
                c > 1 && m_game->cells({r - 1, c - 2}) == PUZ_GALAXY_R ? PUZ_GALAXY : ' ');
            if (c == sidelen() - 1) break;
            out << (m_game->cells({r - 1, c - 1}) == PUZ_GALAXY ? PUZ_GALAXY : '.');
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Galaxies()
{
    using namespace puzzles::Galaxies;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Galaxies.xml", "Puzzles/Galaxies.txt", solution_format::GOAL_STATE_ONLY);
}
