#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Galaxies

	Summary
	Fill the Symmetric Spiral Galaxies

	Description
	1. In the board there are marked centers of a few 'Spiral' Galaxies.
	2. These Galaxies are symmetrical to a rotation of 180 degrees. This
	   means that rotating the shape of the Galaxy by 180 degrees (half a
	   full turn) around the center, will result in an identical shape.
	3. In the end, all the space must be included in Galaxies and Galaxies
	   can't overlap.
	4. There can be single tile Galaxies (with the center inside it) and
	   some Galaxy center will be cross two or four tiles.
*/

namespace puzzles{ namespace Galaxies{

#define PUZ_SPACE		' '
#define PUZ_BOUNDARY	'+'
#define PUZ_GALAXY		'o'
#define PUZ_GALAXY_R	'>'
#define PUZ_GALAXY_C	'v'
#define PUZ_GALAXY_RC	'x'

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
	vector<Position> m_galaxies;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		for(int c = 1; c < m_sidelen - 1; ++c)
			switch(str[c - 1]){
			case PUZ_GALAXY:
				m_galaxies.emplace_back(r * 2, c * 2);
				break;
			case PUZ_GALAXY_R:
				m_galaxies.emplace_back(r * 2, c * 2 + 1);
				break;
			case PUZ_GALAXY_C:
				m_galaxies.emplace_back(r * 2 + 1, c * 2);
				break;
			case PUZ_GALAXY_RC:
				m_galaxies.emplace_back(r * 2 + 1, c * 2 + 1);
				break;
			}
	}
}

struct puz_galaxy
{
	set<Position> m_inner, m_outer;
	Position m_center;
	char m_ch;
	bool m_ready = false;
	bool m_center_in_cell = false;

	puz_galaxy() {}
	puz_galaxy(const Position& p, char ch) : m_center(p), m_ch(ch) {
		Position p2(m_center.first / 2, m_center.second / 2);
		m_inner.insert(p2);
		if(p.first % 2 == 1 && p.second % 2 == 1)
			m_inner.insert(p2 + Position(0, 1));
		else if(p.first % 2 == 0 && p.second % 2 == 0)
			m_center_in_cell = true;
	}
};

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int n, const Position& p);
	void adjust_galaxies();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
	unsigned int get_distance(const puz_state& child) const { return 2; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	vector<puz_galaxy> m_galaxies;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) =
		PUZ_BOUNDARY;

	char ch = 'a';
	for(auto& p : g.m_galaxies){
		puz_galaxy g(p, ch++);
		for(auto& p2 : g.m_inner)
			cells(p2) = cells(g.m_center - p2) = g.m_ch;
		m_galaxies.push_back(g);
	}

	adjust_galaxies();
}

void puz_state::adjust_galaxies()
{
	for(auto& g : m_galaxies){
		g.m_outer.clear();
		if(g.m_ready) continue;
		for(auto& p : g.m_inner)
			for(int i = 0; i < 4; ++i){
				if(g.m_center_in_cell && g.m_inner.size() == 1 && i > 1) continue;
				auto p2 = p + offset[i];
				auto p3 = g.m_center - p2;
				if(cells(p2) == PUZ_SPACE && cells(p3) == PUZ_SPACE)
					g.m_outer.insert(p2);
			}
		if(g.m_outer.empty())
			g.m_ready = true;
	}
}

bool puz_state::make_move(int n, const Position& p)
{
	auto& g = m_galaxies[n];
	for(auto&& p2 : {p, g.m_center - p}){
		g.m_inner.insert(p2);
		cells(p2) = g.m_ch;
	}
	adjust_galaxies();
	return is_goal_state() || boost::algorithm::any_of(m_galaxies, [](const puz_galaxy& g){
		return !g.m_outer.empty();
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(int i = 0; i < m_galaxies.size(); ++i){
		auto& g = m_galaxies[i];
		for(auto& p : g.m_outer){
			children.push_back(*this);
			if(!children.back().make_move(i, p))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1;; ++r){
		// draw horz-walls
		for(int c = 1; c < sidelen() - 1; ++c)
			out << (cells({r - 1, c}) != cells({r, c}) ? " -" : "  ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 1;; ++c){
			Position p(r, c);
			// draw vert-walls
			out << (cells({r, c - 1}) != cells({r, c}) ? '|' : ' ');
			if(c == sidelen() - 1) break;
			out << '.';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Galaxies()
{
	using namespace puzzles::Galaxies;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Galaxies.xml", "Puzzles\\Galaxies.txt", solution_format::GOAL_STATE_ONLY);
}