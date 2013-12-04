#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Hitori

	Summary
	Darken some tiles so no number appears in a row or column more than once

	Description
	1. The goal is to shade squares so that a number appears only once in a
	   row or column.
	2. While doing that, you must take care that shaded squares don't touch
	   horizontally or vertically between them.
	3. In the end all the un-shaded squares must form a single continuous area.
*/

namespace puzzles{ namespace hitori{

#define PUZ_SHADED		100
#define PUZ_CONNECTED	99
#define PUZ_BOUNDARY	0

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

typedef map<pair<int, int>, set<Position>> puz_shaded;

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<int> m_start;
	puz_shaded m_shaded;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);
	for(int r = 0, n = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 0; c < m_sidelen - 2; ++c){
			Position p(r + 1, c + 1);
			int n = atoi(str.substr(c * 2, 2).c_str());
			m_start.push_back(n);
			m_shaded[make_pair(r + 1, n)].insert(p);
			m_shaded[make_pair(m_sidelen + c + 1, n)].insert(p);
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);

	auto it = m_shaded.begin();
	while((it = find_if(it, m_shaded.end(), [](const puz_shaded::value_type& kv){
		return kv.second.size() <= 1;
	})) != m_shaded.end())
		it = m_shaded.erase(it);
}

typedef pair<vector<Position>, int> puz_garden;

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	int& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const pair<int, int>& key, const Position& p);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_shaded, 0, [](int acc, const puz_shaded::value_type& kv){
			return acc + kv.second.size() - 1;
		});
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	puz_shaded m_shaded;
	int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_start), m_game(&g), m_shaded(g.m_shaded)
{
}

struct puz_state2 : vector<int>
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_game->m_sidelen; }
	int cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	void make_move(int i){ (*this)[i] = PUZ_CONNECTED; }
	void gen_children(list<puz_state2>& children) const;

	const puz_game* m_game;
};

puz_state2::puz_state2(const puz_state& s)
: vector<int>(s), m_game(s.m_game)
{
	int i = boost::find_if(*this, [](int n){
		return n != PUZ_BOUNDARY && n != PUZ_SHADED;
	}) - begin();
	make_move(i);
}

void puz_state2::gen_children(list<puz_state2> &children) const
{
	for(int i = 0; i < size(); ++i){
		Position p(i / sidelen(), i % sidelen());
		switch(at(i)){
		case PUZ_BOUNDARY:
		case PUZ_CONNECTED:
		case PUZ_SHADED:
			break;
		default:
			for(auto& os : offset){
				auto p2 = p + os;
				if(cell(p2) == PUZ_CONNECTED){
					children.push_back(*this);
					children.back().make_move(i);
					return;
				}
			}
		}
	}
}

bool puz_state::make_move(const pair<int, int>& key, const Position& p)
{
	cell(p) = PUZ_SHADED;

	m_distance = 0;
	auto f = [&](const pair<int, int>& k){
		auto it = m_shaded.find(k);
		if(it == m_shaded.end()) return;
		++m_distance;
		it->second.erase(p);
		if(it->second.size() == 1)
			m_shaded.erase(it);
	};
	f(key);
	f({key.first < sidelen() ? sidelen() + p.second : p.first, key.second});

	for(auto& os : offset){
		auto p2 = p + os;
		if(cell(p2) == PUZ_SHADED)
			return false;
	}

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	const auto& state = smoves.back();
	return boost::range::find_if(state, [](int n){
		return n != PUZ_BOUNDARY && n != PUZ_CONNECTED && n != PUZ_SHADED;
	}) == state.end();
}

void puz_state::gen_children(list<puz_state> &children) const
{
	auto& kv = *boost::min_element(m_shaded, [](
		const puz_shaded::value_type& kv1,
		const puz_shaded::value_type& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(auto& p : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			int n = cell(p);
			if(n == PUZ_SHADED)
				out << " .";
			else
				out << format("%2d") % n;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_hitori()
{
	using namespace puzzles::hitori;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\hitori.xml", "Puzzles\\hitori.txt", solution_format::GOAL_STATE_ONLY);
}