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
#define PUZ_UNKNOWN		-1

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
};

typedef vector<int> puz_hint;

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
		return ch == PUZ_QM ? PUZ_UNKNOWN : ch == PUZ_SPACE ? 0 : ch - '0';
	};

	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			auto s = str.substr(c * 4 - 4, 4);
			if(s == "    ")
				m_start.push_back(PUZ_SPACE);
			else{
				m_start.push_back(PUZ_HINT);
				auto& hint = m_pos2hint[{r, c}];
				for(int i = 0, n = 0; i < 4; ++i)
					if((n = f(s[i])) != 0)
						hint.push_back(n);
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
		if(filled.size() > 1 && hint.size() > 1 && filled.back() - filled.front() == 7)
			hint.front() += hint.back(), hint.pop_back();

		string perm(8, PUZ_EMPTY);
		for(int j : filled)
			perm[j] = PUZ_FILLED;

		boost::sort(hint);
		m_hint2perms[hint].push_back(perm);

		boost::fill(hint, PUZ_UNKNOWN);
		m_hint2perms[hint].push_back(perm);
	}
}

typedef pair<vector<Position>, int> puz_garden;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& os : offset)
			chars.push_back(cells(p + os));

		auto& perms = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p));
		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return (ch1 == PUZ_BOUNDARY || ch1 == PUZ_HINT) && ch2 == PUZ_EMPTY ||
					ch1 == PUZ_SPACE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perm_ids.front()) ? 1 : 0;
			}
	}
	return 2;
}

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(const auto& kv : g.m_pos2hint){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_hint2perms.at(kv.second).size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
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
	int i = boost::find_if(s, [](char ch){
		return ch == PUZ_SPACE || ch == PUZ_FILLED;
	}) - s.begin();

	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch == PUZ_SPACE || ch == PUZ_FILLED){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move2(const Position& p, int n)
{
	auto& perm = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p))[n];
	for(int i = 0; i < 8; ++i){
		auto p2 = p + offset[i];
		char& ch = cells(p2);
		if(ch == PUZ_SPACE)
			ch = perm[i];
	}

	++m_distance;
	m_matches.erase(p);

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.size() == boost::count_if(*this, [](char ch){
		return ch == PUZ_SPACE || ch == PUZ_FILLED;
	});
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move2(p, n))
		return false;
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_matches.empty()){
		auto& kv = *boost::min_element(m_matches, [](
			const pair<const Position, vector<int>>& kv1,
			const pair<const Position, vector<int>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});

		for(int n : kv.second){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, n))
				children.pop_back();
		}
	}
	else{
	}
}

ostream& puz_state::dump(ostream& out) const
{
	auto f = [&](int n)->char{
		return n == PUZ_UNKNOWN ? PUZ_QM : n == 0 ? PUZ_SPACE : n + '0';
	};

	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_HINT){
				auto& hint = m_game->m_pos2hint.at(p);
				for(int i : hint)
					out << f(i);
				out << string(4 - hint.size(), ' ');
			}
			else
				out << (ch == PUZ_SPACE ? PUZ_FILLED : ch) << "   ";
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Tapa()
{
	using namespace puzzles::Tapa;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Tapa.xml", "Puzzles\\Tapa.txt", solution_format::GOAL_STATE_ONLY);
}