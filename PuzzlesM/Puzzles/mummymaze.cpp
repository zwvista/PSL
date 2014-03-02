#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace mummymaze{

enum EMazeDir {mvNone, mvLeft, mvRight, mvUp, mvDown};
enum EMazeObject {moExplorer, moHorzCrab, moVertCrab, moHorzMummy, moVertMummy};

const Position offset[] = {
	{0, 0},
	{0, -1},
	{0, 1},
	{-1, 0},
	{1, 0},
};

typedef map<Position, EMazeObject> mm_obj_map;
typedef pair<Position, EMazeObject> mm_obj_pair;

struct mm_key_gate
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
	mm_obj_map m_obj_map;
	Position m_goal;
	boost::optional<mm_key_gate> m_key_gate;
	set<Position> m_horz_wall;
	set<Position> m_vert_wall;
	set<Position> m_skull;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	bool is_horz_wall(const Position& p) const {return m_horz_wall.count(p) != 0;}
	bool is_vert_wall(const Position& p) const {return m_vert_wall.count(p) != 0;}
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
	bool is_skull(const Position& p) const {return m_skull.count(p) != 0;}
	bool is_key(const Position& p) const {return m_key_gate && p == m_key_gate->m_key;}
	bool can_move(bool gate_open, Position& p, EMazeDir dir, bool is_man = false) const {
		Position p2 = p + offset[dir];
		if(dir == mvUp && is_horz_barrier(gate_open, p) ||
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

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_size(atoi(attrs.get<string>("rows").c_str()), atoi(attrs.get<string>("cols").c_str()))
{
	if(attrs.count("goal"))
		sscanf(attrs.get<string>("goal").c_str(), "(%d,%d)", &m_goal.first, &m_goal.second);
	if(attrs.count("key")){
		m_key_gate = mm_key_gate();
		sscanf(attrs.get<string>("key").c_str(), "(%d,%d)", &m_key_gate->m_key.first, &m_key_gate->m_key.second);
	}
	for(int r = 0; ; ++r){
		const string& hstr = strs[2 * r];
		for(size_t i = 0; i < hstr.length(); i++){
			Position p(r, i / 2);
			switch(hstr[i]){
			case '-': m_horz_wall.insert(p); break;
			case '=':
				if(!m_key_gate) m_key_gate = mm_key_gate();
				m_key_gate->m_vert = false;
				m_key_gate->m_gate = p;
				break;
			}
		}
		if(r == m_size.first) break;
		const string& vstr = strs[2 * r + 1];
		for(size_t i = 0; i < vstr.length(); i++){
			Position p(r, i / 2);
			switch(vstr[i]){
			case '|': m_vert_wall.insert(p); break;
			case 'E': m_man = p; break;
			case 'M': m_obj_map[p] = moHorzMummy; break;
			case 'N': m_obj_map[p] = moVertMummy; break;
			case 'C': m_obj_map[p] = moHorzCrab; break;
			case 'D': m_obj_map[p] = moVertCrab; break;
			case 'X': m_skull.insert(p); break;
			case 'G': m_goal = p; break;
			case 'K': 
				if(!m_key_gate) m_key_gate = mm_key_gate();
				m_key_gate->m_key = p;
				break;
			case '&':
				if(!m_key_gate) m_key_gate = mm_key_gate();
				m_key_gate->m_vert = true;
				m_key_gate->m_gate = p;
				break;
			}
		}
	}
}

struct puz_step
{
	Position m_pos;
	EMazeObject m_obj;
	EMazeDir m_dir;
	puz_step(const Position& pos, EMazeObject obj, EMazeDir dir)
		: m_pos(pos), m_obj(obj), m_dir(dir) {}
	bool operator==(const puz_step& x) const {
		return m_pos == x.m_pos && m_dir == x.m_dir;
	}
	bool operator<(const puz_step& x) const {
		return m_pos < x.m_pos || m_pos == x.m_pos &&  m_dir < x.m_dir;
	}
};

ostream& operator<<(ostream& out, const puz_step& mi)
{
	char* dirs = "WLRUD";
	out << format("%-10s") % (mi.m_obj == moExplorer ? "Explorer:" : 
		mi.m_obj <= moVertCrab ? "Crab:" : "Mummy:");
	Position pos2 = mi.m_pos - offset[mi.m_dir];
	out << " " << pos2 << " " << dirs[mi.m_dir] << " " << mi.m_pos << endl;
	return out;
}

ostream& operator<<(ostream& out, const list<puz_step>& m_move)
{
	for(const puz_step& mi : m_move)
		out << mi;
	return out;
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g)
		: m_game(&g), m_man(g.m_man), m_obj_map(g.m_obj_map), m_gate_open(false) {}
	bool make_move(EMazeDir dir);
	bool operator==(const puz_state& x) const {
		return m_man == x.m_man && m_gate_open == x.m_gate_open && m_obj_map == x.m_obj_map && m_move == x.m_move;
	}
	bool operator<(const puz_state& x) const {
		return m_man < x.m_man || m_man == x.m_man && m_gate_open < x.m_gate_open ||
			m_man == x.m_man && m_gate_open == x.m_gate_open && m_obj_map < x.m_obj_map;
	}

	// solve_puzzle interface
	bool is_goal_state() const {return m_man == m_game->m_goal;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {return manhattan_distance(m_man, m_game->m_goal);}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {for(const puz_step& m : m_move) out << m;}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	Position m_man;
	mm_obj_map m_obj_map;
	bool m_gate_open;
	list<puz_step> m_move;
};

bool puz_state::make_move(EMazeDir dir)
{
	if(!m_game->can_move(m_gate_open, m_man, dir, true) || m_obj_map.count(m_man) != 0) return false;
	if(dir != mvNone && m_game->is_key(m_man)) m_gate_open = !m_gate_open;
	m_move.clear();
	m_move.push_back(puz_step(m_man, moExplorer, dir));

	mm_obj_map obj_map2;
	for(int k = 0; k < 2; ++k){
		while(!m_obj_map.empty()){
			mm_obj_pair obj_pair = *m_obj_map.begin();
			m_obj_map.erase(m_obj_map.begin());
			Position pos = obj_pair.first;
			EMazeObject obj = obj_pair.second;
			EMazeDir dir = mvNone;
			if(!(k == 1 && (obj == moHorzCrab || obj == moVertCrab)) &&
				(obj == moHorzCrab || obj == moHorzMummy ?
				m_game->can_move_horz(m_man, m_gate_open, pos, dir) ||
				m_game->can_move_vert(m_man, m_gate_open, pos, dir) :
				m_game->can_move_vert(m_man, m_gate_open, pos, dir) ||
				m_game->can_move_horz(m_man, m_gate_open, pos, dir))){
				if(pos == m_man) return false;
				if(m_game->is_key(pos)) m_gate_open = !m_gate_open;
				m_move.push_back(puz_step(pos, obj, dir));
			}
			mm_obj_map::iterator it = obj_map2.find(pos);
			if(it == obj_map2.end())
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
	for(int i = 0; i < 5; i++){
		children.push_back(*this);
		puz_state& state = children.back();
		if(!state.make_move(EMazeDir(i)) || i == 0 && state == *this)
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	char* objs = "ECDMN";
	dump_move(out);
	const puz_game& g = *m_game;
	int rows = g.m_size.first;
	int cols = g.m_size.second;
	for(int r = 0; ; ++r){
		// draw horz wall
		for(int c = 0; c < cols; ++c){
			Position pos(r, c);
			out << (g.is_horz_gate(pos) ? m_gate_open ? "  " : " =" :
				g.is_horz_wall(pos) ? " -" : "  ");
		}
		out << endl;
		if(r == rows) break;
		for(int c = 0; ; ++c){
			Position pos(r, c);
			// draw vert wall
			out << (g.is_vert_gate(pos) ? m_gate_open ? " " : "&" :
				g.is_vert_wall(pos) ? "|" : " ");
			if(c == cols) break;
			// draw man, mummy, crab or space
			out << (pos == m_man ? 'E' :
				m_obj_map.count(pos) != 0 ? objs[m_obj_map.find(pos)->second] :
				g.is_skull(pos) ? 'X' :
				g.is_key(pos) ? 'K' :
				pos == g.m_goal ? 'G' : ' ');
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_mummymaze()
{
	using namespace puzzles::mummymaze;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\mummymaze.xml", "Puzzles\\mummymaze.txt");
}