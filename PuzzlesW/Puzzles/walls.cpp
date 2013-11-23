#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Walls

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

namespace puzzles{ namespace walls{

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
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 0; r < m_sidelen - 2; ++r){
		const string& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
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
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	void find_matches();
	bool make_move(const Position& p, const vector<int>& comb);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_pos2walls.size(); }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<vector<int>>> m_pos2walls;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start)
, m_game(&g)
{
	for(const auto& kv : g.m_pos2num)
		m_pos2walls[kv.first];
	
	find_matches();
}

void puz_state::find_matches()
{
	for(auto& kv : m_pos2walls){
		const auto& p = kv.first;
		auto& combs = kv.second;
		combs.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			bool is_horz = i % 2 == 1;
			const auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os){
				char ch = cell(p2);
				if(ch == PUZ_SPACE ||
					is_horz && ch == PUZ_HORZ ||
					!is_horz && ch == PUZ_VERT)
					nums.push_back(n++);
				else{
					nums.push_back(n);
					break;
				}
			}
		}

		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if(n0 + n1 + n2 + n3 == sum)
							combs.push_back({n0, n1, n2, n3});
	}
}

bool puz_state::make_move(const Position& p, const vector<int>& comb)
{
	for(int i = 0; i < 4; ++i){
		bool is_horz = i % 2 == 1;
		const auto& os = offset[i];
		int n = comb[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			cell(p2) = is_horz ? PUZ_HORZ : PUZ_VERT;
			p2 += os;
		}
		if(cell(p2) == PUZ_SPACE)
			cell(p2) = is_horz ? PUZ_VERT : PUZ_HORZ;
	}
	m_pos2walls.erase(p);
	find_matches();
	return boost::algorithm::none_of(m_pos2walls, [](const pair<const Position, vector<vector<int>>>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(m_pos2walls,
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

void solve_puz_walls()
{
	using namespace puzzles::walls;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\walls.xml", "Puzzles\\walls.txt", solution_format::GOAL_STATE_ONLY);
}