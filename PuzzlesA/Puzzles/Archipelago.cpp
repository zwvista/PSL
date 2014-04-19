#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 8/Archipelago

	Summary
	No bridges, just find the water

	Description
	1. Each number represents a rectangular island in the Archipelago.
	2. The number in itself identifies how many squares the island occupies.
	3. Islands can only touch each other diagonally and by touching they
	   must form a network where no island is isolated from the others.
	4. In other words, every island must be touching another island diagonally
	   and no group of islands must be separated from the others.
	5. Not all the islands you need to find are necessarily marked by numbers.
	6. Finally, no 2*2 square can be occupied by water only (just like Nurikabe).
*/

namespace puzzles{ namespace Archipelago{

#define PUZ_SPACE		' '
#define PUZ_ISLAND		'.'
#define PUZ_WATER		'W'

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

struct puz_island_info
{
	// the area of the island
	int m_area;
	// top-left and bottom-right
	vector<pair<Position, Position>> m_islands;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, puz_island_info> m_pos2islandinfo;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != PUZ_SPACE){
				int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
				m_pos2islandinfo[{r, c}].m_area = n;
			}
		}
	}

	for(auto& kv : m_pos2islandinfo){
		const auto& pn = kv.first;
		auto& info = kv.second;
		int island_area = info.m_area;
		auto& islands = info.m_islands;

		for(int i = 1; i <= m_sidelen; ++i){
			int j = island_area / i;
			if(i * j != island_area || j > m_sidelen) continue;
			Position island_sz(i - 1, j - 1);
			auto p2 = pn - island_sz;
			for(int r = p2.first; r <= pn.first; ++r)
				for(int c = p2.second; c <= pn.second; ++c){
					Position tl(r, c), br = tl + island_sz;
					if(tl.first >= 0 && tl.second >= 0 &&
						br.first < m_sidelen && br.second < m_sidelen &&
						boost::algorithm::none_of(m_pos2islandinfo, [&](
						const pair<const Position, puz_island_info>& kv){
						auto& p = kv.first;
						if(p == pn)
							return false;
						for(int k = 0; k < 4; ++k){
							auto p2 = p + offset[k * 2];
							if(p2.first >= tl.first && p2.second >= tl.second &&
								p2.first <= br.first && p2.second <= br.second)
								return true;
						}
						return false;
					}))
						islands.emplace_back(tl, br);
				}
		}
	}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size() + m_2by2waters.size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<int>> m_matches;
	vector<set<Position>> m_2by2waters;
	vector<pair<Position, Position>> m_islands;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int r = 0; r < g.m_sidelen - 1; ++r)
		for(int c = 0; c < g.m_sidelen - 1; ++c)
			m_2by2waters.push_back({{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}});

	for(auto& kv : g.m_pos2islandinfo){
		auto& island_ids = m_matches[kv.first];
		island_ids.resize(kv.second.m_islands.size());
		boost::iota(island_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& p = kv.first;
		auto& island_ids = kv.second;

		auto& islands = m_game->m_pos2islandinfo.at(p).m_islands;
		boost::remove_erase_if(island_ids, [&](int id){
			auto& island = islands[id];
			for(int r = island.first.first; r <= island.second.first; ++r)
				for(int c = island.first.second; c <= island.second.second; ++c)
					if(this->cells({r, c}) != PUZ_SPACE)
						return true;
			return false;
		});

		if(!init)
			switch(island_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, island_ids.front()) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
	auto& island = m_game->m_pos2islandinfo.at(p).m_islands[n];

	int cnt = m_2by2waters.size();
	auto &tl = island.first, &br = island.second;
	for(int r = tl.first; r <= br.first; ++r)
		for(int c = tl.second; c <= br.second; ++c){
			Position p2(r, c);
			cells(p2) = PUZ_ISLAND;
			boost::remove_erase_if(m_2by2waters, [&](const set<Position>& rng){
				return rng.count(p2) != 0;
			});
		}

	auto f = [&](const Position& p2){
		if(is_valid(p2))
			cells(p2) = PUZ_WATER;
	};
	for(int r = tl.first; r <= br.first; ++r)
		f({r, tl.second - 1}), f({r, br.second + 1});
	for(int c = tl.second; c <= br.second; ++c)
		f({tl.first - 1, c}), f({br.first + 1, c});

	m_distance += cnt - m_2by2waters.size() + 1;
	m_matches.erase(p);
	m_islands.push_back(island);

	if(!is_goal_state())
		return true;
	boost::replace(*this, PUZ_SPACE, PUZ_WATER);
	return boost::algorithm::all_of(m_islands, [&](const pair<Position, Position>& island){
		auto tl = island.first + Position(-1, -1);
		auto br = island.second + Position(1, 1);
		vector<Position> rng{tl, {tl.first, br.second}, {br.first, tl.second}, br};
		return boost::algorithm::any_of(rng, [&](const Position& p2){
			return is_valid(p2) && this->cells(p2) == PUZ_ISLAND;
		});
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
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto it = m_game->m_pos2islandinfo.find(p);
			if(it == m_game->m_pos2islandinfo.end())
				out << cells({r, c}) << ' ';
			else
				out << format("%-2d") % it->second.m_area;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Archipelago()
{
	using namespace puzzles::Archipelago;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Archipelago.xml", "Puzzles\\Archipelago.txt", solution_format::GOAL_STATE_ONLY);
}