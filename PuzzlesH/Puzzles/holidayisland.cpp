#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 11/Holiday Island

	Summary
	This time the campers won't have their way!

	Description
	1. This time the resort is an island, the place is packed and the campers
	   (Tents) must compromise!
	2. The board represents an Island, where there are a few Tents, identified
	   by the numbers.
	3. Your job is to find the water surrounding the island, with these rules:
	4. There is only one, continuous island.
	5. The numbers tell you how many tiles that camper can walk from his Tent,
	   by moving horizontally or vertically. A camper can't cross water or 
	   other Tents.
*/

namespace puzzles{ namespace holidayisland{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_TENT		'T'
#define PUZ_WATER		'W'
#define PUZ_BOUNDARY	'B'

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
	map<Position, int> m_pos2num;
	string m_start;
	int m_reachable = 0;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch != PUZ_SPACE){
				int n = ch - '0';
				m_reachable += n;
				m_pos2num[{r, c}] = n;
			}
			m_start.push_back(ch == PUZ_SPACE ? PUZ_WATER : PUZ_TENT);
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_area
{
	set<Position> m_inner, m_outer;
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
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);
	bool make_move2(const Position& p);
	int adjust_area(bool init);

	//solve_puzzle interface
	bool is_goal_state() const { return get_heuristic() == 0; }
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_game->m_reachable - boost::accumulate(m_pos2area, 0, [this](
			int acc, const pair<const Position, puz_area>& kv){
			return acc + kv.second.m_inner.size() - 1;
		});
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	unsigned int m_distance = 0;
	map<Position, puz_area> m_pos2area;
	Position m_starting;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2num){
		auto& pnum = kv.first;
		m_pos2area[pnum].m_inner.insert(pnum);
	}
	adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_pos2area){
		const auto& pnum = kv.first;
		auto& area = kv.second;
		auto& outer = area.m_outer;

		if(area.m_ready) continue;

		outer.clear();
		for(auto& p : area.m_inner)
			for(auto& os : offset){
				auto p2 = p + os;
				char ch = cells(p2);
				if(ch == PUZ_WATER)
					outer.insert(p2);
			}

		if(!init)
			switch(outer.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(*outer.begin()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
	make_move(s.m_starting);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	if(m_state->cells(*this) == PUZ_TENT) return;

	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch != PUZ_BOUNDARY && ch != PUZ_WATER){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

struct puz_state3 : Position
{
	puz_state3(const puz_state& s);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state3>& children) const;

	const puz_state* m_state;
};

puz_state3::puz_state3(const puz_state& s)
: m_state(&s)
{
	make_move(s.m_starting);
}

void puz_state3::gen_children(list<puz_state3>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch != PUZ_BOUNDARY && ch != PUZ_WATER){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move2(const Position& p)
{
	cells(m_starting = p) = PUZ_EMPTY;
	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	vector<Position> ps_tent, ps_empty;
	for(const auto& p2 : smoves)
		(cells(p2) == PUZ_TENT ? ps_tent : ps_empty).push_back(p2);

	for(const auto& p2 : ps_tent){
		auto& area = m_pos2area.at(p2);
		if(area.m_ready)
			return false;

		auto& inner = area.m_inner;
		int sz = inner.size();
		int num = m_game->m_pos2num.at(p2) + 1;
		inner.insert(ps_empty.begin(), ps_empty.end());
		if(inner.size() > num)
			return false;
		area.m_ready = inner.size() == num;
		m_distance += inner.size() - sz;
	}

	if(boost::algorithm::any_of(m_pos2area, [](const pair<const Position, puz_area>& kv){
		return !kv.second.m_ready;
	}))
		return true;

	list<puz_state3> smoves2;
	puz_move_generator<puz_state3>::gen_moves(*this, smoves2);
	return smoves2.size() == boost::count_if(*this, [](char ch){
		return ch != PUZ_BOUNDARY && ch != PUZ_WATER;
	});
}

bool puz_state::make_move(const Position& p)
{
	m_distance = 0;
	if(!make_move2(p))
		return false;
	int m;
	while((m = adjust_area(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_pos2area, [](
		const pair<const Position, puz_area>& kv1,
		const pair<const Position, puz_area>& kv2){
		return kv1.second < kv2.second;
	});
	for(const auto& p : kv.second.m_outer){
		children.push_back(*this);
		if(!children.back().make_move(p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_TENT)
				out << format("%-2d") % m_game->m_pos2num.at(p);
			else
				out << ch << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_holidayisland()
{
	using namespace puzzles::holidayisland;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\holidayisland.xml", "Puzzles\\holidayisland.txt", solution_format::GOAL_STATE_ONLY);
}