#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 4/Galaxies

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
	string m_start;
	vector<Position> m_galaxies;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	char cells(const Position& p) const { return m_start[p.first * (m_sidelen - 2) + p.second]; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start = boost::accumulate(strs, string());
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
	bool m_center_in_cell = false;

	puz_galaxy() {}
	puz_galaxy(const Position& p) : m_center(p) {
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
	bool make_move(char ch, const Position& p);
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
	map<char, puz_galaxy> m_galaxies;
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
		puz_galaxy glx(p);
		for(auto& p2 : glx.m_inner)
			cells(p2) = cells(glx.m_center - p2) = ch;
		m_galaxies[ch++] = glx;
	}

	adjust_galaxies();
}

void puz_state::adjust_galaxies()
{
	for(auto it = m_galaxies.begin(); it != m_galaxies.end();){
		auto& glx = it->second;
		glx.m_outer.clear();
		for(auto& p : glx.m_inner)
			for(int i = 0; i < 4; ++i){
				if(glx.m_center_in_cell && glx.m_inner.size() == 1 && i > 1) continue;
				auto p2 = p + offset[i];
				auto p3 = glx.m_center - p2;
				if(cells(p2) == PUZ_SPACE && cells(p3) == PUZ_SPACE)
					glx.m_outer.insert(min(p2, p3));
			}
		if(glx.m_outer.empty())
			it = m_galaxies.erase(it);
		else
			++it;
	}
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s, const Position& starting);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s, const Position& starting)
	: m_state(&s)
{
	make_move(starting);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	if(m_state->cells(*this) != PUZ_SPACE)
		return;
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_state->cells(p2) != PUZ_BOUNDARY){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move(char ch, const Position& p)
{
	auto& glx = m_galaxies.at(ch);
	glx.m_inner.insert(p);
	cells(p) = cells(glx.m_center - p) = ch;
	adjust_galaxies();

	set<Position> rng;
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			if(cells(p) == PUZ_SPACE)
				rng.insert(p);
		}
	while(!rng.empty()){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({*this, *rng.begin()}, smoves);
		vector<Position> rng2;
		set<char> rng3;
		for(auto& p : smoves){
			char ch = cells(p);
			if(ch == PUZ_SPACE)
				rng2.push_back(p);
			else
				rng3.insert(ch);
		}
		if(boost::algorithm::any_of(rng2, [&](const Position& p){
			return boost::algorithm::none_of(rng3, [&](char ch){
				auto it = m_galaxies.find(ch);
				if(it == m_galaxies.end())
					return false;
				auto& center = it->second.m_center;
				return p.first <= center.first && p.second <= center.second &&
					this->cells(center - p) == PUZ_SPACE;
			});
		}))
			return false;
		for(auto& p : rng2)
			rng.erase(p);
	}
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(auto& kv : m_galaxies)
		for(auto& p : kv.second.m_outer){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, p))
				children.pop_back();
		}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1;; ++r){
		// draw horz-walls
		for(int c = 1; c < sidelen() - 1; ++c)
			out << (r > 1 && c > 1 && m_game->cells({r - 2, c - 2}) == PUZ_GALAXY_RC ? PUZ_GALAXY : ' ')
			<< (cells({r - 1, c}) != cells({r, c}) ? '-' :
			r > 1 && m_game->cells({r - 2, c - 1}) == PUZ_GALAXY_C ? PUZ_GALAXY : ' ');
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 1;; ++c){
			Position p(r, c);
			// draw vert-walls
			out << (cells({r, c - 1}) != cells({r, c}) ? '|' :
				c > 1 && m_game->cells({r - 1, c - 2}) == PUZ_GALAXY_R ? PUZ_GALAXY : ' ');
			if(c == sidelen() - 1) break;
			out << (m_game->cells({r - 1, c - 1}) == PUZ_GALAXY ? PUZ_GALAXY : '.');
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