#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 13/Robot Fences

	Summary
	BZZZZliip ...cows?

	Description
	1. A bit like Robot Crosswords, you need to fill each region with a
	   randomly ordered sequence of numbers.
	2. Numbers can only be in range 1 to N where N is the board size.
	3. No same number can appear in the same row or column.
*/

namespace puzzles{ namespace RobotFences{

#define PUZ_SPACE		' '

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const Position offset2[] = {
	{0, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, 0},		// w
};

struct puz_area
{
	vector<Position> m_range;
	vector<vector<int>> m_perms;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<puz_area> m_areas;
	map<Position, int> m_pos2num;
	set<Position> m_horz_walls, m_vert_walls;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

struct puz_state2 : Position
{
	puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
		: m_horz_walls(horz_walls), m_vert_walls(vert_walls) {
		make_move(p_start);
	}

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
	, m_sidelen(strs.size() / 2)
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
			char ch = str_v[c * 2 + 1];
			if(ch != PUZ_SPACE)
				m_pos2num[p] = ch - '0';
			rng.insert(p);
		}
	}

	for(int n = 0; !rng.empty(); ++n){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
		m_areas.resize(m_sidelen * 2 + n + 1);
		for(auto& p : smoves){
			m_areas[p.first].m_range.push_back(p);
			m_areas[m_sidelen + p.second].m_range.push_back(p);
			m_areas[m_sidelen * 2 + n].m_range.push_back(p);
			rng.erase(p);
		}
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
	bool make_move(const Position& p, char ch);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {return boost::range::count(*this, PUZ_SPACE);}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
}

bool puz_state::make_move(const Position& p, char ch)
{
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
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

void solve_puz_RobotFences()
{
	using namespace puzzles::RobotFences;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
		("Puzzles\\RobotFences.xml", "Puzzles\\RobotFences.txt", solution_format::GOAL_STATE_ONLY);
}