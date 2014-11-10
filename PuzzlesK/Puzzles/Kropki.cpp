#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/Kropki

	Summary
	Fill the rows and columns with numbers, respecting the relations

	Description
	1. The Goal is to enter numbers 1 to board size once in every row and
	   column.
	2. A Dot between two tiles give you hints about the two numbers:
	3. Black Dot - one number is twice the other.
	4. White Dot - the numbers are consecutive.
	5. Where the numbers are 1 and 2, there can be either a Black Dot(2 is
	   1*2) or a White Dot(1 and 2 are consecutive).
	6. Please note that when two numbers are either consecutive or doubles,
	   there MUST be a Dot between them!

	Variant
	7. In later 9*9 levels you will also have bordered and coloured areas,
	   which must also contain all the numbers 1 to 9.
*/

namespace puzzles{ namespace Kropki{

#define PUZ_SPACE		' '
#define PUZ_BLACK		'B'
#define PUZ_WHITE		'W'
#define PUZ_NOT_BH		'.'
	
const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const Position offset2[] = {
	{0, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, 0},		// w
};

struct puz_game;
struct puz_area
{
	vector<Position> m_rng_nums, m_rng_dots;
	string m_dots;
	vector<pair<int, int>> m_pos_id_pairs;
	vector<int> m_perm_ids;
	void add_pos(const Position& p){
		bool is_num_row = p.first % 2 == 0;
		bool is_num_col = p.second % 2 == 0;
		if(is_num_row && is_num_col)
			m_rng_nums.push_back(p);
		else if(is_num_row || is_num_col)
			m_rng_dots.push_back(p);
	}
	void prepare(const puz_game* g);
	bool add_perm(const vector<int>& perm, puz_game* g);
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	bool m_bordered;
	string m_start;
	vector<puz_area> m_areas;
	vector<string> m_perms;
	set<Position> m_horz_walls, m_vert_walls;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
	puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
		: m_horz_walls(horz_walls), m_vert_walls(vert_walls) { make_move(p_start); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position> &m_horz_walls, &m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(int i = 0; i < 4; ++i){
		auto p = *this + offset[i * 2];
		auto p_wall = *this + offset2[i];
		auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
		if(walls.count(p_wall) == 0){
			children.push_back(*this);
			children.back().make_move(p);
		}
	}
}

void puz_area::prepare(const puz_game* g){
	boost::sort(m_rng_nums);
	boost::sort(m_rng_dots);
	for(const auto& p : m_rng_dots){
		m_dots.push_back(g->cells(p));
		auto f = [&](int n1, int n2){
			auto g = [&](const Position& p2){
				return boost::range::find(m_rng_nums, p2) - m_rng_nums.begin();
			};
			m_pos_id_pairs.emplace_back(g(p + offset[n1]), g(p + offset[n2]));
		};
		p.first % 2 == 1 ? f(0, 2) : f(3, 1);
	}
}

bool puz_area::add_perm(const vector<int>& perm, puz_game* g)
{
	string dots;
	int i01 = -1;
	for(int i = 0; i < perm.size() - 1; ++i){
		int n1 = perm[i], n2 = perm[i + 1];
		if(n1 > n2) ::swap(n1, n2);
		dots.push_back(n2 - n1 == 1 ? PUZ_WHITE :
			n2 == n1 * 2 ? PUZ_BLACK : PUZ_NOT_BH);
		if(n1 == 1 && n2 == 2)
			i01 = i;
	}
	bool b = m_dots == dots || i01 >= 0 &&
		(dots[i01] = PUZ_BLACK, m_dots == dots);
	if(b)
		m_perm_ids.push_back(g->m_perms.size());
	return b;
}

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_bordered(attrs.get<int>("Bordered", 0) == 1)
	, m_sidelen(strs[0].size() * 2 - 1)
	, m_areas(m_sidelen * 2)
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		bool is_horz = r % 2 == 0;
		for(int c = 0; c < m_sidelen / 2; ++c){
			if(is_horz)
				m_start.push_back(PUZ_SPACE);
			char ch = str[c];
			m_start.push_back(ch == PUZ_SPACE ? PUZ_NOT_BH : ch);
			if(!is_horz)
				m_start.push_back(PUZ_SPACE);
		}
		m_start.push_back(str[m_sidelen / 2]);
	}

	for(int r = 0; r < m_sidelen; r += 2)
		for(int c = 0; c < m_sidelen; c += 2){
			Position p(r, c);
			m_areas[r].add_pos(p);
			m_areas[m_sidelen + c].add_pos(p);
		}

	if(m_bordered){
		set<Position> rng;
		for(int r = 0;; r += 2){
			// horz-walls
			auto& str_h = strs[r];
			for(int c = 0; c < m_sidelen + 2; ++c)
				if(str_h[c] == '-')
					m_horz_walls.insert({r, c});
			if(r == m_sidelen + 2) break;
			auto& str_v = strs[r + 1];
			for(int c = 0;; ++c){
				Position p(r, c);
				// vert-walls
				if(str_v[c] == '|')
					m_vert_walls.insert(p);
				if(c == m_sidelen + 2) break;
				if(str_v[c] == PUZ_SPACE)
					rng.insert(p);
			}
		}
		while(!rng.empty()){
			list<puz_state2> smoves;
			puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
			m_areas.emplace_back();
			for(auto& p : smoves){
				m_areas.back().add_pos(p);
				rng.erase(p);
			}
		}
	}

	for(auto& a : m_areas)
		a.prepare(this);

	vector<int> perm(m_sidelen / 2 + 1);
	boost::iota(perm, 1);
	do
		if(boost::algorithm::any_of(m_areas, [&](puz_area& a){
			return a.add_perm(perm, this);
		})){
			string s;
			for(int i : perm)
				s.push_back(i + '0');
			m_perms.push_back(s);
		}
	while(boost::next_permutation(perm));
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(int i, int j);
	void make_move2(int i, int j);
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
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	vector<int> perm_ids(g.m_perms.size());
	boost::iota(perm_ids, 0);

	for(int i = 0; i < sidelen(); i += 2)
		m_matches[i] = m_matches[sidelen() + i] = perm_ids;

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	auto& perms = m_game->m_perms;
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& p : m_game->m_areas[area_id].m_rng_nums)
			chars.push_back(cells(p));

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE && ch2 != PUZ_BLACK && ch2 != PUZ_WHITE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(area_id, perm_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& range = m_game->m_areas[i].m_rng_nums;
	auto& perm = m_game->m_perms[j];

	for(int k = 0; k < perm.size(); ++k)
		cells(range[k]) = perm[k];

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<int, vector<int>>& kv1, 
		const pair<int, vector<int>>& kv2){
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
	for(int r = 0; r < sidelen(); r += 2){
		for(int c = 0; c < sidelen(); c += 2)
			out << cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_Kropki()
{
	using namespace puzzles::Kropki;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Kropki.xml", "Puzzles\\Kropki.txt", solution_format::GOAL_STATE_ONLY);
}