#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/square100

	Summary
	Exactly one hundred

	Description
	1. You are given a 3*3 or 4*4 square with digits in it.
	2. You have to add digits to some (or all) tiles, in order to produce
	   the sum of 100 for every row and column.
	3. You can add digits before or after the given one.
*/

namespace puzzles{ namespace square100{

#define PUZ_SPACE		' '

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<pair<vector<Position>, vector<int>>> m_areas;
	map<vector<int>, vector<vector<int>>> m_nums2combs;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_areas(m_sidelen * 2)
{
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = str[c] - '0';
			m_areas[r].first.push_back(p);
			m_areas[r].second.push_back(n);
			m_areas[m_sidelen + c].first.push_back(p);
			m_areas[m_sidelen + c].second.push_back(n);
		}
	}

	for(const auto& a : m_areas){
		const auto& nums = a.second;
		auto& combs = m_nums2combs[nums];
		int cnt = nums.size();

		//vector<vector<int>> v(cnt);
		//for(int i = 0; i < cnt; ++i){
		//	int n = nums[i];
		//	for(int j = 0; j < 10; ++j){
		//		v[i].push_back(j * 10 + n);
		//		v[i].push_back(n * 10 + j);
		//	}
		//}

		vector<int> indexes(cnt);
		vector<int> comb(cnt);
		for(int i = 0; i < cnt;){
			for(int j = 0; j < cnt; ++j){
				int index = indexes[j];
				comb[j] = index < 10 ? index * 10 + nums[j]
					: nums[j] * 10 + index - 10;
			}
			if(boost::accumulate(comb, 0) == 100)
				combs.push_back(comb);
			for(i = 0; i < cnt && ++indexes[i] == 20; ++i)
				indexes[i] = 0;
		}
	}
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	int& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i, int j);
	void find_matches();

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
	map<int, vector<int>> m_matches;
	int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_sidelen * g.m_sidelen), m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		m_matches[i], m_matches[sidelen() + i];

	find_matches();
}

void puz_state::find_matches()
{
	vector<int> filled;

	for(auto& kv : m_matches){
		const auto& a = m_game->m_areas.at(kv.first);
		vector<int> nums;
		for(const auto& p : a.first)
			nums.push_back(cell(p));
		if(boost::algorithm::none_of(nums, [](int n){return n == 0;})){
			filled.push_back(kv.first);
			continue;
		}

		const auto& combs = m_game->m_nums2combs.at(a.second);
		kv.second.clear();
		for(int i = 0; i < combs.size(); ++i)
			if(boost::equal(nums, combs.at(i), [](int n1, int n2){
				return n1 == 0 || n1 == n2; }))
				kv.second.push_back(i);
	}

	for(int n : filled)
		m_matches.erase(n);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	const auto& area = m_game->m_areas.at(i);
	const auto& comb = m_game->m_nums2combs.at(area.second).at(j);

	for(int k = 0; k < sidelen(); ++k){
		int& n = cell(area.first[k]);
		if(n == 0)
			n = comb[k], ++m_distance;
	}

	find_matches();
	return boost::algorithm::none_of(m_matches, [](const pair<const int, vector<int>>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(m_matches, [](const pair<const int, vector<int>>& kv1, const pair<const int, vector<int>>& kv2){
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
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c){
			out << format("%3d") % cell(Position(r, c));
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_square100()
{
	using namespace puzzles::square100;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\square100.xml", "Puzzles\\square100.txt", solution_format::GOAL_STATE_ONLY);
}