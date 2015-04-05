#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 13/Tatamino

	Summary
	Which is a little Tatami

	Description
	1. Plays like Fillomino, in which you have to guess areas on the board
	   marked by their number.
	2. In Tatamino, however, you only have areas 1, 2 and 3 tiles big.
	3. Please remember two areas of the same number size can't be touching.
*/

namespace puzzles{ namespace Tatamino2{

#define PUZ_SPACE		' '

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	m_start = boost::accumulate(strs, string());
}

struct puz_area
{
	set<Position> m_inner, m_outer;
	int m_cell_count;
	bool m_ready = false;
	bool operator<(const puz_area& x) const {
		return make_pair(m_ready, m_outer.size()) <
			make_pair(x.m_ready, x.m_outer.size());
	}
};

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
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
	int adjust_area(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<int, puz_area> m_id2area;
	map<Position, string> m_pos2nums;
	unsigned int m_distance = 0;
};

struct puz_state2 : Position
{
	puz_state2(const puz_state& s, const Position& p_start)
		: m_state(s), m_num(s.cells(p_start)) { make_move(p_start); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state& m_state;
	const int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
}

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c)
			m_pos2nums[{r, c}] = "123";

	adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_id2area){
		int id = kv.first;
		auto& area = kv.second;

		auto& outer = area.m_outer;
		outer.clear();
		for(auto& p : area.m_inner)
			for(auto& os : offset){
				auto p2 = p + os;
				char ch = cells(p2);
				if(ch == PUZ_SPACE)
					outer.insert(p2);
			}

		if(!init)
			switch(outer.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(*outer.begin(), area.m_cell_count) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
	cells(p) = n;

	vector<int> ids;
	for(auto& kv : m_id2area){
		int id = kv.first;
		auto& area = kv.second;
		if(area.m_cell_count == n && area.m_outer.count(p) == 1)
			ids.push_back(id);
	}

	int id = ids.front();
	auto& area = m_id2area.at(id);
	area.m_inner.insert(p);

	ids.erase(ids.begin());
	for(int id2 : ids){
		auto& area2 = m_id2area.at(id2);
		area.m_inner.insert(area2.m_inner.begin(), area2.m_inner.end());
		m_id2area.erase(id2);
	}

	int sz = area.m_inner.size();
	if(sz > area.m_cell_count)
		return false;

	++m_distance;
	return true;
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move2(p, n))
		return false;
	int m;
	while((m = adjust_area(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	//auto& kv = *boost::min_element(m_id2area, [](
	//	const pair<const int, puz_area>& kv1,
	//	const pair<const int, puz_area>& kv2){
	//	return kv1.second < kv2.second;
	//});

	//if(!kv.second.m_ready)
	//	for(auto& p : kv.second.m_outer){
	//		children.push_back(*this);
	//		if(!children.back().make_move(p, kv.second.m_cell_count))
	//			children.pop_back();
	//	}
	//else{
	//	auto it = boost::find(*this, PUZ_UNKNOWN);
	//	if(it == end()) return;
	//	int i = it - begin();
	//	Position p(i / sidelen(), i % sidelen());
	//	list<puz_state2> smoves;
	//	puz_move_generator<puz_state2>::gen_moves({*this, p}, smoves);
	//	int sz = min(3, static_cast<int>(smoves.size()));
	//	for(int n = 1; n <= sz; ++n){
	//		children.push_back(*this);
	//		if(!children.back().make_move_hidden(p, n))
	//			children.pop_back();
	//	}
	//}
}

ostream& puz_state::dump(ostream& out) const
{
	//for(int r = 0;; ++r){
	//	// draw horz-walls
	//	for(int c = 0; c < sidelen(); ++c)
	//		out << (m_horz_walls.at({r, c}) == PUZ_WALL_ON ? " -" : "  ");
	//	out << endl;
	//	if(r == sidelen()) break;
	//	for(int c = 0;; ++c){
	//		Position p(r, c);
	//		// draw vert-walls
	//		out << (m_vert_walls.at(p) == PUZ_WALL_ON ? '|' : ' ');
	//		if(c == sidelen()) break;
	//		out << cells(p);
	//	}
	//	out << endl;
	//}
	return out;
}

}}

void solve_puz_Tatamino2()
{
	using namespace puzzles::Tatamino2;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Tatamino.xml", "Puzzles\\Tatamino2.txt", solution_format::GOAL_STATE_ONLY);
}