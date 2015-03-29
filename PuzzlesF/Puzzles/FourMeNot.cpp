#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 9/Four-Me-Not

	Summary
	It seems we do a lot of gardening in this game! 

	Description
	1. In Four-Me-Not (or Forbidden Four) you need to create a continuous
	   flower bed without putting four flowers in a row.
	2. More exactly, you have to join the existing flowers by adding more of
	   them, creating a single path of flowers touching horizontally or
	   vertically.
	3. At the same time, you can't line up horizontally or vertically more
	   than 3 flowers (thus Forbidden Four).
	4. Some tiles are marked as squares and are just fixed blocks.
*/

namespace puzzles{ namespace FourMeNot{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_FIXED		'F'
#define PUZ_ADDED		'f'
#define PUZ_BLOCK		'B'

bool is_flower(char ch) { return ch == PUZ_FIXED || ch == PUZ_ADDED; }

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
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(m_sidelen, PUZ_BLOCK);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BLOCK);
		for(int c = 1; c < m_sidelen - 1; ++c)
			m_start.push_back(str[c - 1]);
		m_start.push_back(PUZ_BLOCK);
	}
	m_start.append(m_sidelen, PUZ_BLOCK);
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(Position p);
	void check_field();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_num2outer.size() - 1; }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	// key: the index of the flowerbed
	// value: the neighboring tiles of the flowerbed
	map<int, set<Position>> m_num2outer;
	unsigned int m_distance = 0;
};

struct puz_state2 : Position
{
	puz_state2(const set<Position>& rng) : m_rng(&rng) {
		make_move(*rng.begin());
	}

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

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	set<Position> rng;
	for(int r = 1; r < g.m_sidelen - 1; ++r)
		for(int c = 1; c < g.m_sidelen - 1; ++c){
			Position p(r, c);
			if(is_flower(cells(p)))
				rng.insert(p);
		}

	int n = 0;
	while(!rng.empty()){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves(rng, smoves);

		auto& outer = m_num2outer[n++];
		for(auto& p : smoves){
			rng.erase(p);
			for(auto& os : offset){
				auto p2 = p + os;
				if(cells(p2) == PUZ_SPACE)
					outer.insert(p2);
			}
		}
	}

	check_field();
}

void puz_state::check_field()
{
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			if(cells(p) != PUZ_SPACE)
				continue;

			vector<int> counts;
			for(auto& os : offset){
				int n = 0;
				for(auto p2 = p + os; is_flower(cells(p2)); p2 += os)
					++n;
				counts.push_back(n);
			}
			if(counts[0] + counts[2] < 3 && counts[1] + counts[3] < 3)
				continue;
			// if a flower is put in this tile
			// more than 3 flowers will line up horizontally and/or vertically
			// so this tile must be empty.
			cells(p) = PUZ_EMPTY;
			for(auto& kv : m_num2outer)
				kv.second.erase(p);
		}
}

bool puz_state::make_move(Position p)
{
	vector<int> nums;
	for(auto& kv : m_num2outer)
		if(kv.second.count(p) != 0)
			nums.push_back(kv.first);

	m_distance = 0;
	// merge all the flowerbeds adjacent to p
	while(nums.size() > 1){
		int n2 = nums.back(), n1 = nums.rbegin()[1];
		auto &outer2 = m_num2outer.at(n2), &outer1 = m_num2outer.at(n1);
		outer1.insert(outer2.begin(), outer2.end());
		m_num2outer.erase(n2);
		++m_distance;
		nums.pop_back();
	}

	cells(p) = PUZ_ADDED;
	auto& outer = m_num2outer.at(nums[0]);
	outer.erase(p);
	for(auto& os : offset){
		auto p2 = p + os;
		if(cells(p2) == PUZ_SPACE)
			outer.insert(p2);
	}

	return is_goal_state() || (
		check_field(),
		boost::algorithm::all_of(m_num2outer,
			[](const pair<const int, set<Position>>& kv){
			return kv.second.size() > 0;
		})
	);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_num2outer, [](
		const pair<const int, set<Position>>& kv1,
		const pair<const int, set<Position>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(auto& p : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(p))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			char ch = cells({r, c});
			out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_FourMeNot()
{
	using namespace puzzles::FourMeNot;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\FourMeNot.xml", "Puzzles\\FourMeNot.txt", solution_format::GOAL_STATE_ONLY);
}