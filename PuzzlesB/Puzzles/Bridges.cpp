#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 7/Bridges

	Summary
	Enough Sudoku, let's build!

	Description
	1. The board represents a Sea with some islands on it.
	2. You must connect all the islands with Bridges, making sure every
	   island is connected to each other with a Bridges path.
	3. The number on each island tells you how many Bridges are touching
	   that island.
	4. Bridges can only run horizontally or vertically and can't cross
	   each other.
	5. Lastly, you can connect two islands with either one or two Bridges
	   (or none, of course)
*/

namespace puzzles{ namespace Bridges{

#define PUZ_SPACE			' '
#define PUZ_NUMBER			'N'
#define PUZ_HORZ_1			'-'
#define PUZ_VERT_1			'|'
#define PUZ_HORZ_2			'='
#define PUZ_VERT_2			'H'
#define PUZ_BOUNDARY		'B'

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

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen - 2; ++c){
			char ch = str[c];
			if(ch == PUZ_SPACE)
				m_start.push_back(ch);
			else{
				m_start.push_back(PUZ_NUMBER);
				m_pos2num[{r + 1, c + 1}] = ch - '0';
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
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
	for(const auto& kv : g.m_pos2num)
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
			bool is_horz = i % 2 == 1;
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			nums = {0};
			for(auto p2 = p + os; ; p2 += os){
				char ch = cells(p2);
				if(ch == PUZ_NUMBER){
					if(m_matches.count(p2) != 0)
						nums = {0, 1, 2};
				}
				else if(ch == PUZ_HORZ_1 || ch == PUZ_HORZ_2){
					if(is_horz)
						nums = {ch == PUZ_HORZ_1 ? 1 : 2};
				}
				else if(ch == PUZ_VERT_1 || ch == PUZ_VERT_2){
					if(!is_horz)
						nums = {ch == PUZ_VERT_1 ? 1 : 2};
				}
				if(ch != PUZ_SPACE)
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
		bool is_horz = i % 2 == 1;
		auto& os = offset[i];
		int n = perm[i];
		if(n == 0)
			continue;
		for(auto p2 = p + os; ; p2 += os){
			char& ch = cells(p2);
			if(ch != PUZ_SPACE)
				break;
			ch = is_horz ? n == 1 ? PUZ_HORZ_1 : PUZ_HORZ_2
				: n == 1 ? PUZ_VERT_1 : PUZ_VERT_2;
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

	for(const auto& perm : kv.second){
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
			switch(char ch = cells(p)){
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

void solve_puz_Bridges()
{
	using namespace puzzles::Bridges;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Bridges.xml", "Puzzles\\Bridges.txt", solution_format::GOAL_STATE_ONLY);
}
