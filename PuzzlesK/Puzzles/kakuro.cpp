#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Kakuro

	Summary
	Fill the board with numbers 1 to 9 according to the sums

	Description
	1. Your goal is to write a number in every blank tile (without a diagonal
	   line).
	2. The number on the top of a column or at the left of a row, gives you
	   the sum of the numbers in that column or row.
	3. You can write numbers 1 to 9 in the tiles, however no same number should
	   appear in a consecutive row or column.
	4. Tiles which only have a diagonal line aren't used in the game.
*/

namespace puzzles{ namespace kakuro{

#define PUZ_SPACE		' '

using puz_area = pair<vector<Position>, int>;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, puz_area> m_pos2area_rows, m_pos2area_cols;
	map<Position, int> m_blanks;
	map<pair<int, int>, vector<vector<int>>> m_sum2disps;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen * 4; c += 4){
			Position p(r, c / 4);
			const string s1 = str.substr(c, 2);
			const string s2 = str.substr(c + 2, 2);
			if(s1 == "  ")
				m_blanks[p] = 0;
			else{
				int n1 = atoi(s1.c_str());
				if(n1 != 0)
					m_pos2area_cols[p].second = n1;
				int n2 = atoi(s2.c_str());
				if(n2 != 0)
					m_pos2area_rows[p].second = n2;
			}
		}
	}

	auto f = [this](map<Position, puz_area>& m, const Position& os){
		for(auto& kv : m){
			const auto& p = kv.first;
			auto& ps = kv.second.first;
			int cnt = 0;
			for(auto p2 = p + os; m_blanks.count(p2) != 0; p2 += os)
				++cnt, ps.push_back(p2);
			m_sum2disps[make_pair(kv.second.second, cnt)];
		}
	};
	f(m_pos2area_rows, {0, 1});		// e
	f(m_pos2area_cols, {1, 0});		// s

	for(auto& kv : m_sum2disps){
		int sum = kv.first.first;
		int cnt = kv.first.second;
		auto& disps = kv.second;

		vector<int> disp(cnt, 1);
		for(int i = 0; i < cnt;){
			if(boost::accumulate(disp, 0) == sum &&
				boost::range::adjacent_find(disp) == disp.end())
				disps.push_back(disp);
			for(i = 0; i < cnt && ++disp[i] == 10; ++i)
				disp[i] = 1;
		}
	};
}

using puz_key = pair<Position, bool>;

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const puz_key& kv, int j);
	void make_move2(const puz_key& kv, int j);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, int> m_cells;
	map<puz_key, vector<int>> m_matches;
	int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_blanks), m_game(&g)
{
	for(const auto& kv : g.m_pos2area_cols)
		m_matches[make_pair(kv.first, true)];
	for(const auto& kv : g.m_pos2area_rows)
		m_matches[make_pair(kv.first, false)];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		vector<int> nums;
		auto& area = (kv.first.second ? m_game->m_pos2area_cols : m_game->m_pos2area_rows).at(kv.first.first);
		for(auto& p : area.first)
			nums.push_back(m_cells.at(p));

		kv.second.clear();
		auto& disps = m_game->m_sum2disps.at(make_pair(area.second, area.first.size()));
		for(int i = 0; i < disps.size(); ++i)
			if(boost::equal(nums, disps[i], [](int n1, int n2){
				return n1 == 0 || n1 == n2;
			}))
				kv.second.push_back(i);

		if(!init)
			switch(kv.second.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(kv.first, kv.second.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(const puz_key& kv, int j)
{
	auto& area = (kv.second ? m_game->m_pos2area_cols : m_game->m_pos2area_rows).at(kv.first);
	auto& disp = m_game->m_sum2disps.at(make_pair(area.second, area.first.size()))[j];

	for(int k = 0; k < disp.size(); ++k)
		m_cells.at(area.first[k]) = disp[k];

	++m_distance;
	m_matches.erase(kv);
}

bool puz_state::make_move(const puz_key& kv, int j)
{
	m_distance = 0;
	make_move2(kv, j);
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
		const pair<const puz_key, vector<int>>& kv1,
		const pair<const puz_key, vector<int>>& kv2){
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
			Position p(r, c);
			out << (m_game->m_blanks.count(p) != 0 ? char(m_cells.at(p) + '0') : '.') << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_kakuro()
{
	using namespace puzzles::kakuro;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\kakuro.xml", "Puzzles\\kakuro.txt", solution_format::GOAL_STATE_ONLY);
}