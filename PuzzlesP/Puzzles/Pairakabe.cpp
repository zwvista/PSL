#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
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

namespace puzzles{ namespace Pairakabe{

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
    set<Position> m_inner, m_outer;
    int m_remaining;
    bool m_paired = false;

    puz_garden() {}
    puz_garden(const Position& p, int cnt) : m_remaining(cnt){
        m_inner.insert(p);
    }
    bool operator<(const puz_garden& x) const {
        auto f = [](int n){ return n == 0 ? 100 : n; };
        return f(m_outer.size()) < f(x.m_outer.size());
    }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<char, puz_garden> m_ch2garden;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
    char ch_g = 'A';
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
                m_ch2garden[ch_g] = {p, n - 1};
                m_start.push_back(ch_g++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(char ch, const Position& p);
    bool make_move2(char ch, const Position& p);
    int adjust_area(bool init);
    void check_board();
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ch2garden, 0, [](int acc, const pair<const char, puz_garden>& kv){
            return acc + kv.second.m_remaining;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<char, puz_garden> m_ch2garden;
    vector<set<Position>> m_2by2walls;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_ch2garden(g.m_ch2garden)
{
    for(int r = 1; r < g.m_sidelen - 2; ++r)
        for(int c = 1; c < g.m_sidelen - 2; ++c){
            set<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
            if(boost::algorithm::all_of(rng, [&](const Position& p){
                return cells(p) == PUZ_SPACE;
            }))
                m_2by2walls.push_back(rng);
        }

    adjust_area(true);
}

void puz_state::check_board()
{
    for(int r = 1; r < sidelen() - 1; ++r)
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            if(cells(p) != PUZ_SPACE) continue;
            set<char> chars;
            int n = 0;
            for(auto& os : offset){
                char ch = cells(p + os);
                if(ch != PUZ_BOUNDARY && ch != PUZ_SPACE && ch != PUZ_WALL &&
                    chars.count(ch) == 0){
                    chars.insert(ch);
                    n += m_ch2garden.at(ch).m_paired ? 2 : 1;
                }
            }
            if(n > 2)
                cells(p) = PUZ_WALL;
        }
}

int puz_state::adjust_area(bool init)
{
    for(auto& kv : m_ch2garden){
        char ch = kv.first;
        auto& g = kv.second;
        auto& outer = g.m_outer;
        outer.clear();
        if(g.m_remaining == 0) continue;

        for(auto& p : g.m_inner)
            for(auto& os : offset){
                auto p2 = p + os;
                if(cells(p2) == PUZ_SPACE)
                    outer.insert(p2);
            }

        if(!init)
            switch(outer.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(ch, *outer.begin()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
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

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

bool puz_state::make_move2(char ch, const Position& p)
{
    auto& g = m_ch2garden.at(ch);
    cells(p) = ch;
    g.m_inner.insert(p);
    for(auto& os : offset){
        char ch2 = cells(p + os);
        if(ch2 != PUZ_BOUNDARY && ch2 != PUZ_SPACE && ch2 != PUZ_WALL && ch2 != ch){
            auto& g2 = m_ch2garden.at(ch2);
            g.m_paired = true;
            for(auto& p2 : g2.m_inner)
                cells(p2) = ch;
            g.m_inner.insert(g2.m_inner.begin(), g2.m_inner.end());
            g.m_remaining += g2.m_remaining;
            m_ch2garden.erase(ch2);
        }
    }

    if(--g.m_remaining == 0 && g.m_paired){
        for(auto& p2 : g.m_inner)
            for(auto& os : offset){
                char& ch2 = cells(p2 + os);
                if(ch2 == PUZ_SPACE)
                    ch2 = PUZ_WALL;
            }
        m_ch2garden.erase(ch);
    }
    ++m_distance;

    boost::remove_erase_if(m_2by2walls, [&](const set<Position>& rng){
        return rng.count(p) != 0;
    });

    // pruning
    if(boost::algorithm::any_of(m_ch2garden, [&](const pair<const char, puz_garden>& kv){
        auto& g = kv.second;
        return !g.m_paired && boost::algorithm::none_of(kv.second.m_inner, [&](const Position& p2){
            return boost::algorithm::any_of(m_ch2garden, [&](const pair<const char, puz_garden>& kv2){
                auto& g2 = kv2.second;
                return kv.first != kv2.first && !g2.m_paired &&
                    boost::algorithm::any_of(g2.m_inner, [&](const Position& p3){
                    return manhattan_distance(p2, p3) <= g.m_remaining + g2.m_remaining + 1;
                });
            });
        });
    }))
        return false;

    check_board();
    return !is_goal_state() || m_2by2walls.empty() && is_continuous();
}

bool puz_state::make_move(char ch, const Position& p)
{
    m_distance = 0;
    if(!make_move2(ch, p))
        return false;
    int m;
    while((m = adjust_area(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_ch2garden, [](
        const pair<const char, puz_garden>& kv1,
        const pair<const char, puz_garden>& kv2){
        return kv1.second < kv2.second;
    });
    for(auto& p : kv.second.m_outer){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_SPACE || ch == PUZ_WALL)
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

void solve_puz_Pairakabe()
{
    using namespace puzzles::Pairakabe;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\Pairakabe.xml", "Puzzles\\Pairakabe.txt", solution_format::GOAL_STATE_ONLY);
}