#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/Fillomino

	Summary
	Detect areas marked by their extension

	Description
	1. The goal is to detect areas marked with the tile count of the area
	   itself.
	2. So for example, areas marked '1', will always consist of one single
	   tile. Areas marked with '2' will consist of two (horizontally or
	   vertically) adjacent tiles. Tiles numbered '3' will appear in a group
	   of three and so on.
	3. Two areas with the same areas can also be totally hidden at the start.
	
	Variation
	4. Fillomino has several variants.
	5. No Rectangles: Areas can't form Rectangles.
	6. Only Rectangles: Areas can ONLY form Rectangles.
	7. Non Consecutive: Areas can't touch another area which has +1 or -1
	   as number (orthogonally).
	8. Consecutive: Areas MUST touch another area which has +1 or -1
	   as number (orthogonally).
	9. All Odds: There are only odd numbers on the board.
	10.All Evens: There are only even numbers on the board.
*/

namespace puzzles{ namespace Fillomino{

#define PUZ_UNKNOWN			-1
#define PUZ_WALL_UNKNOWN	'0'
#define PUZ_WALL_ON			'1'
#define PUZ_WALL_OFF		'2'

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

enum puz_game_type
{
	NORMAL_FILLOMINO,
	NO_RECTANGLES_FILLOMINO,
	ONLY_RECTANGLES_FILLOMINO,
	NON_CONSECUTIVE_FILLOMINO,
	CONSECUTIVE_FILLOMINO,
	ALL_ODDS_FILLOMINO,
	ALL_EVENS_FILLOMINO,
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	puz_game_type m_game_type;
	map<Position, int> m_pos2num;
	vector<int> m_start;
	map<Position, char> m_horz_walls, m_vert_walls;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2)
{
	auto game_type = attrs.get<string>("GameType", "Fillomino");
	m_game_type =
		game_type == "No Rectangles" ? NO_RECTANGLES_FILLOMINO :
		game_type == "Only Rectangles" ? ONLY_RECTANGLES_FILLOMINO :
		game_type == "Non Consecutive" ? NON_CONSECUTIVE_FILLOMINO :
		game_type == "Consecutive" ? CONSECUTIVE_FILLOMINO :
		game_type == "All Odds" ? ALL_ODDS_FILLOMINO :
		game_type == "All Evens" ? ALL_EVENS_FILLOMINO :
		NORMAL_FILLOMINO;

	for(int r = 0;; ++r){
		// horz-walls
		auto& str_h = strs[r * 2];
		for(int c = 0; c < m_sidelen; ++c)
			m_horz_walls[{r, c}] = str_h[c * 2 + 1] == '-' ? PUZ_WALL_ON : PUZ_WALL_UNKNOWN;
		if(r == m_sidelen) break;
		auto& str_v = strs[r * 2 + 1];
		for(int c = 0;; ++c){
			Position p(r, c);
			// vert-walls
			m_vert_walls[p] = str_v[c * 2] == '|' ? PUZ_WALL_ON : PUZ_WALL_UNKNOWN;
			if(c == m_sidelen) break;
			char ch = str_v[c * 2 + 1];
			int n = ch == ' ' ? PUZ_UNKNOWN : ch - '0';
			m_start.push_back(n);
			if(n != PUZ_UNKNOWN)
				m_pos2num[p] = n;
		}
	}
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

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
	bool make_move_hidden(const Position& p, int n);
	int adjust_area(bool init);
	bool check_cell_count(const Position& p, function<bool(int)> f);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_UNKNOWN); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<int, puz_area> m_id2area;
	map<Position, char> m_horz_walls, m_vert_walls;
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
	for(int i = 0; i < 4; ++i){
		auto p = *this + offset[i];
		auto p_wall = *this + offset2[i];
		auto& walls = i % 2 == 0 ? m_state.m_horz_walls : m_state.m_vert_walls;
		if(walls.at(p_wall) != PUZ_WALL_ON && m_state.cells(p) == m_num){
			children.push_back(*this);
			children.back().make_move(p);
		}
	}
}

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_start), m_game(&g)
, m_horz_walls(g.m_horz_walls)
, m_vert_walls(g.m_vert_walls)
{
	set<Position> rng;
	for(auto& kv : g.m_pos2num)
		rng.insert(kv.first);

	for(int i = 0; !rng.empty(); ++i){
		auto& p_start = *rng.begin();
		auto& area = m_id2area[i];
		area.m_cell_count = cells(p_start);

		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({*this, p_start}, smoves);
		for(auto& p : smoves){
			area.m_inner.insert(p);
			rng.erase(p);
		}
		area.m_ready = area.m_inner.size() == area.m_cell_count;
	}

	adjust_area(true);
}

