#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 11/Wall Sentinels

	Summary
	It's midnight and all is well!

	Description
	1. On the board there is a single continuous castle wall, which you
	   must discover.
	2. The numbers on the board represent Sentinels (in a similar way to
	   'Blocked View'). The Sentinels can be placed on the Wall or Land.
	3. The number tells you how many tiles that Sentinel can control (see)
	   from there vertically and horizontally - of his type of tile.
	4. That means the number of a Land Sentinel indicates how many Land tiles
	   are visible from there, up to Wall tiles or the grid border.
	5. That works the opposite way for Wall Sentinels: they control all the
	   Wall tiles up to Land tiles or the grid border.
	6. The number includes the tile itself, where the Sentinel is located
	   (again, like 'Blocked View').
*/

namespace puzzles{ namespace WallSentinels{

#define PUZ_SPACE			' '
#define PUZ_LAND			'.'
#define PUZ_LAND_S			's'
#define PUZ_WALL			'W'
#define PUZ_WALL_S			'S'
#define PUZ_BOUNDARY		'+'

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
	map<Position, int> m_pos2num;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			auto s = str.substr(c * 2 - 2, 2);
			if(s == "  ")
				m_start.push_back(PUZ_SPACE);
			else{
				m_start.push_back(s[0] == PUZ_LAND ? PUZ_LAND_S : PUZ_WALL_S);
				m_pos2num[{r, c}] = s[1] - '0';
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const {
		return make_pair(m_cells, m_matches) < make_pair(x.m_cells, x.m_matches); 
	}
	bool make_move_sentinel(const Position& p, const vector<int>& perm);
	bool make_move_sentinel2(const Position& p, const vector<int>& perm);
	bool make_move_space(const Position& p, char ch);
	int find_matches(bool init);
	bool is_continuous() const;
	bool is_valid_square(const Position& p) const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
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
: m_cells(g.m_start), m_game(&g)
{
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
		bool is_wall = cells(p) == PUZ_WALL_S;

		int sum = m_game->m_pos2num.at(p) - 1;
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p + os; n <= sum; p2 += os){
				char ch = cells(p2);
				if(ch == PUZ_SPACE)
					nums.push_back(n++);
				else if(!is_wall && (ch == PUZ_LAND || ch == PUZ_LAND_S) ||
					is_wall && (ch == PUZ_WALL || ch == PUZ_WALL_S))
					++n;
				else{
					nums.push_back(n);
					break;
				}
			}
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
				return make_move_sentinel2(p, perms.front()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& a, const Position& p_start);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>* m_area;
};

puz_state2::puz_state2(const set<Position>& a, const Position& p_start)
: m_area(&a)
{
	make_move(p_start);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_area->count(p2) != 0){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::is_continuous() const
{
	set<Position> a;
	for(int r = 1; r < sidelen(); ++r)
		for(int c = 1; c < sidelen(); ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_SPACE || ch == PUZ_WALL || ch == PUZ_WALL_S)
				a.insert(p);
		}
	
	Position p_start = *boost::find_if(a, [&](const Position& p){
		return cells(p) != PUZ_SPACE;
	});

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves({a, p_start}, smoves);
	for(auto& p : smoves)
		a.erase(p);

	return boost::algorithm::all_of(a, [&](const Position& p){
		return cells(p) == PUZ_SPACE;
	});
}

bool puz_state::is_valid_square(const Position& p) const
{
	auto f = [&](const vector<Position>& rng){
		return boost::algorithm::all_of(rng, [&](const Position& p){
			char ch = this->cells(p);
			return ch == PUZ_WALL || ch == PUZ_WALL_S;
		});
	};

	for(int dr = -1; dr <= 0; ++dr)
		for(int dc = -1; dc <= 0; ++dc){
			Position p2(p.first + dr, p.second + dc);
			if(f({p2, p2 + Position{0, 1}, p2 + Position{1, 0}, p2 + Position(1, 1)}))
				return false;
		}
	return true;
}

bool puz_state::make_move_sentinel2(const Position& p, const vector<int>& perm)
{
	bool is_wall = cells(p) == PUZ_WALL_S;
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = perm[i];
		auto p2 = p + os;
		for(int j = 0; j < n; ++j){
			char& ch = cells(p2);
			if(ch == PUZ_SPACE){
				ch = is_wall ? PUZ_WALL : PUZ_LAND;
				if(ch == PUZ_WALL && !is_valid_square(p2))
					return false;
				++m_distance;
			}
			p2 += os;
		}
		char& ch = cells(p2);
		if(ch == PUZ_SPACE){
			ch = is_wall ? PUZ_LAND : PUZ_WALL;
			if(ch == PUZ_WALL && !is_valid_square(p2))
				return false;
		}
	}

	m_matches.erase(p);
	return is_continuous();
}

bool puz_state::make_move_sentinel(const Position& p, const vector<int>& perm)
{
	m_distance = 0;
	if(!make_move_sentinel2(p, perm))
		return false;
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

bool puz_state::make_move_space(const Position& p, char ch)
{
	cells(p) = ch;
	if(ch == PUZ_WALL && !is_valid_square(p))
		return false;
	m_distance = 1;
	return is_continuous();
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_matches.empty()){
		auto& kv = *boost::min_element(m_matches, [](
			const pair<const Position, vector<vector<int>>>& kv1,
			const pair<const Position, vector<vector<int>>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});

		for(const auto& perm : kv.second){
			children.push_back(*this);
			if(!children.back().make_move_sentinel(kv.first, perm))
				children.pop_back();
		}
	}
	else{
		int i = m_cells.find(PUZ_SPACE);
		for(char ch : {PUZ_LAND, PUZ_WALL}){
			children.push_back(*this);
			if(!children.back().make_move_space({i / sidelen(), i % sidelen()}, ch))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_LAND_S || ch == PUZ_WALL_S)
				out  << (ch == PUZ_LAND_S ? PUZ_LAND : PUZ_WALL)
				<< m_game->m_pos2num.at(p) << ' ';
			else
				out << ch << "  ";
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_WallSentinels()
{
	using namespace puzzles::WallSentinels;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\WallSentinels.xml", "Puzzles\\WallSentinels.txt", solution_format::GOAL_STATE_ONLY);
}