#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Ripple Effect

	Summary
	Fill the Room with the numbers, but take effect of the Ripple Effect

	Description
	1. The goal is to fill the Rooms you see on the board, with numbers 1 to room size.
	2. While doing this, you must consider the Ripple Effect. The same number
	   can only appear on the same row or column at the distance of the number
	   itself.
*/

namespace puzzles{ namespace rippleeffect{

const Position offset[] = {
	{-1, 0},
	{0, 1},
	{1, 0},
	{0, -1},
};

// first: the remaining positions in the room that should be filled
// second: all permutations of the remaining numbers that should be used to fill the room
typedef pair<vector<Position>, vector<vector<int>>> puz_room_info;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_start;
	map<Position, pair<int, int>> m_pos2info;
	map<int, puz_room_info> m_room2info;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2)
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_start[Position(r, c)] = ch - '0';
		}
	}

	vector<vector<Position>> rooms;
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r + m_sidelen];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = str[c] - 'a';
			m_pos2info[p].first = n;
			if(n >= rooms.size())
				rooms.resize(n + 1);
			rooms[n].push_back(p);
		}
	}

	for(int i = 0; i < rooms.size(); ++i){
		const auto& room = rooms[i];
		auto& info = m_room2info[i];
		auto& ps = info.first;
		auto& perms = info.second;

		vector<int> perm;
		perm.resize(room.size());
		boost::iota(perm, 1);

		vector<Position> filled;
		for(const auto& p : room){
			auto it = m_start.find(p);
			if(it != m_start.end()){
				filled.push_back(p);
				boost::range::remove_erase(perm, it->second);
			}
		}
		boost::set_difference(room, filled, back_inserter(ps));
		for(int j = 0; j < ps.size(); ++j)
			m_pos2info[ps[j]].second = j;

		if(info.first.empty())
			m_room2info.erase(i);
		else
			do
				perms.push_back(perm);
			while(boost::next_permutation(perm));
	}
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	int cells(const Position& p) const { return at(p.first * sidelen() + p.second); }
	int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i, const vector<int>& nums);
	void apply_ripple_effect(const Position& p, int n);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, 0);}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<int, puz_room_info> m_room2info;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_sidelen * g.m_sidelen)
, m_game(&g), m_room2info(g.m_room2info)
{
	for(const auto& kv : g.m_start)
		cells(kv.first) = kv.second;

	for(const auto& kv : g.m_start)
		apply_ripple_effect(kv.first, kv.second);
}

void puz_state::apply_ripple_effect(const Position& p, int n)
{
	for(auto& os : offset){
		auto p2 = p;
		for(int k = 0; k < n; ++k){
			p2 += os;
			if(!is_valid(p2)) break;
			if(cells(p2) != 0) continue;
			int i, j;
			tie(i, j) = m_game->m_pos2info.at(p2);
			boost::range::remove_erase_if(m_room2info.at(i).second, [=](const vector<int>& nums){
				return nums[j] == n;
			});
		}
	}
}

bool puz_state::make_move(int i, const vector<int>& nums)
{
	const auto& info = m_room2info.at(i);
	m_distance = nums.size();
	for(int j = 0; j < m_distance; ++j){
		const auto& p = info.first[j];
		int n = nums[j];
		cells(p) = n;
		apply_ripple_effect(p, n);
	}
	m_room2info.erase(i);
	return boost::algorithm::none_of(m_room2info, [](const pair<int, puz_room_info>& kv){
		return kv.second.second.empty();
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_room2info, [](
		const pair<int, puz_room_info>& kv1,
		const pair<int, puz_room_info>& kv2){
		return kv1.second.second.size() < kv2.second.second.size();
	});
	for(const auto& nums : kv.second.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, nums))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cells(Position(r, c)) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_rippleeffect()
{
	using namespace puzzles::rippleeffect;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\rippleeffect.xml", "Puzzles\\rippleeffect.txt", solution_format::GOAL_STATE_ONLY);
}