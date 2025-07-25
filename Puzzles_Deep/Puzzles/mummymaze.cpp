#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Mummy Maze
*/

namespace puzzles::mummymaze{

enum EMazeDir {mvNone, mvLeft, mvRight, mvUp, mvDown};
enum EMazeObject {moExplorer, moHorzCrab, moVertCrab, moHorzMummy, moVertMummy};

constexpr Position offset[] = {
    {0, 0},
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

using puz_obj_map = map<Position, EMazeObject>;
using puz_obj_pair = pair<Position, EMazeObject>;

struct puz_key_gate
{
    Position m_key;
    Position m_gate;
    bool m_vert;
};

struct puz_game
{
    string m_id;
    Position m_size;
    Position m_man;
    puz_obj_map m_obj_map;
    Position m_goal;
    boost::optional<puz_key_gate> m_key_gate;
    set<Position> m_horz_wall;
    set<Position> m_vert_wall;
    set<Position> m_skull;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_horz_wall(const Position& p) const {return m_horz_wall.contains(p);}
    bool is_vert_wall(const Position& p) const {return m_vert_wall.contains(p);}
    bool is_horz_gate(const Position& p) const {
        return m_key_gate && !m_key_gate->m_vert && p == m_key_gate->m_gate;
    }
    bool is_vert_gate(const Position& p) const {
        return m_key_gate && m_key_gate->m_vert && p == m_key_gate->m_gate;
    }
    bool is_horz_barrier(bool gate_open, const Position& p) const {
        return !gate_open && is_horz_gate(p) || is_horz_wall(p);
    }
    bool is_vert_barrier(bool gate_open, const Position& p) const {
        return !gate_open && is_vert_gate(p) || is_vert_wall(p);
    }
    bool is_skull(const Position& p) const {return m_skull.contains(p);}
    bool is_key(const Position& p) const {return m_key_gate && p == m_key_gate->m_key;}
    bool can_move(bool gate_open, Position& p, EMazeDir dir, bool is_man = false) const {
        Position p2 = p + offset[dir];
        if (dir == mvUp && is_horz_barrier(gate_open, p) ||
            dir == mvLeft && is_vert_barrier(gate_open, p) ||
            dir == mvDown && is_horz_barrier(gate_open, p2) ||
            dir == mvRight && is_vert_barrier(gate_open, p2) ||
            is_man && is_skull(p2))
            return false;
        p = p2;
        return true;
    }
    bool can_move_horz(Position& man, bool gate_open, Position& p, EMazeDir& dir) const {
        return p.second < man.second ? can_move(gate_open, p, dir = mvRight) :
            p.second > man.second ? can_move(gate_open, p, dir = mvLeft) : false;
    }
    bool can_move_vert(Position& man, bool gate_open, Position& p, EMazeDir& dir) const {
        return p.first < man.first ? can_move(gate_open, p, dir = mvDown) :
            p.first > man.first ? can_move(gate_open, p, dir = mvUp) : false;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(atoi(level.attribute("rows").value()), atoi(level.attribute("cols").value()))
{
    if (!level.attribute("goal").empty())
        sscanf(level.attribute("goal").value(), "(%d,%d)", &m_goal.first, &m_goal.second);
    if (!level.attribute("key").empty()) {
        m_key_gate = puz_key_gate();
        sscanf(level.attribute("key").value(), "(%d,%d)", &m_key_gate->m_key.first, &m_key_gate->m_key.second);
    }
    for (int r = 0; ; ++r) {
        string_view hstr = strs[2 * r];
        for (size_t i = 0; i < hstr.length(); i++)
            switch(Position p(r, i / 2); hstr[i]) {
            case '-': m_horz_wall.insert(p); break;
            case '=':
                if (!m_key_gate) m_key_gate = puz_key_gate();
                m_key_gate->m_vert = false;
                m_key_gate->m_gate = p;
                break;
            }
        if (r == m_size.first) break;
        string_view vstr = strs[2 * r + 1];
        for (size_t i = 0; i < vstr.length(); i++)
            switch(Position p(r, i / 2); vstr[i]) {
            case '|': m_vert_wall.insert(p); break;
            case 'E': m_man = p; break;
            case 'M': m_obj_map[p] = moHorzMummy; break;
            case 'N': m_obj_map[p] = moVertMummy; break;
            case 'C': m_obj_map[p] = moHorzCrab; break;
            case 'D': m_obj_map[p] = moVertCrab; break;
            case 'X': m_skull.insert(p); break;
            case 'G': m_goal = p; break;
            case 'K': 
                if (!m_key_gate) m_key_gate = puz_key_gate();
                m_key_gate->m_key = p;
                break;
            case '&':
                if (!m_key_gate) m_key_gate = puz_key_gate();
                m_key_gate->m_vert = true;
                m_key_gate->m_gate = p;
                break;
            }
    }
}

struct puz_move
{
    Position m_pos;
    EMazeObject m_obj;
    EMazeDir m_dir;
    puz_move(const Position& pos, EMazeObject obj, EMazeDir dir)
        : m_pos(pos), m_obj(obj), m_dir(dir) {}
    bool operator==(const puz_move& x) const {
        return m_pos == x.m_pos && m_dir == x.m_dir;
    }
    bool operator<(const puz_move& x) const {
        return m_pos < x.m_pos || m_pos == x.m_pos &&  m_dir < x.m_dir;
    }
};

ostream& operator<<(ostream& out, const puz_move& mi)
{
    const string_view dirs = "WLRUD";
    out << format("{:<10}", mi.m_obj == moExplorer ? "Explorer:" : 
        mi.m_obj <= moVertCrab ? "Crab:" : "Mummy:");
    Position pos2 = mi.m_pos - offset[mi.m_dir];
    out << " " << pos2 << " " << dirs[mi.m_dir] << " " << mi.m_pos << endl;
    return out;
}

ostream& operator<<(ostream& out, const list<puz_move>& m_move)
{
    for (const puz_move& mi : m_move)
        out << mi;
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_game(&g), m_man(g.m_man), m_obj_map(g.m_obj_map), m_gate_open(false) {}
    bool make_move(EMazeDir dir);
    bool operator==(const puz_state& x) const {
        return tie(m_man, m_gate_open, m_obj_map, m_move) == tie(x.m_man, x.m_gate_open, x.m_obj_map, x.m_move);
    }
    bool operator<(const puz_state& x) const {
        return tie(m_man, m_gate_open, m_obj_map) < tie(x.m_man, x.m_gate_open, x.m_obj_map);
    }

    // solve_puzzle interface
    bool is_goal_state() const {return m_man == m_game->m_goal;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return manhattan_distance(m_man, m_game->m_goal);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {for(const puz_move& m : m_move) out << m;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    Position m_man;
    puz_obj_map m_obj_map;
    bool m_gate_open;
    list<puz_move> m_move;
};

bool puz_state::make_move(EMazeDir dir)
{
    if (!m_game->can_move(m_gate_open, m_man, dir, true) || m_obj_map.contains(m_man)) return false;
    if (dir != mvNone && m_game->is_key(m_man)) m_gate_open = !m_gate_open;
    m_move.clear();
    m_move.push_back(puz_move(m_man, moExplorer, dir));

    puz_obj_map obj_map2;
    for (int k = 0; k < 2; ++k) {
        while (!m_obj_map.empty()) {
            auto it = m_obj_map.begin();
            auto [pos, obj] = static_cast<pair<Position, EMazeObject>>(*it);
            m_obj_map.erase(it);
            EMazeDir dir = mvNone;
            if (!(k == 1 && (obj == moHorzCrab || obj == moVertCrab)) &&
                (obj == moHorzCrab || obj == moHorzMummy ?
                m_game->can_move_horz(m_man, m_gate_open, pos, dir) ||
                m_game->can_move_vert(m_man, m_gate_open, pos, dir) :
                m_game->can_move_vert(m_man, m_gate_open, pos, dir) ||
                m_game->can_move_horz(m_man, m_gate_open, pos, dir))) {
                if (pos == m_man) return false;
                if (m_game->is_key(pos)) m_gate_open = !m_gate_open;
                m_move.push_back(puz_move(pos, obj, dir));
            }
            if (auto it = obj_map2.find(pos); it == obj_map2.end())
                obj_map2[pos] = obj;
            else
                it->second = max(it->second, obj);
        }
        swap(m_obj_map, obj_map2);
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 5; i++) {
        children.push_back(*this);
        puz_state& state = children.back();
        if (!state.make_move(EMazeDir(i)) || i == 0 && state == *this)
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    const string_view objs = "ECDMN";
    dump_move(out);
    const puz_game& g = *m_game;
    int rows = g.m_size.first;
    int cols = g.m_size.second;
    for (int r = 0; ; ++r) {
        // draw horz wall
        for (int c = 0; c < cols; ++c) {
            Position pos(r, c);
            out << (g.is_horz_gate(pos) ? m_gate_open ? "  " : " =" :
                g.is_horz_wall(pos) ? " -" : "  ");
        }
        println(out);
        if (r == rows) break;
        for (int c = 0; ; ++c) {
            Position pos(r, c);
            // draw vert wall
            out << (g.is_vert_gate(pos) ? m_gate_open ? " " : "&" :
                g.is_vert_wall(pos) ? "|" : " ");
            if (c == cols) break;
            // draw man, mummy, crab or space
            out << (pos == m_man ? 'E' :
                m_obj_map.contains(pos) ? objs[m_obj_map.find(pos)->second] :
                g.is_skull(pos) ? 'X' :
                g.is_key(pos) ? 'K' :
                pos == g.m_goal ? 'G' : ' ');
        }
        println(out);
    }
    return out;
}

}

void solve_puz_mummymaze()
{
    using namespace puzzles::mummymaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/mummymaze.xml", "Puzzles/mummymaze.txt");
}
