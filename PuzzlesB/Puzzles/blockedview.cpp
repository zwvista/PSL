#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/Blocked View

	Summary
	This time it's one Garden and many Towers

	Description
	1. On the Board there are a few sentinels. These sentinels are marked with
	   a number.
	2. The number tells you how many tiles that Sentinel can control (see) from
	   there vertically and horizontally. This includes the tile where he is
	   located.
	3. You must put Towers on the Boards in accordance with these hints, keeping
	   in mind that a Tower blocks the Sentinel View.
	4. The restrictions are that there must be a single continuous Garden, and
	   two Towers can't touch horizontally or vertically.
	5. Towers can't go over numbered squares. But numbered squares don't block
	   Sentinel View.
*/

namespace puzzles{ namespace blockedview{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_SENTINEL	'S'
#define PUZ_TOWER		'T'

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
	void make_move2(const Position& p, const vector<int>& comb);
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
: map<Position, vector<vector<int>>>()
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
	for(int r = 0; r < sidelen(); ++r)
		cell(Position(r, 0)) = cell(Position(r, sidelen() - 1)) = PUZ_TOWER;
	for(int c = 0; c < sidelen(); ++c)
		cell(Position(0, c)) = cell(Position(sidelen() - 1, c)) = PUZ_TOWER;

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

		int sum = m_game->m_start.at(p) - 1;
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			const auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os)
				switch(cell(p2)){
				case PUZ_SPACE:
					nums.push_back(n++);
					break;
				case PUZ_EMPTY:
				case PUZ_SENTINEL:
					++n;
					break;
				case PUZ_TOWER:
					goto blocked;
				}
blocked:
			nums.push_back(n);
		}

		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if(n0 + n1 + n2 + n3 == sum)
							combs.push_back({n0, n1, n2, n3});

		if(!init)
			switch(combs.size()){
			case 0:
				return 0;
			case 1:
				make_move2(p, combs.front());
				return 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, const vector<int>& comb)
{
	for(int i = 0; i < 4; ++i){
		const auto& os = offset[i];
		int n = comb[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			char& ch = cell(p2);
			if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
			p2 += os;
		}
		cell(p2) = PUZ_TOWER;
	}

	++m_distance;
	erase(p);
}

bool puz_state::make_move(const Position& p, const vector<int>& comb)
{
	m_distance = 0;
	make_move2(p, comb);
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
	const auto& kv = *boost::min_element(*this,
		[](const pair<const Position, vector<vector<int>>>& kv1,
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

void solve_puz_blockedview()
{
	using namespace puzzles::blockedview;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\blockedview.xml", "Puzzles\\blockedview.txt", solution_format::GOAL_STATE_ONLY);
}