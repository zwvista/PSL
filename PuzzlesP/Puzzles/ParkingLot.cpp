#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 11/ParkingLot

	Summary
	BEEEEP BEEEEEEP !!!

	Description
	1. The board represents a parking lot seen from above.
	2. Each number identifies a ca and all cars are identified by a number,
	   there are no hidden cars.
	3. Cars can be regular sports cars (2*1 tiles) or limousines (3*1 tiles)
	   and can be oriented horizontally or vertically.
	4. The number in itself specifies how far the car can move forward or
	   backward, in tiles.
	5. For example, a car that has one tile free in front and one tile free
	   in the back, would be marked with a '2'.
	6. Find all the cars !!
*/

namespace puzzles{ namespace ParkingLot{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'

struct puz_car_kind
{
	string m_str;
	Position m_offset_car;
	Position m_offset_move1, m_offset_move2;
};

const vector<puz_car_kind> car_kinds = {
	{"h-", {0, 1}, {0, -1}, {0, 1}},	// horizontal sports car
	{"H--",{0, 2}, {0, -1}, {0, 1}},	// horizontal limousine
	{"v|", {1, 0}, {-1, 0}, {1, 0}},	// vertical sports car
	{"V||",{2, 0}, {-1, 0}, {1, 0}},	// vertical limousine
};

struct puz_car
{
	puz_car(int k, const Position& tl, const Position& br)
	: m_kind(k), m_topleft(tl), m_bottomright(br) {}

	// the kind of the car
	int m_kind;
	// top-left and bottom-right
	Position m_topleft, m_bottomright;
};

struct puz_car_info
{
	// how far the car can move forward or backward
	int m_move_count;
	// all dispositions
	vector<puz_car> m_cars;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, puz_car_info> m_pos2carinfo;

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
			if(ch != PUZ_SPACE)
				m_pos2carinfo[Position(r, c)].m_move_count = ch - '0';
		}
	}

	for(auto& kv : m_pos2carinfo){
		const auto& pn = kv.first;
		auto& info = kv.second;
		auto& cars = info.m_cars;

		for(int i = 0; i < 4; ++i){
			auto& ck = car_kinds[i];
			auto& os = ck.m_offset_car;
			auto p2 = pn - os;
			for(int r = p2.first; r <= pn.first; ++r)
				for(int c = p2.second; c <= pn.second; ++c){
					Position tl(r, c), br = tl + os;
					if(tl.first >= 0 && tl.second >= 0 &&
						br.first < m_sidelen && br.second < m_sidelen &&
						boost::algorithm::none_of(m_pos2carinfo, [&](
						const pair<const Position, puz_car_info>& kv){
						auto& p = kv.first;
						return p != pn &&
							p.first >= tl.first && p.second >= tl.second &&
							p.first <= br.first && p.second <= br.second;
					}))
						cars.emplace_back(i, tl, br);
				}
		}
	}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
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
	map<Position, vector<int>> m_matches;
	map<Position, puz_car> m_pos2car;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_EMPTY), m_game(&g)
{
	for(auto& kv : g.m_pos2carinfo){
		auto& car_ids = m_matches[kv.first];
		car_ids.resize(kv.second.m_cars.size());
		boost::iota(car_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& p = kv.first;
		auto& car_ids = kv.second;

		auto& cars = m_game->m_pos2carinfo.at(p).m_cars;
		boost::remove_erase_if(car_ids, [&](int id){
			auto& car = cars[id];
			for(int r = car.m_topleft.first; r <= car.m_bottomright.first; ++r)
				for(int c = car.m_topleft.second; c <= car.m_bottomright.second; ++c)
					if(cells({r, c}) != PUZ_EMPTY)
						return true;
			return false;
		});

		if(!init)
			switch(car_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, car_ids.front()) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
	{
		auto& info = m_game->m_pos2carinfo.at(p);
		auto& car = info.m_cars[n];
		auto& ck = car_kinds[car.m_kind];

		for(int r = car.m_topleft.first, i = 0; r <= car.m_bottomright.first; ++r)
			for(int c = car.m_topleft.second; c <= car.m_bottomright.second; ++c)
				cells({r, c}) = ck.m_str[i++];

		m_pos2car.emplace(p, car);
		++m_distance;
		m_matches.erase(p);
	}

	for(const auto& kv : m_pos2car){
		auto& info = m_game->m_pos2carinfo.at(kv.first);
		auto& car = kv.second;
		auto& ck = car_kinds[car.m_kind];
		int move_count = 0;
		auto f = [&](Position p, const Position& os){
			for(p += os; is_valid(p) && cells(p) == PUZ_EMPTY; p += os)
				++move_count;
		};
		f(car.m_topleft, ck.m_offset_move1);
		f(car.m_bottomright, ck.m_offset_move2);
		if(move_count < info.m_move_count ||
			m_matches.empty() && move_count > info.m_move_count)
			return false;
	}
	return true;
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move2(p, n))
		return false;
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
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << format("%-2s") % cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_ParkingLot()
{
	using namespace puzzles::ParkingLot;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\ParkingLot.xml", "Puzzles\\ParkingLot.txt", solution_format::GOAL_STATE_ONLY);
}