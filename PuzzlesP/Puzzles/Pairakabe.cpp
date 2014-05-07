#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 12/Pairakabe

	Summary
	Just to confuse things a bit more

	Description
	1. Plays like Nurikabe, with an interesting twist.
	2. Instead of just one number, each 'garden' contains two numbers and
	   the area of the garden is given by the sum of both.
*/

namespace puzzles{ namespace Pairakabe{

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
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1, n = 0; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(PUZ_WALL);
			else{
				m_start.push_back('a' + n++);
				m_pos2garden[{r, c}] = ch - '0';
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
}

typedef pair<vector<Position>, int> puz_garden;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p);
	bool is_continuous() const;

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
			m_gardens.push_back({{kv.first}, kv.second - 1});

	boost::sort(m_gardens, [](const puz_garden& kv1, const puz_garden& kv2){
		return kv1.second > kv2.second;
	});

	for(int r = 1; r < g.m_sidelen - 2; ++r)
		for(int c = 1; c < g.m_sidelen - 2; ++c){
			set<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
			if(boost::algorithm::all_of(rng, [&](const Position& p){
				return cells(p) == PUZ_WALL;
			}))
				m_2by2walls.push_back(rng);
		}
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
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
	for(int i = 0; i < length(); ++i)
		if((*this)[i] == PUZ_WALL)
			a.insert({i / sidelen(), i % sidelen()});

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(a, smoves);
	return smoves.size() == a.size();
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

	boost::remove_erase_if(m_2by2walls, [&](const set<Position>& rng){
		return rng.count(p) != 0;
	});

	if(boost::algorithm::any_of(m_2by2walls, [&](const set<Position>& rng){
		return boost::algorithm::none_of(rng, [&](const Position& p2){
			return boost::algorithm::any_of(m_gardens, [&](const puz_garden& g){
				return boost::algorithm::any_of(g.first, [&](const Position& p3){
					return manhattan_distance(p2, p3) <= g.second;
				});
			});
		});
	}))
		return false;

	return is_continuous();
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
	for(int r = 1; r < sidelen() - 1; ++r){
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

void solve_puz_Pairakabe()
{
	using namespace puzzles::Pairakabe;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Pairakabe.xml", "Puzzles\\Pairakabe.txt", solution_format::GOAL_STATE_ONLY);
}