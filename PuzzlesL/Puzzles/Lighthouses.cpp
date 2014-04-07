#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 9/Lighthouses

	Summary
	Lighten Up at Sea

	Description
	1. You are at sea and you need to find the lighthouses and light the boats.
	2. Each boat has a number on it that tells you how many lighthouses are lighting it.
	3. A lighthouse lights all the tiles horizontally and vertically and doesn't
	   stop at boats or other lighthouses.
	4. Finally, no boat touches another boat or lighthouse, not even diagonally.
	   No lighthouse touches another lighthouse as well.
*/

namespace puzzles{ namespace Lighthouses{

#define PUZ_SPACE			' '
#define PUZ_EMPTY			'.'
#define PUZ_BOAT			'T'
#define PUZ_LIGHTHOUSE		'L'
#define PUZ_BOUNDARY		'B'

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

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen - 2; ++c){
			char ch = str[c];
			if(ch == PUZ_SPACE)
				m_start.push_back(ch);
			else{
				m_start.push_back(PUZ_BOAT);
				m_pos2num[{r + 1, c + 1}] = ch - '0';
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

typedef pair<vector<Position>, string> puz_move;

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, const puz_move& kv);
	void make_move2(const Position& p, const puz_move& kv);
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
	map<Position, vector<puz_move>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	for(auto& kv : g.m_pos2num)
		for(auto& os : offset){
			char& ch = cells(kv.first + os);
			if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
		}

	for(const auto& kv : g.m_pos2num)
		m_matches[kv.first];
	
	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		perms.clear();

		vector<Position> rng_s, rng_l;
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i * 2];
			for(auto p2 = p + os; ; p2 += os){
				char ch = cells(p2);
				if(ch == PUZ_SPACE)
					rng_s.push_back(p2);
				else if(ch == PUZ_LIGHTHOUSE)
					rng_l.push_back(p2);
				else if(ch == PUZ_BOUNDARY)
					break;
			}
		}

		int ns = rng_s.size(), nl = rng_l.size();
		int n = m_game->m_pos2num.at(p), m = n - nl;

		if(m >= 0 && m <= ns){
			auto perm = string(ns - m, PUZ_EMPTY) + string(m, PUZ_LIGHTHOUSE);
			vector<Position> rng(m);
			do{
				for(int i = 0, j = 0; i < perm.length(); ++i)
					if(perm[i] == PUZ_LIGHTHOUSE)
						rng[j++] = rng_s[i];
				if([&]{
					// no touching
					for(const auto& p1 : rng)
						for(const auto& p2 : rng)
							if(boost::algorithm::any_of_equal(offset, p1 - p2))
								return false;
					return true;
				}())
					perms.emplace_back(rng_s, perm);
			}while(boost::next_permutation(perm));
		}

		if(!init)
			switch(perms.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perms.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const Position& p, const puz_move& kv)
{
	auto& rng = kv.first;
	auto& str = kv.second;
	for(int i = 0; i < rng.size(); ++i){
		auto& p2 = rng[i];
		if((cells(p2) = str[i]) == PUZ_LIGHTHOUSE)
			for(auto& os : offset){
				char& ch = cells(p2 + os);
				if(ch == PUZ_SPACE)
					ch = PUZ_EMPTY;
			}
	}

	++m_distance;
	m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, const puz_move& kv)
{
	m_distance = 0;
	make_move2(p, kv);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<puz_move>>& kv1,
		const pair<const Position, vector<puz_move>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(const auto& kv2 : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, kv2))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_BOAT)
				out << format("%2d") % m_game->m_pos2num.at(p);
			else
				out << ' ' << ch;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Lighthouses()
{
	using namespace puzzles::Lighthouses;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Lighthouses.xml", "Puzzles\\Lighthouses.txt", solution_format::GOAL_STATE_ONLY);
}