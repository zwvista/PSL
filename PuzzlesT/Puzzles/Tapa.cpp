#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 9/Tapa

	Summary
	Turkish art of PAint(TAPA)

	Description
	1. The goal is to fill some tiles forming a single orthogonally continuous
	   path. Just like Tapa.
	2. A number indicates how many of the surrounding tiles are filled. If a
	   tile has more than one number, it hints at multiple separated groups
	   of filled tiles.
	3. For example, a cell with a 1 and 3 means there is a  continuous group
	   of 3 filled cells around it and one more single filled cell, separated
	   from the other 3. The order of the numbers in this case is irrelevant.
	4. Filled tiles can't cover an area of 2*2 or larger (just like Tapa).
	   Tiles with numbers can be considered 'empty'.

	Variations
	5. Tapa has plenty of variations. Some are available in the levels of this
	   game. Stronger variations are B-W Tapa, Island Tapa and Pata and have
	   their own game.
	6. Equal Tapa - The board contains an equal number of white and black tiles.
	   Tiles with numbers or question marks are NOT counted as empty or filled
	   for this rule (i.e. they're left out of the count).
	7. Four-Me-Tapa - Four-Me-Not rule apply: you can't have more than three
	   filled tiles in line.
	8. No-Square Tapa - No 2*2 area of the board can be left empty.
*/

namespace puzzles{ namespace Tapa{

#define PUZ_SPACE		' '
#define PUZ_QM			'?'
#define PUZ_FILLED		'F'
#define PUZ_EMPTY		'.'
#define PUZ_HINT		'H'
#define PUZ_BOUNDARY	'B'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

typedef pair<int, int> puz_hint;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, puz_hint> m_pos2hint;
	map<puz_hint, vector<string>> m_hint2perms;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
{
	auto f = [](char ch) {
		return ch == PUZ_QM ? -1 : ch == PUZ_SPACE ? 0 : ch - '0';
	};

	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			auto s = str.substr(c * 2 - 2, 2);
			if(s == "  ")
				m_start.push_back(PUZ_FILLED);
			else{
				m_start.push_back(PUZ_HINT);
				int n1 = f(s[0]), n2 = f(s[1]);
				if(n1 != -1 && n1 < n2)
					std::swap(n1, n2);
				m_pos2hint[{r, c}] = {n1, n2};
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);

	// A tile is surrounded by at most 8 tiles, each of which has two states:
	// filled or empty. So all combinations of the states of the
	// surrounding tiles can be coded into an 8-bit number(0 -- 255).
	for(int i = 1; i < 256; ++i){
		vector<int> filled;
		for(int j = 0; j < 8; ++j)
			if(i & (1 << j))
				filled.push_back(j);

		vector<int> hint;
		for(int j = 0; j < filled.size(); ++j)
			if(j == 0 || filled[j] - filled[j - 1] != 1)
				hint.push_back(1);
			else
				++hint.back();
		if(filled.size() > 1 && hint.size() > 1 && filled.front() == 0 && filled.back() == 7)
			hint.front() += hint.back(), hint.pop_back();
		if(hint.size() > 2)
			continue;

		string perm(8, PUZ_EMPTY);
		for(int j : filled)
			perm[j] = PUZ_FILLED;
		int n1 = hint[0], n2 = hint.size() == 1 ? 0 : hint[1];
		if(n1 < n2)
			std::swap(n1, n2);
		m_hint2perms[{n1, n2}].push_back(perm);
		m_hint2perms[{-1, n2 == 0 ? 0 : -1}].push_back(perm);
	}
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
	int i = s.find(PUZ_FILLED);
	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_state->cells(p2) == PUZ_FILLED){
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
		if(ch2 != PUZ_BOUNDARY && ch2 != PUZ_FILLED && ch2 != ch)
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
	return smoves.size() == boost::count(*this, PUZ_FILLED);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(auto& p : m_gardens.back().first)
		for(auto& os : offset){
			auto p2 = p + os;
			if(cells(p2) == PUZ_FILLED){
				children.push_back(*this);
				if(!children.back().make_move(p2))
					children.pop_back();
			}
		}
}

ostream& puz_state::dump(ostream& out) const
{
	//for(int r = 1; r < sidelen() - 1; ++r) {
	//	for(int c = 1; c < sidelen() - 1; ++c){
	//		Position p(r, c);
	//		char ch = cells(p);
	//		if(ch == PUZ_FILLED)
	//			out << ch << ' ';
	//		else{
	//			auto it = m_game->m_pos2garden.find(p);
	//			if(it == m_game->m_pos2garden.end())
	//				out << ". ";
	//			else
	//				out << format("%-2d") % it->second;
	//		}
	//	}
	//	out << endl;
	//}
	return out;
}

}}

void solve_puz_Tapa()
{
	using namespace puzzles::Tapa;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Tapa.xml", "Puzzles\\Tapa.txt", solution_format::GOAL_STATE_ONLY);
}