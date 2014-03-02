#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/Minesweeper

	Summary
	You know the drill :)

	Description
	1. Find the mines on the field.
	2. Numbers tell you how many mines there are close by, touching that
	   number horizontally, vertically or diagonally.
*/

namespace puzzles{ namespace Minesweeper{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_MINE		'M'
#define PUZ_BOUNDARY	'B'
	
const Position offset[] = {
	{-1, 0},	// n
	{-1, 1},	// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},	// sw
	{0, -1},	// w
	{-1, -1},	// nw
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	map<Position, int> m_pos2num;
	// all permutations
	vector<vector<string>> m_num2perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
, m_num2perms(9)
{
	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen - 2; ++c){
			char ch = str[c];
			m_start.push_back(ch == PUZ_SPACE ? PUZ_SPACE : PUZ_EMPTY);
			if(ch != PUZ_SPACE)
				m_pos2num[{r + 1, c + 1}] = ch - '0';
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);

	for(int i = 0; i <= 8; ++i){
		auto perm = string(8 - i, PUZ_EMPTY) + string(i, PUZ_MINE);
		do
			m_num2perms[i].push_back(perm);
		while(boost::next_permutation(perm));
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, int n);
	void make_move2(const Position& p, int n);
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
	map<Position, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2num){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_num2perms[kv.second].size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& p = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& os : offset)
			chars.push_back(cells(p + os));

		auto& perms = m_game->m_num2perms[m_game->m_pos2num.at(p)];
		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == ch2 ||
					ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perm_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
	auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];

	for(int k = 0; k < perm.size(); ++k){
		char& ch = cells(p + offset[k]);
		if(ch == PUZ_SPACE)
			ch = perm[k];
	}

	++m_distance;
	m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	make_move2(p, n);
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
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
		if(!children.back().make_move(kv.first, n))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			auto it = m_game->m_pos2num.find(p);
			if(it == m_game->m_pos2num.end())
				out << format("%-2s") % cells(p);
			else
				out << format("%-2d") % it->second;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Minesweeper()
{
	using namespace puzzles::Minesweeper;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Minesweeper.xml", "Puzzles\\Minesweeper.txt", solution_format::GOAL_STATE_ONLY);
}