#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/Branches

	Summary
	Fill the board with Branches departing from the numbers

	Description
	1. In Branches you must fill the board with straight horizontal and
	   vertical lines(Branches) that stem from each number.
	2. The number itself tells you how many tiles its Branches fill up.
	   The tile with the number doesn't count.
	3. There can't be blank tiles and Branches can't overlap, nor run over
	   the numbers. Moreover Branches must be in a single straight line
	   and can't make corners.
*/

namespace puzzles{ namespace branches{

#define PUZ_SPACE		' '
#define PUZ_NUMBER		'N'
#define PUZ_BRANCH		'B'
#define PUZ_WALL		'W'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const string str_branch = "|-|-^>v<";

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_WALL));
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_WALL);
		for(int c = 0; c < m_sidelen - 2; ++c)
			switch(char ch = str[c]){
			case PUZ_SPACE:
				m_start.push_back(ch);
				break;
			default:
				m_start.push_back(PUZ_NUMBER);
				m_pos2num[Position(r + 1, c + 1)] = ch - '0';
				break;
			}
		m_start.push_back(PUZ_WALL);
	}
	m_start.append(string(m_sidelen, PUZ_WALL));
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
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(const auto& kv : g.m_pos2num)
		(*this)[kv.first];
	
	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : *this){
		const auto& p = kv.first;
		auto& combs = kv.second;
		combs.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os)
				if(cell(p2) == PUZ_SPACE)
					nums.push_back(n++);
				else{
					nums.push_back(n);
					break;
				}
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
				return make_move2(p, combs.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, const vector<int>& comb)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = comb[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			cell(p2) = str_branch[i + (j == n - 1 ? 4 : 0)];
			p2 += os;
		}
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
			case PUZ_NUMBER:
				out << format("%2d") % m_game->m_pos2num.at(p);
				break;
			default:
				out << ' ' << ch;
				break;
			};
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_branches()
{
	using namespace puzzles::branches;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\branches.xml", "Puzzles\\branches.txt", solution_format::GOAL_STATE_ONLY);
}