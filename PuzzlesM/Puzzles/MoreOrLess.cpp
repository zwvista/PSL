#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 7/More Or Less

	Summary
	Mr. Futoshiki, meet Mr. Sudoku

	Description
	1. More or Less can be seen as a mix between Sudoku and Futoshiki.
	2. Standard Sudoku rules apply, i.e. you need to fill the board with
	   numbers 1 to 9 in every row, column and area, without repetitions.
	3. The hints however, are given by the Greater Than or Less Than symbols
	   between tiles.

	Variation
	4. Further levels have irregular areas, instead of the classic 3*3 Sudoku.
	   All the other rules are the same.
*/

namespace puzzles{ namespace MoreOrLess{

#define PUZ_SPACE		' '
#define PUZ_ROW_LT		'<'
#define PUZ_ROW_GT		'>'
#define PUZ_ROW_LINE	'|'
#define PUZ_COL_LT		'^'
#define PUZ_COL_GT		'v'
#define PUZ_COL_LINE	'-'
	
const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const Position offset2[] = {
	{0, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, 0},		// w
};

struct puz_area_info
{
	vector<Position> m_range;
	vector<pair<int, int>> m_pairs;
	string m_hints;
	vector<string> m_perms;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, char> m_horz_walls, m_vert_walls;
	vector<puz_area_info> m_area_info;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

struct puz_state2 : Position
{
	puz_state2(const map<Position, char>& horz_walls, const map<Position, char>& vert_walls, const Position& p_start)
		: m_horz_walls(horz_walls), m_vert_walls(vert_walls) {
		make_move(p_start);
	}

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const map<Position, char> &m_horz_walls, &m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(int i = 0; i < 4; ++i){
		auto p = *this + offset[i];
		auto p_wall = *this + offset2[i];
		auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
		char ch = walls.at(p_wall);
		if(ch != PUZ_ROW_LINE && ch != PUZ_COL_LINE){
			children.push_back(*this);
			children.back().make_move(p);
		}
	}
}

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2)
, m_area_info(m_sidelen * 3)
{
	set<Position> rng;
	for(int r = 0;; ++r){
		// horz-walls
		auto& str_h = strs[r * 2];
		for(int c = 0; c < m_sidelen; ++c)
			m_horz_walls[{r, c}] = str_h[c * 2 + 1];
		if(r == m_sidelen) break;
		auto& str_v = strs[r * 2 + 1];
		for(int c = 0;; ++c){
			Position p(r, c);
			// vert-walls
			m_vert_walls[p] = str_v[c * 2];
			if(c == m_sidelen) break;
			rng.insert(p);
			m_area_info[p.first].m_range.push_back(p);
			m_area_info[m_sidelen + p.second].m_range.push_back(p);
		}
	}
	for(int n = 0; !rng.empty(); ++n){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
		auto& info = m_area_info[m_sidelen * 2 + n];
		for(auto& p : smoves){
			info.m_range.push_back(p);
			rng.erase(p);
		}
		boost::sort(info.m_range);
	}
	for(int i = 0; i < m_area_info.size(); ++i){
		auto& info = m_area_info[i];
		auto& rng = info.m_range;
		auto f = [&](const Position& p){
			return boost::range::find(rng, p) - rng.begin();
		};
		for(auto& p : rng){
			if(i < m_sidelen || i >= m_sidelen * 2){
				char ch = m_vert_walls.at(p);
				if(ch != PUZ_ROW_LINE){
					info.m_pairs.emplace_back(f(p + Position(0, -1)), f(p));
					info.m_hints.push_back(ch);
				}
			}
			if(i >= m_sidelen){
				char ch = m_horz_walls.at(p);
				if(ch != PUZ_COL_LINE){
					info.m_pairs.emplace_back(f(p + Position(-1, 0)), f(p));
					info.m_hints.push_back(ch == PUZ_COL_GT ? PUZ_ROW_GT : PUZ_ROW_LT);
				}
			}
		}
	}
	string perm = "123456789";
	do
		for(auto& info : m_area_info)
			if([&]{
				for(int i = 0; i < info.m_pairs.size(); ++i){
					auto& kv = info.m_pairs[i];
					if(info.m_hints[i] != (perm[kv.first] < perm[kv.second] ? PUZ_ROW_LT : PUZ_ROW_GT))
						return false;
				}
				return true;
			}())
				info.m_perms.push_back(perm);
	while(boost::next_permutation(perm));
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(int i, int j);
	void make_move2(int i, int j);
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
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int i = 0; i < g.m_area_info.size(); ++i){
		auto& info = g.m_area_info[i];
		auto& perm_ids = m_matches[i];
		perm_ids.resize(info.m_perms.size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& info = m_game->m_area_info[area_id];
		auto& perms = info.m_perms;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& p : info.m_range)
			chars.push_back(cells(p));

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(area_id, perm_ids.front()), 1;
		}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& info = m_game->m_area_info[i];
	auto& rng = info.m_range;
	auto& perm = info.m_perms[j];

	for(int k = 0; k < perm.size(); ++k)
		cells(rng[k]) = perm[k];

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<int, vector<int>>& kv1, 
		const pair<int, vector<int>>& kv2){
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
			out << cells({r, c}) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_MoreOrLess()
{
	using namespace puzzles::MoreOrLess;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\MoreOrLess.xml", "Puzzles\\MoreOrLess.txt", solution_format::GOAL_STATE_ONLY);
}