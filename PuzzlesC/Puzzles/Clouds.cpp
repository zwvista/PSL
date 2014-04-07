#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Clouds

	Summary
	Weather Radar Report

	Description
	1. You must find Clouds in the sky.
	2. The hints on the borders tell you how many tiles are covered by Clouds
	   in that row or column.
	3. Clouds only appear in rectangular or square areas. Furthermore, their
	   width and height is always at least two tiles wide.
	4. Clouds can't touch between themselves, not even diagonally. 
*/

namespace puzzles{ namespace Clouds{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_CLOUD		'C'
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
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	Position m_cloud_max_size;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen{strs.size() + 1}
, m_piece_counts_rows(m_sidelen)
, m_piece_counts_cols(m_sidelen)
{
	for(int r = 1; r < m_sidelen; ++r){
		auto& str = strs[r - 1];
		for(int c = 1; c < m_sidelen; c++){
			char ch = str[c - 1];
			if(ch != PUZ_SPACE)
				(c == m_sidelen - 1 ? m_piece_counts_rows[r] : m_piece_counts_cols[c])
				= ch - '0';
		}
	}
	m_cloud_max_size = {
		*boost::max_element(m_piece_counts_cols),
		*boost::max_element(m_piece_counts_rows)
	};
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int h, int w);
	void check_area();
	void find_matches();

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return 
		boost::accumulate(m_piece_counts_rows, 0);
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	vector<pair<Position, Position>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
, m_piece_counts_rows(g.m_piece_counts_rows)
, m_piece_counts_cols(g.m_piece_counts_cols)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

	check_area();
	find_matches();
}

void puz_state::check_area()
{
	auto f = [](char& ch){
		if(ch == PUZ_SPACE)
			ch = PUZ_EMPTY;
	};

	for(int i = 1; i < sidelen() - 1; ++i){
		if(m_piece_counts_rows[i] == 0)
			for(int j = 1; j < sidelen() - 1; ++j)
				f(cells({i, j}));
		if(m_piece_counts_cols[i] == 0)
			for(int j = 1; j < sidelen() - 1; ++j)
				f(cells({j, i}));
	}
}

void puz_state::find_matches()
{
	m_matches.clear();
	for(int h = 2; h <= m_game->m_cloud_max_size.first; ++h)
		for(int r = 1; r < sidelen() - h; ++r)
			for(int w = 2; w <= m_game->m_cloud_max_size.second; ++w)
				for(int c = 1; c < sidelen() - w; ++c)
					if([=](){
						for(int dr = 0; dr < h; ++dr)
							for(int dc = 0; dc < w; ++dc){
								Position p(r + dr, c + dc);
								if(m_piece_counts_cols[p.second] < h ||
									m_piece_counts_rows[p.first] < w ||
									cells(p) != PUZ_SPACE)
									return false;
							}
						return true;
					}())
						m_matches.push_back({{r, c}, {h, w}});
}

bool puz_state::make_move(const Position& p, int h, int w)
{
	for(int dr = 0; dr < h; ++dr)
		for(int dc = 0; dc < w; ++dc){
			auto p2 = p + Position(dr, dc);
			cells(p2) = PUZ_CLOUD;
			--m_piece_counts_rows[p2.first];
			--m_piece_counts_cols[p2.second];
		}

	auto f = [this](const Position& p){
		char& ch = cells(p);
		if(ch == PUZ_SPACE)
			ch = PUZ_EMPTY;
	};
	for(int dr = -1; dr <= h; ++dr)
		f(p + Position(dr, -1)), f(p + Position(dr, w));
	for(int dc = -1; dc <= w; ++dc)
		f(p + Position(-1, dc)), f(p + Position(h, dc));

	m_distance = h * w;
	check_area();

	// pruning
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p2(r, c);
			auto g = [&](int i, int j){
				return cells(p2 + offset[i]) == PUZ_SPACE ||
					cells(p2 + offset[j]) == PUZ_SPACE;
			};
			if(cells(p2) == PUZ_SPACE && (!g(0, 2) || !g(1, 3)))
				return false;
		}

	find_matches();
	return is_goal_state() || !m_matches.empty();
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(auto& kv : m_matches){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, kv.second.first, kv.second.second))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen(); ++r){
		for(int c = 1; c < sidelen(); ++c)
			if(r == sidelen() - 1 && c == sidelen() - 1)
				break;
			else if(c == sidelen() - 1)
				out << m_game->m_piece_counts_rows[r];
			else if(r == sidelen() - 1)
				out << m_game->m_piece_counts_cols[c];
			else
				out << cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_Clouds()
{
	using namespace puzzles::Clouds;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Clouds.xml", "Puzzles\\Clouds.txt", solution_format::GOAL_STATE_ONLY);
}
