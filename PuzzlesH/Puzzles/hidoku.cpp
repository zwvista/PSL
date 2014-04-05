#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Hidoku

	Summary
	Jump from one neighboring tile to another and fill the board

	Description
	Starting at the tile number 1, reach the last tile by jumping from tile to tile.
	1. When jumping from a tile, you can land on any tile around it, horizontally, 
	   vertically or diagonally touching.
	2. The goal is to jump on every tile, only once and reach the last tile.
*/

namespace puzzles{ namespace Hidoku{

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	map<int, Position> m_num2pos;
	vector<int> m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = atoi(str.substr(c * 3, 3).c_str());
			m_start.push_back(n);
			if(n != 0)
				m_num2pos[n] = p;
		}
	}
}

struct puz_segment
{
	pair<Position, int> m_cur, m_dest;
	vector<Position> m_next;
};

struct puz_state : vector<int>
{
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	void segment_next(puz_segment& o) const;
	bool make_move(int i, int j);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_segments, 0, [&](int acc, const puz_segment& o){
			return acc + o.m_dest.second - o.m_cur.second - 1;
		});
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	vector<puz_segment> m_segments;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_start), m_game(&g)
{
	for(auto prev = m_game->m_num2pos.begin(), first = std::next(prev),
		last = m_game->m_num2pos.end(); first != last; ++prev, ++first)
		if(first->first - prev->first > 1){
			m_segments.emplace_back();
			auto& o = m_segments.back();
			o.m_cur = {prev->second, prev->first};
			o.m_dest = {first->second, first->first};
			segment_next(o);
		}
}

void puz_state::segment_next(puz_segment& o) const
{
	int n = o.m_cur.second + 1, n2 = o.m_dest.second;
	auto& p2 = o.m_dest.first;
	o.m_next.clear();
	for(auto& os : offset){
		auto p = o.m_cur.first + os;
		if(is_valid(p) && cells(p) == 0 &&
			// pruning : should not be too far to jump to
			max(myabs(p.first - p2.first), myabs(p.second - p2.second)) <= n2 - n)
			o.m_next.push_back(p);
	}
}

bool puz_state::make_move(int i, int j)
{
	auto& o = m_segments[i];
	auto p = o.m_next[j];
	cells(o.m_cur.first = p) = ++o.m_cur.second;
	if(o.m_dest.second - o.m_cur.second == 1)
		m_segments.erase(m_segments.begin() + i);
	else
		segment_next(o);
	for(auto& o2 : m_segments)
		boost::remove_erase(o2.m_next, p);

	return boost::algorithm::none_of(m_segments, [](const puz_segment& o2){
		return o2.m_next.empty();
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto it = boost::min_element(m_segments, [](
		const puz_segment& o1, const puz_segment& o2){
		return o1.m_next.size() < o2.m_next.size();
	});
	int i = it - m_segments.begin();

	for(int j = 0; j < it->m_next.size(); ++j){
		children.push_back(*this);
		if(!children.back().make_move(i, j))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << format("%3d") % cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_Hidoku()
{
	using namespace puzzles::Hidoku;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Hidoku.xml", "Puzzles\\Hidoku.txt", solution_format::GOAL_STATE_ONLY);
}
