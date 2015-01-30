#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 1/Nurikabe

	Summary
	Draw a continuous wall that divides gardens as big as the numbers

	Description
	1. Each number on the grid indicates a garden, occupying as many tiles
	   as the number itself.
	2. Gardens can have any form, extending horizontally and vertically, but
	   can't extend diagonally.
	3. The garden is separated by a single continuous wall. This means all
	   wall tiles on the board must be connected horizontally or vertically.
	   There can't be isolated walls.
	4. You must find and mark the wall following these rules:
	5. All the gardens in the puzzle are numbered at the start, there are no
	   hidden gardens.
	6. A wall can't go over numbered squares.
	7. The wall can't form 2*2 squares.
*/

namespace puzzles{ namespace Nurikabe{

#define PUZ_SPACE		' '
#define PUZ_WALL		'W'
#define PUZ_BOUNDARY	'+'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_garden
{
	set<Position> m_inner, m_outer;
	int m_remaining;

	puz_garden() {}
	puz_garden(const Position& p, int cnt) : m_remaining(cnt){
		m_inner.insert(p);
	}
	bool operator<(const puz_garden& x) const {
		return m_remaining < x.m_remaining;
	}
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	map<char, puz_garden> m_ch2garden;
	string m_start;
	int m_max_size = 0;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	char ch_g = 'a';
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(PUZ_SPACE);
			else{
				int n = ch - '0';
				Position p(r, c);
				m_pos2num[p] = n;
				if(n == 1)
					m_start.push_back('.');
				else{
					m_ch2garden[ch_g] = {p, n - 1};
					m_start.push_back(ch_g++);
					m_max_size = max(m_max_size, n);
				}
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(char ch, const Position& p);
	bool make_move2(char ch, const Position& p);
	int adjust_area(bool init);
	void check_board();
	bool is_continuous() const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::count(*this, PUZ_SPACE);
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<char, puz_garden> m_ch2garden;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_ch2garden(g.m_ch2garden)
{
	for(auto& kv : g.m_pos2num)
		if(kv.second == 1)
			for(auto& os : offset){
				char& ch = cells(kv.first + os);
				if(ch == PUZ_SPACE)
					ch = PUZ_WALL;
			}

	check_board();
	adjust_area(true);
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s, const Position& starting)
		: Position(starting), m_state(&s)
	{}

	void make_move(const Position& p){
		static_cast<Position&>(*this) = p;
		++m_distance;
	}
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
	int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	if(m_state->cells(*this) != PUZ_SPACE ||
		m_distance == m_state->m_game->m_max_size)
		return;
	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch != PUZ_WALL && ch != PUZ_BOUNDARY){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

void puz_state::check_board()
{
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char& ch = cells(p);
			if(ch != PUZ_SPACE) continue;

			list<puz_state2> smoves;
			puz_move_generator<puz_state2>::gen_moves({*this, p}, smoves);
			if(boost::algorithm::none_of(smoves, [&](const puz_state2& s2){
				auto it = m_ch2garden.find(cells(s2));
				return it != m_ch2garden.end() &&
					s2.m_distance <= it->second.m_remaining;
			})){
				// Cells that cannot be reached by any garden must be a wall
				ch = PUZ_WALL, ++m_distance;
				continue;
			}

			set<char> chars;
			for(auto& os : offset){
				char ch2 = cells(p + os);
				if(ch2 != PUZ_BOUNDARY && ch2 != PUZ_SPACE && ch2 != PUZ_WALL)
					chars.insert(ch2);
			}
			// Gardens must be separated by a wall
			if(chars.size() > 1)
				ch = PUZ_WALL, ++m_distance;
		}
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_ch2garden){
		char ch = kv.first;
		auto& g = kv.second;
		auto& outer = g.m_outer;
		outer.clear();

		for(auto& p : g.m_inner)
			for(auto& os : offset){
				auto p2 = p + os;
				if(cells(p2) == PUZ_SPACE)
					outer.insert(p2);
			}

		if(!init)
			switch(outer.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(ch, *outer.begin()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state3 : Position
{
	puz_state3(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state3>& children) const;

	const set<Position>* m_rng;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_rng->count(p2) != 0){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::is_continuous() const
{
	set<Position> a;
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_SPACE || ch == PUZ_WALL)
				a.insert(p);
		}

	list<puz_state3> smoves;
	puz_move_generator<puz_state3>::gen_moves(a, smoves);
	return smoves.size() == a.size();
}

bool puz_state::make_move2(char ch, const Position& p)
{
	auto& g = m_ch2garden.at(ch);
	cells(p) = ch, ++m_distance;
	g.m_inner.insert(p);
	if(--g.m_remaining == 0){
		for(auto& p2 : g.m_inner)
			for(auto& os : offset){
				char& ch2 = cells(p2 + os);
				if(ch2 == PUZ_SPACE)
					ch2 = PUZ_WALL, ++m_distance;
			}
		m_ch2garden.erase(ch);
	}

	check_board();

	// pruning
	for(int r = 1; r < sidelen() - 2; ++r)
		for(int c = 1; c < sidelen() - 2; ++c){
			vector<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
			if(boost::algorithm::all_of(rng, [&](const Position& p){
				return cells(p) == PUZ_WALL;
			}))
				return false;
		}

	return is_continuous();
}

bool puz_state::make_move(char ch, const Position& p)
{
	m_distance = 0;
	if(!make_move2(ch, p))
		return false;
	int m;
	while((m = adjust_area(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_ch2garden, [](
		const pair<char, puz_garden>& kv1,
		const pair<char, puz_garden>& kv2){
		return kv1.second < kv2.second;
	});
	for(auto& p : kv.second.m_outer){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_WALL)
				out << PUZ_WALL << ' ';
			else{
				auto it = m_game->m_pos2num.find(p);
				if(it == m_game->m_pos2num.end())
					out << ". ";
				else
					out << format("%-2d") % it->second;
			}
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Nurikabe()
{
	using namespace puzzles::Nurikabe;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Nurikabe.xml", "Puzzles\\Nurikabe.txt", solution_format::GOAL_STATE_ONLY);
}