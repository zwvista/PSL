#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 2/Lighten Up

	Summary
	Place lightbulbs to light up all the room squares

	Description
	1. What you see from above is a room and the marked squares are walls.
	2. The goal is to put lightbulbs in the room so that all the blank(non-wall)
	   squares are lit, following these rules.
	3. Lightbulbs light all free, unblocked squares horizontally and vertically.
	4. A lightbulb can't light another lightbulb.
	5. Walls block light. Also walls with a number tell you how many lightbulbs
	   are adjacent to it, horizontally and vertically.
	6. Walls without a number can have any number of lightbulbs. However,
	   lightbulbs don't need to be adjacent to a wall.
*/

namespace puzzles{ namespace lightenup{

#define PUZ_WALL		'W'
#define PUZ_BULB		'B'
#define PUZ_SPACE		' '
#define PUZ_UNLIT		'.'
#define PUZ_LIT			'+'

Position offset[] = {
	{-1, 0},	// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},	// w
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_walls;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			switch(char ch = str[c]){
			case PUZ_SPACE:
			case PUZ_WALL:
				m_start.push_back(ch);
				break;
			default:
				m_start.push_back(PUZ_WALL);
				m_walls[p] = ch - '0';
				break;
			}
		}
	}
}

using puz_area = pair<vector<Position>, vector<string>>;

struct puz_state : pair<string, vector<puz_area>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cell(const Position& p) const { return first.at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return first[p.first * sidelen() + p.second]; }
	void check_areas();
	bool make_move_space(const Position& p);
	bool make_move_area(int i, const string& comb);
	bool make_move(const Position& p, char ch_p);

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::count_if(first, [](char ch){
			return ch == PUZ_SPACE || ch == PUZ_UNLIT;
		});
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: pair<string, vector<puz_area>>(g.m_start, {})
, m_game(&g)
{
	for(const auto& kv : g.m_walls){
		const auto& p = kv.first;
		int n = kv.second;
		vector<Position> ps;
		for(const auto& os : offset){
			auto p2 = p + os;
			if(is_valid(p2) && cell(p2) != PUZ_WALL)
				ps.push_back(p2);
		}

		vector<string> combs;
		string comb;
		for(int i = 0; i < ps.size() - n; ++i)
			comb.push_back(PUZ_UNLIT);
		for(int i = 0; i < n; ++i)
			comb.push_back(PUZ_BULB);
		do
			combs.push_back(comb);
		while(boost::next_permutation(comb));
		second.emplace_back(ps, combs);
	}
}

bool puz_state::make_move_space(const Position& p)
{
	m_distance = 0;
	return make_move(p, PUZ_BULB);
}

bool puz_state::make_move_area(int i, const string& comb)
{
	m_distance = 0;

	const auto& ps = second[i].first;
	for(int j = 0; j < ps.size(); ++j)
		if(!make_move(ps[j], comb[j]))
			return false;

	second.erase(second.begin() + i);
	return true;
}

bool puz_state::make_move(const Position& p, char ch_p)
{
	char& ch = cell(p);
	if(ch_p == PUZ_UNLIT){
		if(ch == PUZ_BULB)
			return false;
		if(ch == PUZ_SPACE)
			ch = ch_p;
	}
	else{
		//PUZ_BULB
		if(ch != PUZ_SPACE)
			return ch == ch_p;
		ch = ch_p;
		++m_distance;
		for(const auto& os : offset){
			for(auto p2 = p + os; is_valid(p2); p2 += os)
				switch(char& ch2 = cell(p2))
			{
				case PUZ_BULB:
					return false;
				case PUZ_WALL:
					goto blocked;
				case PUZ_UNLIT:
				case PUZ_SPACE:
					ch2 = PUZ_LIT;
					++m_distance;
					break;
			};
		blocked:
			;
		}
	}
	return true;
}

void puz_state::gen_children(list<puz_state> &children) const
{
	if(second.empty()){
		vector<Position> ps;
		for(int r = 0; r < sidelen(); ++r)
			for(int c = 0; c < sidelen(); ++c){
				Position p(r, c);
				if(cell(p) == PUZ_SPACE)
					ps.push_back(p);
			}
		for(const Position& p : ps){
			children.push_back(*this);
			if(!children.back().make_move_space(p))
				children.pop_back();
		}
	}
	else{
		int i = boost::min_element(second, [](const puz_area& a1, const puz_area& a2){
			return a1.second.size() < a2.second.size();
		}) - second.begin();
		for(const auto& comb : second[i].second){
			children.push_back(*this);
			if(!children.back().make_move_area(i, comb))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto i = m_game->m_walls.find(p);
			out << (i == m_game->m_walls.end() ? cell(p) : char(i->second + '0')) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_lightenup()
{
	using namespace puzzles::lightenup;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\lightenup.xml", "Puzzles\\lightenup.txt", solution_format::GOAL_STATE_ONLY);
}
