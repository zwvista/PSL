#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Nurikabe

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

namespace puzzles{ namespace nurikabe{

#define PUZ_SPACE		' '
#define PUZ_WALL		'W'
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
	map<Position, int> m_pos2garden;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 0, n = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen - 2; ++c){
			switch(char ch = str[c]){
			case PUZ_SPACE:
				m_start.push_back(PUZ_WALL);
				break;
			default:
				m_start.push_back('a' + n++);
				m_pos2garden[Position(r + 1, c + 1)] = ch - '0';
				break;
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

typedef pair<vector<Position>, int> puz_garden;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_gardens, 0, [](int acc, const puz_garden& g){
			return acc + g.second;
		});
	}
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	vector<puz_garden> m_gardens;
	vector<set<Position>> m_2by2walls;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2garden)
		if(kv.second > 1)
			m_gardens.emplace_back(vector<Position>{kv.first}, kv.second - 1);

	boost::sort(m_gardens, [](const puz_garden& kv1, const puz_garden& kv2){
		return kv1.second > kv2.second;
	});

	for(int r = 1; r < g.m_sidelen - 2; ++r)
		for(int c = 1; c < g.m_sidelen - 2; ++c){
			Position p1(r, c), p2(r, c + 1), p3(r + 1, c), p4(r + 1, c + 1);
			if(cells(p1) == PUZ_WALL && cells(p2) == PUZ_WALL &&
				cells(p3) == PUZ_WALL && cells(p4) == PUZ_WALL)
				m_2by2walls.push_back({p1, p2, p3, p4});
		}
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_state->sidelen(); }
	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
	int i = s.find(PUZ_WALL);
	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_state->cells(p2) == PUZ_WALL){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move(const Position& p)
{
	auto& g = m_gardens.back();
	char ch = cells(g.first.front());
	cells(p) = ch;
	g.first.push_back(p);
	if(--g.second == 0)
		m_gardens.pop_back();

	for(auto& os : offset){
		char ch2 = cells(p + os);
		if(ch2 != PUZ_BOUNDARY && ch2 != PUZ_WALL && ch2 != ch)
			return false;
	}

	boost::remove_erase_if(m_2by2walls, [&](const set<Position>& ps){
		return ps.count(p) != 0;
	});

	if(boost::algorithm::any_of(m_2by2walls, [&](const set<Position>& ps){
		return boost::algorithm::none_of(ps, [&](const Position& p2){
			return boost::algorithm::any_of(m_gardens, [&](const puz_garden& g){
				return boost::algorithm::any_of(g.first, [&](const Position& p3){
					return manhattan_distance(p2, p3) <= g.second;
				});
			});
		});
	}))
		return false;

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.size() == boost::count(*this, PUZ_WALL);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(auto& p : m_gardens.back().first)
		for(auto& os : offset){
			auto p2 = p + os;
			if(cells(p2) == PUZ_WALL){
				children.push_back(*this);
				if(!children.back().make_move(p2))
					children.pop_back();
			}
		}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_WALL)
				out << ch << ' ';
			else{
				auto it = m_game->m_pos2garden.find(p);
				if(it == m_game->m_pos2garden.end())
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

void solve_puz_nurikabe()
{
	using namespace puzzles::nurikabe;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\nurikabe.xml", "Puzzles\\nurikabe.txt", solution_format::GOAL_STATE_ONLY);
}