bool puz_state::check_cell_count(const Position& p, function<bool(int)> f)
{
	for(int i = 0; i < 4; ++i){
		auto p2 = p + offset[i];
		auto p_wall = p + offset2[i];
		auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
		if(walls.at(p_wall) == PUZ_WALL_ON && is_valid(p2) && f(cells(p2)))
			return true;
	}
	return false;
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_id2area){
		int id = kv.first;
		auto& area = kv.second;

		auto& outer = area.m_outer;
		outer.clear();
		if(area.m_ready) continue;

		for(auto& p : area.m_inner)
			for(int i = 0; i < 4; ++i){
				auto p2 = p + offset[i];
				auto p_wall = p + offset2[i];
				auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
				if(walls.at(p_wall) != PUZ_WALL_ON && cells(p2) == PUZ_UNKNOWN &&
					!check_cell_count(p2, [&](int cnt){
					return cnt == area.m_cell_count;
				}))
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

	area.m_ready = sz == area.m_cell_count;
	for(auto& p2 : area.m_inner)
		for(int i = 0; i < 4; ++i){
			auto p3 = p2 + offset[i];
			auto p_wall2 = p2 + offset2[i];
			auto& walls2 = i % 2 == 0 ? m_horz_walls : m_vert_walls;
			if(area.m_inner.count(p3) == 1)
				walls2.at(p_wall2) = PUZ_WALL_OFF;
			else if(area.m_ready)
				walls2.at(p_wall2) = PUZ_WALL_ON;
		}

	if(area.m_ready)
		if(m_game->m_game_type == ALL_ODDS_FILLOMINO ||
			m_game->m_game_type == ALL_EVENS_FILLOMINO){
			if((m_game->m_game_type == ALL_EVENS_FILLOMINO) != (sz % 2 == 0))
				return false;
		}
		else if(m_game->m_game_type == NO_RECTANGLES_FILLOMINO ||
			m_game->m_game_type == ONLY_RECTANGLES_FILLOMINO){
			auto mm1 = std::minmax_element(area.m_inner.begin(), area.m_inner.end(),
				[](const Position& p1, const Position& p2){
				return p1.first < p2.first;
			});
			auto mm2 = std::minmax_element(area.m_inner.begin(), area.m_inner.end(),
				[](const Position& p1, const Position& p2){
				return p1.second < p2.second;
			});
			int sz2 = (mm1.second->first - mm1.first->first + 1) *
				(mm2.second->second - mm2.first->second + 1);
			if((m_game->m_game_type == ONLY_RECTANGLES_FILLOMINO) != (sz == sz2))
				return false;
		}

	++m_distance;

	auto f = [&](const Position& p2, int cnt){
		return check_cell_count(p2, [&](int cnt2){
			return myabs(cnt2 - cnt) == 1;
		});
	};
	return m_game->m_game_type != NON_CONSECUTIVE_FILLOMINO &&
		m_game->m_game_type != CONSECUTIVE_FILLOMINO ||
		!is_goal_state() ||
		m_game->m_game_type == NON_CONSECUTIVE_FILLOMINO &&
		boost::algorithm::all_of(m_id2area, [&](const pair<int, puz_area>& kv){
			return boost::algorithm::none_of(kv.second.m_inner, [&](const Position& p2){
				return f(p2, kv.second.m_cell_count);
			});
		}) || m_game->m_game_type == CONSECUTIVE_FILLOMINO &&
		boost::algorithm::all_of(m_id2area, [&](const pair<int, puz_area>& kv){
			return boost::algorithm::any_of(kv.second.m_inner, [&](const Position& p2){
				return f(p2, kv.second.m_cell_count);
			});
		});
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

bool puz_state::make_move_hidden(const Position& p, int n)
{
	int id = boost::max_element(m_id2area, [](
		const pair<int, puz_area>& kv1,
		const pair<int, puz_area>& kv2){
		return kv1.first < kv2.first;
	})->first + 1;
	auto& area = m_id2area[id];
	if(check_cell_count(p, [n](int cnt){ return cnt == n; }))
		return false;
	area.m_cell_count = n;
	area.m_outer.insert(p);
	return make_move(p, n);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_id2area, [](
		const pair<int, puz_area>& kv1,
		const pair<int, puz_area>& kv2){
		return kv1.second < kv2.second;
	});

	if(!kv.second.m_ready)
		for(auto& p : kv.second.m_outer){
			children.push_back(*this);
			if(!children.back().make_move(p, kv.second.m_cell_count))
				children.pop_back();
		}
	else{
		auto it = boost::find(*this, PUZ_UNKNOWN);
		if(it == end()) return;
		int i = it - begin();
		Position p(i / sidelen(), i % sidelen());
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({*this, p}, smoves);
		for(int n = 1; n <= smoves.size(); ++n){
			children.push_back(*this);
			if(!children.back().make_move_hidden(p, n))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-walls
		for(int c = 0; c < sidelen(); ++c)
			out << (m_horz_walls.at({r, c}) == PUZ_WALL_ON ? " -" : "  ");
		out << endl;
		if(r == sidelen()) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-walls
			out << (m_vert_walls.at(p) == PUZ_WALL_ON ? '|' : ' ');
			if(c == sidelen()) break;
			out << cells(p);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Fillomino()
{
	using namespace puzzles::Fillomino;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Fillomino.xml", "Puzzles\\Fillomino.txt", solution_format::GOAL_STATE_ONLY);
}