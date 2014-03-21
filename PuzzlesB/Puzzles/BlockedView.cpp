#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/Blocked View

	Summary
	This time it's one Garden and many Towers

	Description
	1. On the Board there are a few sentinels. These sentinels are marked with
	   a number.
	2. The number tells you how many tiles that Sentinel can control (see) from
	   there vertically and horizontally. This includes the tile where he is
	   located.
	3. You must put Towers on the Boards in accordance with these hints, keeping
	   in mind that a Tower blocks the Sentinel View.
	4. The restrictions are that there must be a single continuous Garden, and
	   two Towers can't touch horizontally or vertically.
	5. Towers can't go over numbered squares. But numbered squares don't block
	   Sentinel View.
*/

namespace puzzles{ namespace BlockedView{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_SENTINEL	'S'
#define PUZ_TOWER		'T'
#define PUZ_BOUNDARY	'B'

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
	map<Position, int> m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
{
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 2; ++c){
			auto s = str.substr(c * 2, 2);
			if(s != "  ")
				m_start[{r + 1, c + 1}] = atoi(s.c_str());
		}
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, const vector<int>& perm);
	bool make_move2(const Position& p, const vector<int>& perm);
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
	map<Position, vector<vector<int>>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

	for(const auto& kv : g.m_start)
		cells(kv.first) = PUZ_SENTINEL, m_matches[kv.first];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		perms.clear();

		int sum = m_game->m_start.at(p) - 1;
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			[&]{
				for(auto p2 = p + os; n <= sum; p2 += os)
					switch(cells(p2)){
					case PUZ_SPACE:
						nums.push_back(n++);
						break;
					case PUZ_EMPTY:
					case PUZ_SENTINEL:
						++n;
						break;
					case PUZ_TOWER:
					case PUZ_BOUNDARY:
						nums.push_back(n);
						return;
					}
			}();
		}

		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if(n0 + n1 + n2 + n3 == sum)
							perms.push_back({n0, n1, n2, n3});

		if(!init)
			switch(perms.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perms.front()) ? 1 : 0;
			}
	}
	return 2;
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
	int i = boost::find_if(s.m_cells, [](char ch){
		return ch != PUZ_BOUNDARY && ch != PUZ_TOWER;
	}) - s.m_cells.begin();
	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		char ch = m_state->cells(p2);
		if(ch != PUZ_BOUNDARY && ch != PUZ_TOWER){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move2(const Position& p, const vector<int>& perm)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = perm[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			char& ch = cells(p2);
			if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
			p2 += os;
		}
		char& ch = cells(p2);
		if(ch == PUZ_SPACE){
			for(auto& os2 : offset){
				auto p3 = p2 + os2;
				if(cells(p3) == PUZ_TOWER)
					return false;
			}
			ch = PUZ_TOWER;
		}
	}

	++m_distance;
	m_matches.erase(p);

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.size() == boost::count_if(m_cells, [](char ch){
		return ch != PUZ_BOUNDARY && ch != PUZ_TOWER;
	});
}

bool puz_state::make_move(const Position& p, const vector<int>& perm)
{
	m_distance = 0;
	if(!make_move2(p, perm))
		return false;
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<vector<int>>>& kv1,
		const pair<const Position, vector<vector<int>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(const auto& perm : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, perm))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			switch(char ch = cells({r, c})){
			case PUZ_SENTINEL:
				out << format("%2d") % m_game->m_start.at(p);
				break;
			default:
				out << ' ' << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
				break;
			};
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_BlockedView()
{
	using namespace puzzles::BlockedView;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\BlockedView.xml", "Puzzles\\BlockedView.txt", solution_format::GOAL_STATE_ONLY);
}