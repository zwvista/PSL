#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 3/Branches

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

namespace puzzles{ namespace Branches{

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
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_WALL);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(ch);
			else{
				m_start.push_back(PUZ_NUMBER);
				int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
				m_pos2num[{r, c}] = n;
			}
		}
		m_start.push_back(PUZ_WALL);
	}
	m_start.append(string(m_sidelen, PUZ_WALL));
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, const vector<int>& perm);
	void make_move2(const Position& p, const vector<int>& perm);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	map<Position, vector<vector<int>>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2num)
		m_matches[kv.first];
	
	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		perms.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os)
				if(cells(p2) == PUZ_SPACE)
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
							perms.push_back({n0, n1, n2, n3});

		if(!init)
			switch(perms.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perms.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, const vector<int>& perm)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = perm[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			cells(p2) = str_branch[i + (j == n - 1 ? 4 : 0)];
			p2 += os;
		}
	}

	++m_distance;
	m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, const vector<int>& perm)
{
	m_distance = 0;
	make_move2(p, perm);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<vector<int>>>& kv1,
		const pair<const Position, vector<vector<int>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(auto& perm : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, perm))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			switch(char ch = cells({r, c})){
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

void solve_puz_Branches()
{
	using namespace puzzles::Branches;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Branches.xml", "Puzzles\\Branches.txt", solution_format::GOAL_STATE_ONLY);
}