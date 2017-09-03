#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 15/Over Under

    Summary
    Over and Under regions

    Description
    1. Divide the board in regions following these rules:
    2. Each region must contain two numbers.
    3. The region size must be between the two numbers.
*/

namespace puzzles{ namespace OverUnder{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_WALL         'W'
#define PUZ_BOUNDARY     '+'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};
const Position offset2[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
};

struct puz_region
{
    // character that represents the region. 'a', 'b' and so on
    char m_name;
    // key: number of the tiles occupied by the region
    // value: all permutations (forms) of the region
    map<int, set<Position>> m_num2perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    // key: positions of the two numbers (hints)
    map<pair<Position, Position>, puz_region> m_pair2region;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game& game, const Position& p, const Position& p2, int num)
        : m_game(&game), m_p2(&p2), m_num(num) {make_move(p);}

    bool is_goal_state() const { return m_distance == m_num; }
    bool make_move(const Position& p) {
        insert(p); ++m_distance;
        return boost::algorithm::any_of(*this, [&](const Position& p2){
            // cannot go too far away
            return manhattan_distance(p2, *m_p2) <= m_num - m_distance;
        });
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const Position* m_p2;
    int m_num;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for(auto& p : *this)
        for(auto& os : offset){
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if(ch2 != PUZ_SPACE && p2 != *m_p2 || count(p2) != 0) continue;
            children.push_back(*this);
            if(!children.back().make_move(p2))
                children.pop_back();
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_r = 'a';
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
                m_start.push_back(ch_r++);
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
            int n2 = kv2.second;
            if(n > n2) swap(n, n2);
            // The region size must be between the two numbers.
            // cannot make a pairing with a tile too far away
            if(n2 - n < 2 || manhattan_distance(p, p2) + 1 >= n2) continue;
            auto kv3 = make_pair(p, p2);
            auto& region = m_pair2region[kv3];
            region.m_name = cells(p);
            for(int i = n + 1; i < n2; ++i){
                puz_state2 sstart(*this, p, p2, i);
                list<list<puz_state2>> spaths;
                puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths);
                for(auto& spath : spaths)
                    region.m_num2perms[i] = spath.back();
            }
            if(region.m_num2perms.empty())
                m_pair2region.erase(kv3);
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
    bool make_move2(Position p, Position p2, int n);
    int find_matches(bool init);
    bool is_continuous() const;
    bool check_2x2();
    bool is_valid_move() { return check_2x2() && is_continuous(); }

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
    for(auto& kv : g.m_pair2region){
        auto &p = kv.first.first, &p2 = kv.first.second;
        auto sz = kv.second.m_num2perms.size();
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
    //map<Position, set<pair<Position, Position>>> space2hints;
    //for(int r = 1; r < sidelen() - 1; ++r)
    //    for(int c = 1; c < sidelen() - 1; ++c){
    //        Position p(r, c);
    //        char ch = cells(p);
    //        if(ch == PUZ_SPACE || ch == PUZ_EMPTY)
    //            space2hints[p];
    //    }

    //for(auto& kv : m_matches){
    //    auto& p = kv.first;
    //    auto& v = kv.second;
    //    boost::remove_erase_if(v, [&](auto& kv2){
    //        auto& p2 = kv2.first;
    //        auto& perm = m_game->m_pair2region.at({min(p, p2), max(p, p2)}).m_num2perms[kv2.second];
    //        return boost::algorithm::any_of(perm, [&](auto& p3){
    //            char ch = cells(p3);
    //            return p3 != p && p3 != p2 && ch != PUZ_SPACE && ch != PUZ_EMPTY;
    //        });
    //    });
    //    for(auto& kv2 : v){
    //        auto& p2 = kv2.first;
    //        auto& perm = m_game->m_pair2region.at({min(p, p2), max(p, p2)}).m_num2perms[kv2.second];
    //        //for(auto& p3 : perm)
    //        //    space2hints[p3].emplace(min(p, p2), max(p, p2));
    //    }

    //    if(!init)
    //        switch(v.size()){
    //        case 0:
    //            return 0;
    //        case 1:
    //            return make_move2(p, v[0].first, v[0].second) ? 1 : 0;
    //        }
    //}
    //bool changed = false;
    //for(auto& kv : space2hints){
    //    const auto& p = kv.first;
    //    auto& h = kv.second;
    //    if(!h.empty()) continue;
    //    // Cells that cannot be reached by any region can be nothing but a wall
    //    char& ch = cells(p);
    //    if(ch == PUZ_EMPTY)
    //        return false;
    //    ch = PUZ_WALL;
    //    changed = true;
    //}
    //if(changed){
    //    if(!check_2x2()) return 0;
    //    for(auto& kv : space2hints){
    //        const auto& p = kv.first;
    //        auto& h = kv.second;
    //        if(h.size() != 1) continue;
    //        char ch = cells(p);
    //        if(ch == PUZ_SPACE) continue;
    //        // Cells that can be reached by only one region
    //        auto& kv2 = *h.begin();
    //        auto& perms = m_game->m_pair2region.at(kv2).m_num2perms;
    //        boost::remove_erase_if(m_matches.at(kv2.first), [&](auto& kv3){
    //            return kv3.first == kv2.second && boost::algorithm::none_of_equal(perms[kv3.second], p);
    //        });
    //        boost::remove_erase_if(m_matches.at(kv2.second), [&](auto& kv3){
    //            return kv3.first == kv2.first && boost::algorithm::none_of_equal(perms[kv3.second], p);
    //        });
    //    }
    //    if(!init){
    //        for(auto& kv : m_matches){
    //            const auto& p = kv.first;
    //            auto& v = kv.second;
    //            if(v.size() == 1)
    //                return make_move2(p, v[0].first, v[0].second) ? 1 : 0;
    //        }
    //        if(!is_continuous())
    //            return 0;
    //    }
    //}
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

bool puz_state::check_2x2()
{
    // The wall can't form 2*2 squares.
    for(int r = 1; r < sidelen() - 2; ++r)
        for(int c = 1; c < sidelen() - 2; ++c){
            Position p(r, c);
            vector<Position> rng1, rng2;
            for(auto& os : offset2){
                auto p2 = p + os;
                switch(cells(p2)){
                case PUZ_WALL: rng1.push_back(p2); break;
                case PUZ_SPACE: rng2.push_back(p2); break;
                }
            }
            if(rng1.size() == 4) return false;
            if(rng1.size() == 3 && rng2.size() == 1)
                cells(rng2[0]) = PUZ_EMPTY;
        }
    return true;
}

bool puz_state::make_move2(Position p, Position p2, int n)
{
    //auto& region = m_game->m_pair2region.at({min(p, p2), max(p, p2)});
    //auto& perm = region.m_num2perms[n];

    //for(auto& p2 : perm)
    //    cells(p2) = region.m_name;
    //for(auto& p2 : perm)
    //    // regions are separated by a wall.
    //    for(auto& os : offset)
    //        switch(char& ch2 = cells(p2 + os)){
    //        case PUZ_SPACE: ch2 = PUZ_WALL; break;
    //        case PUZ_EMPTY: return false;
    //        }

    //for(auto& kv : m_matches){
    //    auto& p3 = kv.first;
    //    if(p3 == p || p3 == p2) continue;
    //    auto &v = kv.second;
    //    boost::remove_erase_if(v, [&](auto& kv2){
    //        auto &p4 = kv2.first;
    //        return p == p4 || p2 == p4;
    //    });
    //}
    //m_matches.erase(p);
    //m_matches.erase(p2);
    //m_distance += 2;

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
    return m == 2;
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

void solve_puz_OverUnder()
{
    using namespace puzzles::OverUnder;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/OverUnder.xml", "Puzzles/OverUnder.txt", solution_format::GOAL_STATE_ONLY);
}
