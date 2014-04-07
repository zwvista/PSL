#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 2/Tatami

	Summary
	1,2,3... 1,2,3... Fill the mats

	Description
	1. Each rectangle represents a mat(Tatami) which is of the same size.
	   You must fill each Tatami with a number ranging from 1 to size.
	2. Each number can appear only once in each Tatami.
	3. In one row or column, each number must appear the same number of times.
	4. You can't have two identical numbers touching horizontally or vertically.
*/

namespace puzzles{ namespace tatami{

#define PUZ_HORZ		'H'
#define PUZ_VERT		'V'
#define PUZ_SPACE		' '

const Position offset[] = {
	{0, 1},
	{1, 0},
	{0, -1},
	{-1, 0},
};

struct puz_numbers : set<char>
{
	puz_numbers() {}
	puz_numbers(int num){
		for(int i = 0; i < num; ++i)
			insert(i + '1');
	}
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_size_of_tatami;
	int m_num_tatamis;
	vector<vector<Position>> m_area_pos;
	puz_numbers m_numbers;
	map<Position, char> m_start;
	map<Position, int> m_pos2tatami;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size() / 2)
	, m_size_of_tatami(attrs.get<int>("TatamiSize"))
	, m_num_tatamis(m_sidelen * m_sidelen / m_size_of_tatami)
	, m_area_pos(m_num_tatamis + m_sidelen + m_sidelen)
	, m_numbers(m_size_of_tatami)
{
	for(int r = 0, n = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c)
			switch(char ch = str[c]){
			case PUZ_SPACE:
				break;
			default:
				m_start[{r, c}] = ch;
				break;
			}
	}
	for(int r = 0, n = 0; r < m_sidelen; ++r){
		const string& str = strs[r + m_sidelen];
		for(int c = 0; c < m_sidelen; ++c)
			switch(char ch = str[c]){
			case PUZ_HORZ:
			case PUZ_VERT:
				for(int d = 0; d < m_size_of_tatami; ++d){
					auto p = 
						ch == PUZ_HORZ ? Position(r, c + d)
						: Position(r + d, c);
					m_pos2tatami[p] = n;
					m_area_pos[n].push_back(p);
					m_area_pos[m_num_tatamis + p.first].push_back(p);
					m_area_pos[m_num_tatamis + m_sidelen + p.second].push_back(p);
				}
				++n;
				break;
			}
	}
}

// second.key : all char numbers used to fill a position
// second.value : the number of remaining times that the key char number can be used in the area
struct puz_area : pair<int, map<char, int>>
{
	puz_area() {}
	puz_area(int index, const puz_numbers& numbers, int num_times_appear)
		: pair<int, map<char, int>>(index, map<char, int>()){
		for(char ch : numbers)
			second.emplace(ch, num_times_appear);
	}
	bool fill_cells(const Position& p, char ch){ return --second.at(ch); }
};

struct puz_group : vector<puz_area>
{
	puz_group() {}
	puz_group(int index, int sz, const puz_numbers& numbers, int num_times_appear){
		for(int i = 0; i < sz; i++)
			emplace_back(index++, numbers, num_times_appear);
	}
};

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, char ch);
	void remove_pair(const Position& p, char ch){
		auto i = m_pos2nums.find(p);
		if(i != m_pos2nums.end())
			i->second.erase(ch);
	}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {return boost::range::count(*this, PUZ_SPACE);}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	puz_group m_grp_tatamis;
	puz_group m_grp_rows;
	puz_group m_grp_cols;
	map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
, m_grp_tatamis(0, g.m_num_tatamis, g.m_numbers, 1)
, m_grp_rows(g.m_num_tatamis, g.m_sidelen, g.m_numbers, g.m_sidelen / g.m_size_of_tatami)
, m_grp_cols(g.m_num_tatamis + g.m_sidelen, g.m_sidelen, g.m_numbers, g.m_sidelen / g.m_size_of_tatami)
{
	for(int r = 0; r < g.m_sidelen; ++r)
		for(int c = 0; c < g.m_sidelen; ++c)
			m_pos2nums[{r, c}] = g.m_numbers;

	for(const auto& kv : g.m_start)
		make_move(kv.first, kv.second);
}

bool puz_state::make_move(const Position& p, char ch)
{
	cells(p) = ch;
	m_pos2nums.erase(p);

	auto areas = {
		&m_grp_tatamis[m_game->m_pos2tatami.at(p)],
		&m_grp_rows[p.first],
		&m_grp_cols[p.second]
	};
	for(puz_area* a : areas)
		if(a->fill_cells(p, ch) == 0)
			for(const auto& p2 : m_game->m_area_pos[a->first])
				remove_pair(p2, ch);

	// no touch
	for(auto& os : offset){
		const auto& p2 = p + os;
		if(is_valid(p2))
			remove_pair(p2, ch);
	}

	return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, puz_numbers>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	const auto& kv = *boost::min_element(m_pos2nums, 
		[](const pair<const Position, puz_numbers>& kv1, const pair<const Position, puz_numbers>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	const auto& p = kv.first;
	for(char ch : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(p, ch))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cells({r, c}) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_tatami()
{
	using namespace puzzles::tatami;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
		("Puzzles\\tatami.xml", "Puzzles\\tatami.txt", solution_format::GOAL_STATE_ONLY);
}