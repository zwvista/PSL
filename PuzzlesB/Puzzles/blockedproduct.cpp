#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 11/Blocked Product

	Summary
	Multiplicative Towers

	Description
	1. On the Board there are a few sentinels. These sentinels are marked with
	   a number.
	2. The number tells you the product of the tiles that Sentinel can control
	   (see) from there vertically and horizontally. This includes the tile 
	   where he is located.
	3. You must put Towers on the Boards in accordance with these hints, keeping
	   in mind that a Tower blocks the Sentinel View.
	4. The restrictions are that there must be a single continuous Garden, and
	   two Towers can't touch horizontally or vertically.
	5. Towers can't go over numbered squares. But numbered squares don't block
	   Sentinel View.
*/

namespace puzzles{ namespace blockedproduct{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_SENTINEL	'S'
#define PUZ_TOWER		'T'
#define PUZ_CONNECTED	'C'
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
	map<Position, int> m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 2; ++c){
			auto s = str.substr(c * 2, 2);
			if(s != "  ")
				m_start[Position(r + 1, c + 1)] = atoi(s.c_str());
		}
	}
}

struct puz_state : map<Position, vector<vector<int>>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, const vector<int>& comb);
	bool make_move2(const Position& p, const vector<int>& comb);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return size();}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
	for(int r = 0; r < sidelen(); ++r)
		cell(Position(r, 0)) = cell(Position(r, sidelen() - 1)) = PUZ_BOUNDARY;
	for(int c = 0; c < sidelen(); ++c)
		cell(Position(0, c)) = cell(Position(sidelen() - 1, c)) = PUZ_BOUNDARY;

	for(const auto& kv : g.m_start)
		cell(kv.first) = PUZ_SENTINEL, (*this)[kv.first];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : *this){
		const auto& p = kv.first;
		auto& combs = kv.second;
		combs.clear();

		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			[&](){
				for(auto p2 = p + os; ; p2 += os)
					switch(cell(p2)){
					case PUZ_SPACE:
						nums.push_back(n++);
						break;
					case PUZ_EMPTY:
					case PUZ_SENTINEL:
						++n;
						break;
					case PUZ_TOWER:
					case PUZ_BOUNDARY:
						nums.push_back(n);
						return;
					}
			}();
		}

		int product = m_game->m_start.at(p);
		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if((n0 + n2 + 1) * (n1 + n3 + 1) == product)
							combs.push_back({n0, n1, n2, n3});

		if(!init)
			switch(combs.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, combs.front()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : string
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_game->m_sidelen; }
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	void make_move(int i){ (*this)[i] = PUZ_CONNECTED; }
	void gen_children(list<puz_state2>& children) const;

	const puz_game* m_game;
};

puz_state2::puz_state2(const puz_state& s)
: string(s.m_cells), m_game(s.m_game)
{
	int i = boost::find_if(*this, [](int n){
		return n != PUZ_BOUNDARY && n != PUZ_TOWER;
	}) - begin();
	make_move(i);
}

void puz_state2::gen_children(list<puz_state2> &children) const
{
	for(int i = 0; i < length(); ++i){
		Position p(i / sidelen(), i % sidelen());
		switch(at(i)){
		case PUZ_BOUNDARY:
		case PUZ_CONNECTED:
		case PUZ_TOWER:
			break;
		default:
			for(auto& os : offset){
				auto p2 = p + os;
				if(cell(p2) == PUZ_CONNECTED){
					children.push_back(*this);
					children.back().make_move(i);
					return;
				}
			}
		}
	}
}

bool puz_state::make_move2(const Position& p, const vector<int>& comb)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = comb[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			char& ch = cell(p2);
			if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
			p2 += os;
		}
		char& ch = cell(p2);
		if(ch == PUZ_SPACE){
			for(auto& os2 : offset){
				auto p3 = p2 + os2;
				if(cell(p3) == PUZ_TOWER)
					return false;
			}
			ch = PUZ_TOWER;
		}
	}

	++m_distance;
	erase(p);

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	const auto& state = smoves.back();
	return boost::algorithm::all_of(state, [](int n){
		return n == PUZ_BOUNDARY || n == PUZ_CONNECTED || n == PUZ_TOWER;
	});
}

bool puz_state::make_move(const Position& p, const vector<int>& comb)
{
	m_distance = 0;
	if(!make_move2(p, comb))
		return false;
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(*this, [](
		const pair<const Position, vector<vector<int>>>& kv1,
		const pair<const Position, vector<vector<int>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(const auto& comb : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, comb))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			switch(char ch = cell(Position(r, c))){
			case PUZ_SENTINEL:
				out << format("%2d") % m_game->m_start.at(p);
				break;
			default:
				out << ' ' << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
				break;
			};
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_blockedproduct()
{
	using namespace puzzles::blockedproduct;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\blockedproduct.xml", "Puzzles\\blockedproduct.txt", solution_format::GOAL_STATE_ONLY);
}