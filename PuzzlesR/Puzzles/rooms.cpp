#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Rooms

	Summary
	Close the doors between Rooms

	Description
	1. The view of the board is a castle with every tile identifying a Room.
	   Between Rooms there are doors that can be open or closed. At the start
	   of the game all doors are open.
	2. Each number inside a Room tells you how many other Rooms you see from
	   there, in a straight line horizontally or vertically when the appropriate
	   doors are closed.
	3. At the end of the solution, each Room must be reachable from the others.
	   That means no single Room or group of Rooms can be divided by the others.
	4. In harder levels some tiles won't tell you how many Rooms are visible
	   at all.
*/

namespace puzzles{ namespace rooms{

#define PUZ_SPACE			' '
#define PUZ_CONNECTED		'C'
#define PUZ_DOOR_UNKNOWN	'0'
#define PUZ_DOOR_OPEN		'1'
#define PUZ_DOOR_CLOSED		'2'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const Position door_offset[] = {
	{0, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, 0},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	string m_cells;

	char cell(const Position& p) const { return m_cells.at(p.first * m_sidelen + p.second); }
	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	m_cells = boost::accumulate(strs, string());
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_pos2num[Position(r, c)] = ch - '0';
		}
	}
}

struct puz_state : map<Position, vector<vector<int>>>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char get_door_status(const Position& p, int i) const {
		auto& doors = i % 2 == 0 ? m_horz_doors : m_vert_doors;
		return doors.at(p + door_offset[i]);
	}
	void set_door_status(const Position& p, int i, char status){
		auto& doors = i % 2 == 0 ? m_horz_doors : m_vert_doors;
		doors.at(p + door_offset[i]) = status;
	}
	bool make_move(const Position& p, const vector<int>& comb);
	bool make_move2(const Position& p, const vector<int>& comb);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return size();}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	unsigned int m_distance = 0;
	map<Position, char> m_horz_doors, m_vert_doors;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
	for(int r = 0; r <= sidelen(); ++r)
		for(int c = 0; c <= sidelen(); ++c){
			if(r < sidelen())
				m_vert_doors[Position(r, c)] = c == 0 || c == sidelen() ?
					PUZ_DOOR_CLOSED : PUZ_DOOR_UNKNOWN;
			if(c < sidelen())
				m_horz_doors[Position(r, c)] = r == 0 || r == sidelen() ?
					PUZ_DOOR_CLOSED : PUZ_DOOR_UNKNOWN;
		}

	for(const auto& kv : g.m_pos2num)
		(*this)[kv.first];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : *this){
		const auto& p = kv.first;
		auto& combs = kv.second;
		combs.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p; n <= sum; p2 += os)
				switch(get_door_status(p2, i)){
				case PUZ_DOOR_UNKNOWN:
					nums.push_back(n++);
					break;
				case PUZ_DOOR_OPEN:
					++n;
					break;
				case PUZ_DOOR_CLOSED:
					nums.push_back(n);
					goto blocked;
				}
blocked:	;
		}

		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if(n0 + n1 + n2 + n3 == sum)
							combs.push_back({n0, n1, n2, n3});

		if(!init)
			switch(combs.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, combs.front()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : string
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_game->m_sidelen; }
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	void make_move(int i){ (*this)[i] = PUZ_CONNECTED; }
	void gen_children(list<puz_state2>& children) const;

	const puz_game* m_game;
	const puz_state& m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_game(s.m_game)
, m_state(s)
{
	append(sidelen() * sidelen(), PUZ_SPACE);
	make_move(0);
}

void puz_state2::gen_children(list<puz_state2> &children) const
{
	for(int i = 0; i < length(); ++i){
		if(at(i) == PUZ_CONNECTED) continue;
		Position p(i / sidelen(), i % sidelen());
		for(int j = 0; j < 4; ++j)
			if(m_state.get_door_status(p, j) != PUZ_DOOR_CLOSED &&
				cell(p + offset[j]) == PUZ_CONNECTED){
				children.push_back(*this);
				children.back().make_move(i);
				return;
			}
	}
}

bool puz_state::make_move2(const Position& p, const vector<int>& comb)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = comb[i];
		auto p2 = p;
		for(int j = 0; j < n; ++j){
			set_door_status(p2, i, PUZ_DOOR_OPEN);
			p2 += os;
		}
		set_door_status(p2, i, PUZ_DOOR_CLOSED);
	}

	++m_distance;
	erase(p);

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.back().find(PUZ_SPACE) == -1;
}

bool puz_state::make_move(const Position& p, const vector<int>& comb)
{
	m_distance = 0;
	if(!make_move2(p, comb))
		return false;
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(*this, [](
		const pair<const Position, vector<vector<int>>>& kv1,
		const pair<const Position, vector<vector<int>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(const auto& comb : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, comb))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-doors
		for(int c = 0; c < sidelen(); ++c)
			out << (m_horz_doors.at(Position(r, c)) == PUZ_DOOR_CLOSED ? " -" : "  ");
		out << endl;
		if(r == sidelen()) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-doors
			out << (m_vert_doors.at(p) == PUZ_DOOR_CLOSED ? '|' : ' ');
			if(c == sidelen()) break;
			out << m_game->cell(p);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_rooms()
{
	using namespace puzzles::rooms;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\rooms.xml", "Puzzles\\rooms.txt", solution_format::GOAL_STATE_ONLY);
}