#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Box It Up

	Summary
	Numbered Areas Interval

	Description
	1. A simple puzzle where you have to divide the Board in Boxes (Rectangles).
	2. Each Box must contain one number and the number represents the area of
	   that Box.
*/

namespace puzzles{ namespace boxitup{

#define PUZ_SPACE		' '

// top-left and bottom-right
typedef pair<Position, Position> puz_box;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_start;
	map<Position, vector<puz_box>> m_combs;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			auto s = str.substr(c * 2, 2);
			int n = atoi(s.c_str());
			if(n != 0)
				m_start[Position(r, c)] = n;
		}
	}

	for(const auto& kv : m_start){
		auto& pn = kv.first;
		int box_area = kv.second;
		auto& boxes = m_combs[pn];

		for(int i = 1; i <= m_sidelen; ++i){
			int j = box_area / i;
			if(i * j != box_area || j > m_sidelen) continue;
			Position box_sz(i - 1, j - 1);
			auto p2 = pn - box_sz;
			for(int r = p2.first; r <= pn.first; ++r)
				for(int c = p2.second; c <= pn.second; ++c){
					Position tl(r, c), br = tl + box_sz;
					if(tl.first >= 0 && tl.second >= 0 &&
						br.first < m_sidelen && br.second < m_sidelen &&
						[&](){
							for(const auto& kv : m_start){
								auto& p = kv.first;
								if(p != pn &&
									p.first >= tl.first && p.second >= tl.second &&
									p.first <= br.first && p.second <= br.second)
									return false;
							}
							return true;
						}())
						boxes.emplace_back(tl, br);
				}
		}
	}
}

struct puz_state : map<Position, vector<int>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(int r, int c) const { return m_cells[r * sidelen() + c]; }
	char& cell(int r, int c) { return m_cells[r * sidelen() + c]; }
	bool make_move(const Position& p, int n);
	void make_move2(const Position& p, int n);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	unsigned int m_distance = 0;
	char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(sidelen() * sidelen(), PUZ_SPACE)
{
	for(auto& kv : g.m_start)
		(*this)[kv.first];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : *this){
		auto& boxes = m_game->m_combs.at(kv.first);

		kv.second.clear();
		for(int i = 0; i < boxes.size(); ++i){
			auto& box = boxes[i];
			if([&](){
				for(int r = box.first.first; r <= box.second.first; ++r)
					for(int c = box.first.second; c <= box.second.second; ++c)
						if(cell(r, c) != PUZ_SPACE)
							return false;
				return true;
			}())
				kv.second.push_back(i);
		}

		if(!init)
			switch(kv.second.size()){
			case 0:
				return 0;
			case 1:
				make_move2(kv.first, kv.second.front());
				return 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
	auto& box = m_game->m_combs.at(p)[n];

	for(int r = box.first.first; r <= box.second.first; ++r)
		for(int c = box.first.second; c <= box.second.second; ++c)
			cell(r, c) = m_ch;

	++m_distance, ++m_ch;
	erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	make_move2(p, n);
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
	return true;
}

void puz_state::gen_children(list<puz_state> &children) const
{
	auto& kv = *boost::min_element(*this, [](
		const pair<const Position, vector<int>>& kv1,
		const pair<const Position, vector<int>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(int n : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, n))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << cell(r, c);
		out << endl;
	}
	return out;
}

}}

void solve_puz_boxitup()
{
	using namespace puzzles::boxitup;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\boxitup.xml", "Puzzles\\boxitup.txt", solution_format::GOAL_STATE_ONLY);
}