#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 1/Nurikabe

    Summary
    Draw a continuous wall that divides gardens as big as the numbers

    Description
    1. Each number on the grid indicates a garden, occupying as many tiles
       as the number itself.
    2. Gardens can have any form, extending horizontally and vertically, but
       can't extend diagonally.
    3. The garden is separated by a single continuous wall. This means all
       wall tiles on the board must be connected horizontally or vertically.
       There can't be isolated walls.
    4. You must find and mark the wall following these rules:
    5. All the gardens in the puzzle are numbered at the start, there are no
       hidden gardens.
    6. A wall can't go over numbered squares.
    7. The wall can't form 2*2 squares.
*/

namespace puzzles{ namespace Nurikabe2{

#define PUZ_SPACE        ' '
#define PUZ_WALL        'W'
#define PUZ_BOUNDARY    '+'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_garden
{
    char m_name;
    int m_num;
    vector<set<Position>> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_garden> m_pos2garden;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const puz_garden& garden, const Position& p)
        : m_game(&game), m_garden(&garden) {make_move(p);}

    bool is_goal_state() const { return m_distance == m_garden->m_num; }
    void make_move(const Position& p) { insert(p); ++m_distance; }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const puz_garden* m_garden;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for(auto& p : *this)
        for(auto& os : offset){
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if(ch2 != PUZ_SPACE || count(p2) != 0 || boost::algorithm::any_of(offset, [&](const Position& os2){
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                return count(p3) == 0 && ch3 != PUZ_SPACE && ch3 != PUZ_BOUNDARY;
            })) continue;
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_g = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(int r = 1; r < m_sidelen - 1; ++r){
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for(int c = 1; c < m_sidelen - 1; ++c){
            char ch = str[c - 1];
            if(ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else{
                int n = ch - '0';
                Position p(r, c);
                auto& garden = m_pos2garden[p];
                garden.m_num = n;
                m_start.push_back(garden.m_name = n == 1 ? '.' : ch_g++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(auto& kv : m_pos2garden){
        auto& p = kv.first;
        auto& garden = kv.second;
        if(garden.m_num == 1)
            garden.m_perms = {{p}};
        else {
            puz_state2 sstart(*this, garden, p);
            list<list<puz_state2>> spaths;
            puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
            for(auto& spath : spaths)
                garden.m_perms.push_back(spath.back());
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
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;
    bool no_2x2() const;
    bool is_valid_move() const { return no_2x2() && is_continuous(); }

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for(auto& kv : g.m_pos2garden){
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(kv.second.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    set<Position> rng;
    for(int r = 1; r < sidelen() - 1; ++r)
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            if(cells(p) == PUZ_SPACE)
                rng.insert(p);
        }

    for(auto& kv : m_matches){
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto& perms = m_game->m_pos2garden.at(p).m_perms;
        boost::remove_erase_if(perm_ids, [&](int id){
            return boost::algorithm::any_of(perms[id], [&](const Position& p2){
                return p != p2 && cells(p2) != PUZ_SPACE;
            });
        });
        for(int id : perm_ids)
            for(auto& p2 : perms[id])
                rng.erase(p2);

        if(!init)
            switch(perm_ids.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    for(auto& p : rng)
        // Cells that cannot be reached by any garden must be a wall
        cells(p) = PUZ_WALL;
    return 2;
}

struct puz_state3 : Position
{
    puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for(auto& os : offset){
        auto p2 = *this + os;
        if(m_rng->count(p2) != 0){
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_continuous() const
{
    set<Position> a;
    for(int r = 1; r < sidelen() - 1; ++r)
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_SPACE || ch == PUZ_WALL)
                a.insert(p);
        }

    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

bool puz_state::no_2x2() const
{
    // The wall can't form 2*2 squares.
    for(int r = 1; r < sidelen() - 2; ++r)
        for(int c = 1; c < sidelen() - 2; ++c){
            vector<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
            if(boost::algorithm::all_of(rng, [&](const Position& p){
                return cells(p) == PUZ_WALL;
            }))
                return false;
        }
    return true;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& garden = m_game->m_pos2garden.at(p);
    auto& perm = garden.m_perms[n];

    for(auto& p2 : perm)
        cells(p2) = garden.m_name;
    for(auto& p2 : perm)
        // Gardens are separated by a wall.
        for(auto& os : offset){
            char& ch2 = cells(p2 + os);
            if(ch2 == PUZ_SPACE)
                ch2 = PUZ_WALL;
        }
    ++m_distance;
    m_matches.erase(p);

    return is_valid_move();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if(!make_move2(p, n))
        return false;
    int m;
    while((m = find_matches(false)) == 1);
    return m == 2 && is_valid_move();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2){
        return kv1.second.size() < kv2.second.size();
    });
    for(int n : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_WALL)
                out << PUZ_WALL << ' ';
            else{
                auto it = m_game->m_pos2garden.find(p);
                if(it == m_game->m_pos2garden.end())
                    out << ". ";
                else
                    out << format("%-2d") % it->second.m_num;
            }
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Nurikabe2()
{
    using namespace puzzles::Nurikabe2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Nurikabe.xml", "Puzzles/Nurikabe2.txt", solution_format::GOAL_STATE_ONLY);
}
