#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Matchmania
*/

namespace puzzles{ namespace Matchmania{

#define PUZ_SPACE        ' '
#define PUZ_HOLE         'O'
#define PUZ_STONE        'S'
#define PUZ_MUSHROOM     'm'

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

struct puz_bunny_info
{
    char m_bunny_name, m_food_name;
	Position m_bunny;
	set<Position> m_food;
    int steps() const { return m_food.size() + 1; }
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    map<char, puz_bunny_info> m_bunny2info;
    set<Position> m_holes, m_mushrooms;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
	, m_size(strs.size() / 2, strs[0].size() / 2)
{
    for(int r = 0;; ++r){
        // horz-walls
        auto& str_h = strs[r * 2];
        for(int c = 0; c < cols(); ++c)
            if(str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if(r == rows()) break;
        auto& str_v = strs[r * 2 + 1];
        for(int c = 0;; ++c){
            Position p(r, c);
            if(str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if(c == cols()) break;
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
            switch(ch){
            case 'C': // Calvin
            case 'T': // Otto
            case 'P': // Peanut
            case 'D': // Daisy
            {
                auto& info = m_bunny2info[ch];
                info.m_bunny_name = ch;
                info.m_bunny = p;
                break;
            }
            case 'c':
            case 't':
            case 'p':
            case 'd':
            {
                auto& info = m_bunny2info[toupper(ch)];
                info.m_food_name = ch;
                info.m_food.insert(p);
                break;
            }
            case 'm':
                m_mushrooms.insert(p);
        		break;
            case PUZ_HOLE:
            	m_holes.insert(p);
        		break;
            }
        }
    }
}

struct puz_step : pair<Position, Position>
{
    using pair::pair;
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: %1% -> %2%\n") % mi.first % mi.second;
    return out;
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p1, const Position& p2);

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { 
        return boost::accumulate(m_bunny2info, 0, [](int acc, const pair<const char, puz_bunny_info>& kv){
            return acc + kv.second.steps();
        }) + m_mushrooms.size();
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const { if(m_move) out << *m_move; }
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<char, puz_bunny_info> m_bunny2info;
    set<Position> m_mushrooms;
    char m_curr_bunny = 0;
    unsigned int m_distance = 0;
    boost::optional<puz_step> m_move;
};

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info) {
        make_move(info.m_bunny);
    }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    char ch = m_state->cells(*this);
    if(ch == m_info->m_food_name || ch == PUZ_MUSHROOM) return;
    for(int i = 0; i < 4; ++i){
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_state->m_game->m_horz_walls : m_state->m_game->m_vert_walls;
        if(walls.count(p_wall) != 0) continue;
        ch = m_state->cells(p);
        if(ch == m_info->m_bunny_name || ch == PUZ_SPACE ||
            ch == m_info->m_food_name || ch == PUZ_MUSHROOM){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

struct puz_state3 : Position
{
    puz_state3(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info) { make_move(info.m_bunny); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    char ch = m_state->cells(*this);
    if(ch == PUZ_HOLE) return;
    for(int i = 0; i < 4; ++i){
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_state->m_game->m_horz_walls : m_state->m_game->m_vert_walls;
        if(walls.count(p_wall) != 0) continue;
        ch = m_state->cells(p);
        if(ch == m_info->m_bunny_name || ch == PUZ_HOLE ||
           ch == m_info->m_food_name || ch == PUZ_MUSHROOM){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
, m_bunny2info(g.m_bunny2info), m_mushrooms(g.m_mushrooms)
{
}
    
bool puz_state::make_move(const Position& p1, const Position& p2)
{
    char &ch1 = cells(p1), &ch2 = cells(p2);
    auto& info = m_bunny2info.at(ch1);
    if(ch2 == PUZ_HOLE){
        if(!info.m_food.empty())
            return false;
        m_bunny2info.erase(ch1);
        if(m_bunny2info.empty() && !m_mushrooms.empty())
            return false;
        ch1 = PUZ_SPACE;
        m_curr_bunny = 0;
    }
    else{
        info.m_food.erase(info.m_bunny = p2);
        m_curr_bunny = ch2 = exchange(ch1, PUZ_SPACE);
    }

    // pruning
    for(auto& kv : m_bunny2info){
        auto& info = kv.second;
        list<puz_state3> smoves;
        puz_move_generator<puz_state3>::gen_moves({*this, info}, smoves);
        // 1. The bunny must reach one of the holes after taking all its own food.
        // 2. The bunny must take all mushrooms if he is the last bunny.
        if(boost::algorithm::none_of(smoves, [&](const Position& p){
            return cells(p) == PUZ_HOLE;
        }) || boost::count_if(smoves, [&](const Position& p){
            return cells(p) == info.m_food_name;
        }) != info.m_food.size() || m_bunny2info.size() == 1 &&
            boost::count_if(smoves, [&](const Position& p){
            return cells(p) == PUZ_MUSHROOM;
        }) != m_mushrooms.size())
            return false;
    }

    m_move = puz_step(p1, p2);
    m_distance = 1;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if(m_curr_bunny == 0)
        for(auto& kv : m_bunny2info){
            auto& info = kv.second;
            list<puz_state2> smoves;
            puz_move_generator<puz_state2>::gen_moves({*this, info}, smoves);
            smoves.remove_if([&](const Position& p){
                return info.m_food.count(p) == 0;
            });
            for(auto& p : smoves){
                children.push_back(*this);
                if(!children.back().make_move(info.m_bunny, p))
                    children.pop_back();
            }
        }
    else{
        auto& info = m_bunny2info.at(m_curr_bunny);
        auto& p1 = info.m_bunny;
        for(int i = 0; i < 4; ++i){
            auto p_wall = p1 + offset2[i];
            auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
            if(walls.count(p_wall) != 0) continue;
            auto p2 = p1 + offset[i];
            char ch = cells(p2);
            if(ch == info.m_food_name || ch == PUZ_MUSHROOM){
                children.push_back(*this);
                if(!children.back().make_move(p1, p2))
                    children.pop_back();
            }
            else if(ch == PUZ_HOLE){
                children.push_back(*this);
                if(!children.back().make_move(p1, p2))
                    children.pop_back();
            }
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for(int r = 0;; ++r){
        // draw horz-walls
        for(int c = 0; c < cols(); ++c)
            out << (m_game->m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if(r == rows()) break;
        for(int c = 0;; ++c){
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
            if(c == cols()) break;
            out << cells(p);
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Matchmania()
{
    using namespace puzzles::Matchmania;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Matchmania.xml", "Puzzles/Matchmania.txt");
}
