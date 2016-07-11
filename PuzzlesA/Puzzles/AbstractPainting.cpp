#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Abstract Painting

    Summary
    Abstract Logic

    Description
    1. The goal is to reveal part of the abstract painting behind the board.
    2. Outer numbers tell how many tiles form the painting on the row and column.
    3. The region of the painting can be entirely hidden or revealed.
*/

namespace puzzles{ namespace AbstractPainting{

#define PUZ_PAINTING    'P'
#define PUZ_SPACE       '.'
#define PUZ_UNKNOWN     -1

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},         // n
    {0, 1},         // e
    {1, 0},         // s
    {0, 0},         // w
};

struct puz_region
{
    vector<Position> m_rng;
    map<int, int> m_rc2count;
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    vector<int> m_painting_counts;
    int m_painting_total_count;
    vector<puz_region> m_regions;
    map<Position, int> m_pos2region;
    map<pair<int, int>, int> m_rc_region2count;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(horz_walls), m_vert_walls(vert_walls) { make_move(p_start); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> &m_horz_walls, &m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for(int i = 0; i < 4; ++i){
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
        if(walls.count(p_wall) == 0){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size() / 2 - 1)
    , m_painting_counts(m_sidelen * 2)
{
    set<Position> rng;
    for(int r = 0;; ++r){
        // horz-walls
        auto& str_h = strs[r * 2];
        for(int c = 0; c < m_sidelen; ++c)
            if(str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if(r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for(int c = 0;; ++c){
            Position p(r, c);
            // vert-walls
            if(str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if(c == m_sidelen) break;
            rng.insert(p);
        }
    }

    auto f = [](const string& s) { return s == "  " ? PUZ_UNKNOWN : stoi(s); };
    for(int i = 0; i < m_sidelen; ++i){
        m_painting_counts[i] = f(strs[i * 2 + 1].substr(m_sidelen * 2 + 1, 2));
        m_painting_counts[i + m_sidelen] = f(strs[m_sidelen * 2 + 1].substr(i * 2, 2));
    }
    m_painting_total_count = boost::accumulate(m_painting_counts, 0) / 2;

    for(int n = 0; !rng.empty(); ++n){
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        puz_region region;
        for(auto& p : smoves){
            m_pos2region[p] = n;
            rng.erase(p);
            region.m_rng.push_back(p);
            ++region.m_rc2count[p.first];
            ++region.m_rc2count[p.second + m_sidelen];
        }
        m_regions.push_back(region);
    }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n);
    bool make_move2(int n);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_game->m_painting_total_count - boost::count(*this, PUZ_PAINTING);
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<int> m_painting_counts;
    // key: index of the row/column
    // value.elem: index of the region that can be revealed
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
    , m_painting_counts(g.m_painting_counts)
{
    for(int i = 0; i < g.m_regions.size(); ++i)
        for(auto& kv : g.m_regions[i].m_rc2count)
            m_matches[kv.first].push_back(i);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        int rc = kv.first;
        auto& region_ids = kv.second;

        boost::remove_erase_if(region_ids, [&](int id){
            return m_game->m_regions[id].m_rc2count.at(rc) > m_painting_counts[rc];
        });

        if(!init)
            switch(region_ids.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(*region_ids.begin()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int n)
{
    auto& region = m_game->m_regions[n];
    for(auto& p : region.m_rng)
        cells(p) = PUZ_PAINTING;
}

bool puz_state::make_move(int n)
{
    auto& region = m_game->m_regions[n];
    for(auto& p : region.m_rng)
        cells(p) = PUZ_PAINTING;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2){
        return kv1.second.size() < kv2.second.size();
    });
    for(int n : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0;; ++r){
        // draw horz-walls
        for(int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if(r == sidelen()) break;
        for(int c = 0;; ++c){
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
            if(c == sidelen()) break;
            out << cells(p);
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_AbstractPainting()
{
    using namespace puzzles::AbstractPainting;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\AbstractPainting.xml", "Puzzles\\AbstractPainting.txt", solution_format::GOAL_STATE_ONLY);
}
