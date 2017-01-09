#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 11/Tierra Del Fuego

    Summary
    Fuegians!

    Description
    1. The board represents the 'Tierra del Fuego' archipelago, where native
       tribes, the Fuegians, live.
    2. Being organized in tribes, each tribe, marked with a different letter,
       has occupied an island in the archipelago.
    3. The archipelago is peculiar because all bodies of water separating the
       islands are identical in shape and occupied a 2*1 or 1*2 space.
    4. These bodies of water can only touch diagonally.
    5. Your task is to find these bodies of water.
    6. Please note there are no hidden tribes or islands without a tribe on it.
*/

namespace puzzles{ namespace TierraDelFuego{

#define PUZ_SPACE        ' '
#define PUZ_ISLAND        '.'
#define PUZ_WATER        'W'

const Position offset[] = {
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
    set<char> m_tribes;
    vector<vector<Position>> m_water_info;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());

    auto f = [&](char ch, const Position& p1, const Position& p2){
        if(ch == PUZ_SPACE && cells(p2) == PUZ_SPACE)
            m_water_info.push_back({p1, p2});
    };
    for(int r = 0; r < m_sidelen; ++r)
        for(int c = 0; c < m_sidelen; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(r < m_sidelen - 1)
                f(ch, p, {r + 1, c});
            if(c < m_sidelen - 1)
                f(ch, p, {r, c + 1});
            if(ch != PUZ_SPACE)
                m_tribes.insert(ch);
        }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n);

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_tribes_index - m_game->m_tribes.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<int> m_waters;
    int m_tribes_index = 0;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_tribes_index(g.m_tribes.size() * g.m_tribes.size())
{
    m_waters.resize(g.m_water_info.size());
    boost::iota(m_waters, 0);
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a) : m_area(&a) { make_move(*a.begin()); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for(auto& os : offset){
        auto p = *this + os;
        if(m_area->count(p) != 0){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::make_move(int n)
{
    {
        auto& info = m_game->m_water_info[n];
        for(auto& p : info){
            cells(p) = PUZ_WATER;
            for(auto& os : offset){
                auto p2 = p + os;
                if(!is_valid(p2)) continue;
                char& ch = cells(p2);
                if(ch == PUZ_SPACE)
                    ch = PUZ_ISLAND;
            }
        }
    }

    boost::remove_erase_if(m_waters, [&](int id){
        auto& info = m_game->m_water_info[id];
        return boost::algorithm::any_of(info, [&](const Position& p){
            return this->cells(p) != PUZ_SPACE;
        });
    });

    set<Position> a;
    for(int i = 0; i < length(); ++i){
        char ch = (*this)[i];
        if(ch != PUZ_WATER && ch != PUZ_SPACE)
            a.insert({i / sidelen(), i % sidelen()});
    }
    while(!a.empty()){
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves(a, smoves);
        set<char> tribes;
        for(auto& p : smoves){
            a.erase(p);
            char ch = cells(p);
            if(isalpha(ch))
                tribes.insert(ch);
        }
        if(tribes.size() > 1)
            return false;
    }

    for(int i = 0; i < length(); ++i){
        char ch = (*this)[i];
        if(ch != PUZ_WATER)
            a.insert({i / sidelen(), i % sidelen()});
    }
    int tribes_index = 0;
    set<char> all_tribes;
    while(!a.empty()){
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves(a, smoves);
        set<char> tribes;
        for(auto& p : smoves){
            a.erase(p);
            char ch = cells(p);
            if(isalpha(ch)){
                if(all_tribes.count(ch) != 0)
                    return false;
                tribes.insert(ch);
            }
        }
        all_tribes.insert(tribes.begin(), tribes.end());
        if(tribes.empty())
            return false;
        int n = tribes.size();
        tribes_index += n * n;
    }
    m_distance = m_tribes_index - tribes_index;
    m_tribes_index = tribes_index;

    return is_goal_state() || !m_waters.empty();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for(int n : m_waters){
        children.push_back(*this);
        if(!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c){
            char ch = cells({r, c});
            out << (ch == PUZ_SPACE ? PUZ_ISLAND : ch) << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_TierraDelFuego()
{
    using namespace puzzles::TierraDelFuego;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TierraDelFuego.xml", "Puzzles/TierraDelFuego.txt", solution_format::GOAL_STATE_ONLY);
}
