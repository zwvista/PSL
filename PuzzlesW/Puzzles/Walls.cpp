#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 5/Walls

	Summary
	Find the maze of Bricks

	Description
	1. In Walls you must fill the board with straight horizontal and
	   vertical lines (walls) that stem from each number.
	2. The number itself tells you the total length of Wall segments
	   connected to it.
	3. Wall pieces have two ways to be put, horizontally or vertically.
	4. Not every wall piece must be connected with a number, but the
	   board must be filled with wall pieces.
*/

namespace puzzles{ namespace Walls{

#define PUZ_SPACE			' '
#define PUZ_NUMBER			'N'
#define PUZ_HORZ			'-'
#define PUZ_VERT			'|'
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
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(ch);
			else{
				m_start.push_back(PUZ_NUMBER);
				m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
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
	void make_move3(const Position& p, const vector<int>& perm, int i, bool stopped);
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
	// key: the position of the number
	// value.elem: respective lengths of the wall segments that stem from
	//             the number in all the four directions
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
	bool matches_changed = false;
	set<Position> spaces;
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		auto perms_old = perms;
		perms.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			bool is_horz = i % 2 == 1;
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os){
				char ch = cells(p2);
				if(ch == PUZ_SPACE){
					// we can stop here
					nums.push_back(n++);
					spaces.insert(p2);
				}
				else if(is_horz && ch == PUZ_HORZ ||
					!is_horz && ch == PUZ_VERT)
					// we cannot stop here
					++n;
				else{
					// we have to stop here
					nums.push_back(n);
					break;
				}
			}
		}

		// Compute the total length of the wall segments connected to the number
		// Record the combination if the sum is equal to the given number
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
			default:
				matches_changed = matches_changed || perms != perms_old;
				break;
			}
	}
	// pruning
	// All the branches added up should cover all the remaining spaces
	if(boost::count(m_cells, PUZ_SPACE) != spaces.size())
		return 0;

	if(!matches_changed)
		return 2;

	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		for(int i = 0; i < 4; ++i){
			auto f = [=](const vector<int>& v1, const vector<int>& v2){
				return v1[i] < v2[i];
			};
			auto perm = *boost::min_element(perms, f);
			int n = boost::max_element(perms, f)->at(i);
			make_move3(p, perm, i, perm[i] == n);
		}
	}
	return 1;
}

void puz_state::make_move3(const Position& p, const vector<int>& perm, int i, bool stopped)
{
	bool is_horz = i % 2 == 1;
	auto& os = offset[i];
	int n = perm[i];
	auto p2 = p + os;
	for(int j = 0; j < n; ++j){
		cells(p2) = is_horz ? PUZ_HORZ : PUZ_VERT;
		p2 += os;
	}
	if(stopped && cells(p2) == PUZ_SPACE)
		// we choose to stop here, so it must be in other direction
		cells(p2) = is_horz ? PUZ_VERT : PUZ_HORZ;
}

void puz_state::make_move2(const Position& p, const vector<int>& perm)
{
	for(int i = 0; i < 4; ++i)
		make_move3(p, perm, i, true);

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
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_NUMBER)
				out << format("%2d") % m_game->m_pos2num.at(p);
			else
				out << ' ' << ch;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Walls()
{
	using namespace puzzles::Walls;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Walls.xml", "Puzzles\\Walls.txt", solution_format::GOAL_STATE_ONLY);
}