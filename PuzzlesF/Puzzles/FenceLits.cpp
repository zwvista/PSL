#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 12/FenceLits

	Summary
	Fencing Tetris

	Description
	1. The goal is to divide the board into Tetris pieces, including the
	   square one (differently from LITS).
	2. The number in a cell tells you how many of the sides are marked
	   (like slitherlink).
	3. Please consider that the outside border of the board as marked.
*/

namespace puzzles{ namespace FenceLits{

#define PUZ_SPACE		' '	
#define PUZ_UNKNOWN		-1

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

const vector<vector<Position>> tetrominoes = {
	// L
	{{0, 0}, {1, 0}, {2, 0}, {2, 1}},
	{{0, 1}, {1, 1}, {2, 0}, {2, 1}},
	{{0, 0}, {0, 1}, {0, 2}, {1, 0}},
	{{0, 0}, {0, 1}, {0, 2}, {1, 2}},
	{{0, 0}, {0, 1}, {1, 0}, {2, 0}},
	{{0, 0}, {0, 1}, {1, 1}, {2, 1}},
	{{0, 0}, {1, 0}, {1, 1}, {1, 2}},
	{{0, 2}, {1, 0}, {1, 1}, {1, 2}},
	// I
	{{0, 0}, {1, 0}, {2, 0}, {3, 0}},
	{{0, 0}, {0, 1}, {0, 2}, {0, 3}},
	// T
	{{0, 0}, {0, 1}, {0, 2}, {1, 1}},
	{{0, 1}, {1, 0}, {1, 1}, {2, 1}},
	{{0, 1}, {1, 0}, {1, 1}, {1, 2}},
	{{0, 0}, {1, 0}, {1, 1}, {2, 0}},
	// S
	{{0, 0}, {0, 1}, {1, 1}, {1, 2}},
	{{0, 1}, {0, 2}, {1, 0}, {1, 1}},
	{{0, 0}, {1, 0}, {1, 1}, {2, 1}},
	{{0, 1}, {1, 0}, {1, 1}, {2, 0}},
	// O
	{{0, 0}, {0, 1}, {1, 0}, {1, 1}},
};

struct tetromino
{
	vector<Position> m_offset;
	vector<int> m_nums;
	vector<Position> m_horz_walls, m_vert_walls;
};

typedef pair<Position, int> puz_lit;

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<tetromino> m_tetros;
	map<Position, int> m_pos2num;
	vector<puz_lit> m_lits;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_tetros(tetrominoes.size())
{
	for(int i = 0; i < m_tetros.size(); ++i){
		auto& t = m_tetros[i];
		t.m_offset = tetrominoes[i];
		t.m_nums.resize(4);
		for(int j = 0; j < 4; ++j){
			int& n = t.m_nums[j];
			auto& p = t.m_offset[j];
			for(int k = 0; k < 4; ++k)
				if(boost::algorithm::none_of_equal(t.m_offset, p + offset[k])){
					++n;
					(k % 2 == 0 ? t.m_horz_walls : t.m_vert_walls).push_back(p + offset2[k]);
				}
		}
	}

	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			m_pos2num[{r, c}] = ch == PUZ_SPACE ? PUZ_UNKNOWN : ch - '0';
		}
	}

	for(int r = 0; r < m_sidelen; ++r)
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			for(int i = 0; i < m_tetros.size(); ++i){
				auto& t = m_tetros[i];
				if([&]{
					for(int j = 0; j < 4; ++j){
						auto p2 = p + t.m_offset[j];
						auto it = m_pos2num.find(p2);
						if(it == m_pos2num.end() ||
							it->second != PUZ_UNKNOWN &&
							it->second != t.m_nums[j]
						)
							return false;
					}
					return true;
				}())
					m_lits.emplace_back(p, i);
			}
		}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int n);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE) / 4; }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<int>> m_matches;
	set<Position> m_horz_walls, m_vert_walls;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int i = 0; i < g.m_lits.size(); ++i){
		auto& lit = g.m_lits[i];
		auto& p = lit.first;
		for(auto& os : tetrominoes[lit.second])
			m_matches[p + os].push_back(i);
	}
}

bool puz_state::make_move(int n)
{
	auto& lit = m_game->m_lits[n];
	auto& p = lit.first;
	auto& t = m_game->m_tetros[lit.second];

	for(auto& os : t.m_horz_walls)
		m_horz_walls.insert(p + os);
	for(auto& os : t.m_vert_walls)
		m_vert_walls.insert(p + os);

	set<int> lit_ids;
	for(int i = 0; i < 4; ++i){
		auto p2 = p + t.m_offset[i];
		cells(p2) = t.m_nums[i] + '0';
		auto& v = m_matches.at(p2);
		lit_ids.insert(v.begin(), v.end());
		m_matches.erase(p2);
	}
	for(auto& kv : m_matches){
		auto& v = kv.second;
		for(int i : lit_ids)
			boost::remove_erase(v, i);
		if(v.empty())
			return false;
	}
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<int>>& kv1,
		const pair<const Position, vector<int>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(int n : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(n))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-walls
		for(int c = 0; c < sidelen(); ++c)
			out << (m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
		out << endl;
		if(r == sidelen()) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-walls
			out << (m_vert_walls.count(p) == 1 ? '|' : ' ');
			if(c == sidelen()) break;
			out << cells(p);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_FenceLits()
{
	using namespace puzzles::FenceLits;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\FenceLits.xml", "Puzzles\\FenceLits.txt", solution_format::GOAL_STATE_ONLY);
}