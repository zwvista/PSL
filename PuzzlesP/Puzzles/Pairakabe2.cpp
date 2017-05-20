#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 12/Pairakabe

    Summary
    Just to confuse things a bit more

    Description
    1. Plays like Nurikabe, with an interesting twist.
    2. Instead of just one number, each 'garden' contains two numbers and
       the area of the garden is given by the sum of both.
*/

namespace puzzles{ namespace Pairakabe2{

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
    map<Position, int> m_pos2num;
    map<pair<Position, Position>, puz_garden> m_pair2garden;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const puz_garden& garden, const Position& p, const Position& p2)
        : m_game(&game), m_garden(&garden), m_p2(&p2) {make_move(p);}

    bool is_goal_state() const { return m_distance == m_garden->m_num; }
    bool make_move(const Position& p) {
        insert(p); ++m_distance; 
        return boost::algorithm::any_of(*this, [&](const Position& p2){
            return manhattan_distance(p2, *m_p2) <= m_garden->m_num - m_distance;
        });
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const puz_garden* m_garden;
    const Position* m_p2;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for(auto& p : *this)
        for(auto& os : offset){
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if(ch2 != PUZ_SPACE && p2 != *m_p2 || count(p2) != 0 ||
                boost::algorithm::any_of(offset, [&](const Position& os2){
                auto p3 = p2 + os2;
                char ch3 = m_game->cells(p3);
                return p3 != *m_p2 && count(p3) == 0 && ch3 != PUZ_SPACE && ch3 != PUZ_BOUNDARY;
            })) continue;
            children.push_back(*this);
            if(!children.back().make_move(p2))
                children.pop_back();
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
                m_pos2num[p] = n;
                m_start.push_back(ch_g++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for(auto& kv : m_pos2num){
        auto& p = kv.first;
        int n = kv.second;
        for(auto& kv2 : m_pos2num){
            auto p2 = kv2.first;
            if(p2 <= p) continue;
            int n3 = n + kv2.second;
            if(manhattan_distance(p, p2) + 1 > n3) continue;
            auto kv3 = make_pair(p, p2);
            auto& garden = m_pair2garden[kv3];
            garden.m_name = cells(p);
            garden.m_num = n3;
            puz_state2 sstart(*this, garden, p, p2);
            list<list<puz_state2>> spaths;
            puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
            for(auto& spath : spaths)
                garden.m_perms.push_back(spath.back());
            if(garden.m_perms.empty())
                m_pair2garden.erase(kv3);
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
    bool make_move(const Position& p, const Position& p2, int n);
    bool make_move2(const Position& p, const Position& p2, int n);
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
    map<Position, vector<pair<Position, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for(auto& kv : g.m_pair2garden){
        auto &p = kv.first.first, &p2 = kv.first.second;
        auto sz = kv.second.m_perms.size();
        auto& v = m_matches[p];
        auto& v2 = m_matches[p2];
        for(int i = 0; i < sz; i++){
            v.emplace_back(p2, i);
            v2.emplace_back(p, i);
        }
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
        auto& p = kv.first;
        auto& v = kv.second;
        boost::remove_erase_if(v, [&](auto& kv2){
            auto& p2 = kv2.first;
            auto& perm = m_game->m_pair2garden.at({min(p, p2), max(p, p2)}).m_perms[kv2.second];
            return boost::algorithm::any_of(perm, [&](auto& p3){
                return p3 != p && p3 != p2 && cells(p3) != PUZ_SPACE;
            });
        });
        for(auto& kv2 : v){
            auto& p2 = kv2.first;
            auto& perm = m_game->m_pair2garden.at({min(p, p2), max(p, p2)}).m_perms[kv2.second];
            for(auto& p3 : perm)
                rng.erase(p3);
        }

        if(!init)
            switch(v.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(p, v[0].first, v[0].second) ? 1 : 0;
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
    for(auto& p : smoves)
        a.erase(p);
    return boost::algorithm::all_of(a, [&](auto& p){
        return cells(p) == PUZ_SPACE;
    });
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

bool puz_state::make_move2(const Position& p, const Position& p2, int n)
{
    auto& garden = m_game->m_pair2garden.at({min(p, p2), max(p, p2)});
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

    for(auto& kv : m_matches){
        auto& p3 = kv.first;
        if(p3 == p || p3 == p2) continue;
        auto &v = kv.second;
        boost::remove_erase_if(v, [&](auto& kv2){
            auto &p4 = kv2.first;
            return p == p4 || p2 == p4;
        });
    }
    m_matches.erase(p); m_matches.erase(p2);
    m_distance += 2;

    return boost::algorithm::none_of(m_matches, [&](auto& kv){
        return kv.second.empty();
    }) && is_valid_move();
}

bool puz_state::make_move(const Position& p, const Position& p2, int n)
{
    m_distance = 0;
    if(!make_move2(p, p2, n))
        return false;
    int m;
    while((m = find_matches(false)) == 1);
    return m == 2 && is_valid_move();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](auto& kv1, auto& kv2){
        return kv1.second.size() < kv2.second.size();
    });
    for(auto& kv2 : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, kv2.first, kv2.second))
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
                auto it = m_game->m_pos2num.find(p);
                if(it == m_game->m_pos2num.end())
                    out << ". ";
                else
                    out << format("%-2d") % it->second;
            }
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Pairakabe2()
{
    using namespace puzzles::Pairakabe2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Pairakabe.xml", "Puzzles/Pairakabe2.txt", solution_format::GOAL_STATE_ONLY);
}
