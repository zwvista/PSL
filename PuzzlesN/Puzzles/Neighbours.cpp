#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 8/Neighbours

	Summary
	Neighbours, yes, but not equally sociable

	Description
	1. The board represents a piece of land bought by a bunch of people. They
	   decided to split the land in equal parts.
	2. However some people are more social and some are less, so each owner
	   wants an exact number of neighbours around him.
	3. Each number on the board represents an owner house and the number of
	   neighbours he desires.
	4. Devide the land so that each one has an equal number of squares and
	   the requested number of neighbours.
	5. Later on, there will be Question Marks, which represents an owner for
	   which you don't know the neighbours preference.
*/

namespace puzzles{ namespace Neighbours{

#define PUZ_SPACE		' '
#define PUZ_BOUNDARY	'B'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_area_info
{
	puz_area_info(const Position& p, int cnt)
	: m_pHouse(p), m_neighbour_count(cnt) {}
	Position m_pHouse;
	int m_neighbour_count;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<char, puz_area_info> m_id2info;
	int m_total_neighbour_count = 0;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	for(int r = 0, n = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 2; ++c){
			char ch = str[c];
			if(ch == PUZ_SPACE) continue;
			char id = n++ + 'a';
			int cnt = ch - '0';
			m_id2info.emplace(id, puz_area_info({r + 1, c + 1}, cnt));
			m_total_neighbour_count += cnt;
		}
	}
}

struct puz_area
{
	set<Position> m_inner, m_outer;
	set<char> m_neighbours;
	bool m_ready = false;

	void add_neighbour(char id, int cnt){
		m_neighbours.insert(id);
		m_ready = m_neighbours.size() == cnt;
	}

	bool operator<(const puz_area& x) const {
		return make_pair(m_ready, m_outer.size()) <
			make_pair(x.m_ready, x.m_outer.size());
	}
};

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(char id, const Position& p);
	bool make_move2(char id, const Position& p);
	int adjust_area(bool init);

	//solve_puzzle interface
	bool is_goal_state() const { return get_heuristic() == 0; }
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_game->m_total_neighbour_count - boost::accumulate(m_id2area, 0, [this](
			int acc, const pair<char, puz_area>& kv){
			return acc + kv.second.m_neighbours.size();
		});
	}
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	unsigned int m_distance = 0;
	map<char, puz_area> m_id2area;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

	for(auto& kv : g.m_id2info)
		make_move2(kv.first, kv.second.m_pHouse);

	adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_id2area){
		auto& area = kv.second;
		auto& outer = area.m_outer;

		if(area.m_ready) continue;

		outer.clear();
		for(auto& p : area.m_inner)
			for(auto& os : offset){
				auto p2 = p + os;
				char ch = cells(p2);
				if(ch == PUZ_SPACE)
					outer.insert(p2);
			}

		if(!init)
			switch(outer.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(kv.first, *outer.begin()) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(char id, const Position& p)
{
	auto& area = m_id2area[id];
	area.m_inner.insert(p);
	cells(p) = id;
	int cnt = m_game->m_id2info.at(id).m_neighbour_count;

	for(auto& os : offset){
		auto p2 = p + os;
		char ch = cells(p2);
		if(ch == PUZ_SPACE || ch == PUZ_BOUNDARY ||
			ch == id || area.m_neighbours.count(ch) != 0)
			continue;

		auto& area2 = m_id2area.at(ch);
		int cnt2 = m_game->m_id2info.at(ch).m_neighbour_count;
		if(area.m_ready || area2.m_ready)
			return false;

		area.add_neighbour(ch, cnt);
		area2.add_neighbour(id, cnt2);
		m_distance += 2;
	}
	return true;
}

bool puz_state::make_move(char id, const Position& p)
{
	m_distance = 0;
	if(!make_move2(id, p))
		return false;
	for(;;)
		switch(adjust_area(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_id2area, [](
		const pair<char, puz_area>& kv1,
		const pair<char, puz_area>& kv2){
		return kv1.second < kv2.second;
	});
	for(const auto& p : kv.second.m_outer){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c)
			out << format("%-2c") % cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_Neighbours()
{
	using namespace puzzles::Neighbours;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Neighbours.xml", "Puzzles\\Neighbours.txt", solution_format::GOAL_STATE_ONLY);
}