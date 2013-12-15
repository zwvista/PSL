#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 8/Kakurasu

	Summary
	Cloud Kakuro on a Skyscraper

	Description
	1. On the bottom and right border, you see the value of (respectively)
	   the columns and rows.
	2. On the other borders, on the top and the left, you see the hints about
	   which tile have to be filled on the board. These numbers represent the
	   sum of the values mentioned above.
*/

namespace puzzles{ namespace kakurasu{

#define PUZ_SPACE	-1

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<int> m_start;
	vector<vector<Position>> m_area2range;
	vector<vector<int>> m_disps_rows, m_disps_cols;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
	vector<int> values(m_sidelen * 2);
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			auto s = str.substr(c * 2, 2);
			int n = atoi(s.c_str());
			m_start.push_back(n == 0 ? PUZ_SPACE : n);
			if(r == m_sidelen - 1 || c == m_sidelen - 1){
				if(r == m_sidelen - 1)
					values[m_sidelen + c] = n;
				if(c == m_sidelen - 1)
					values[r] = n;
			}
			else{
				Position p(r, c);
				m_area2range[r].push_back(p);
				m_area2range[m_sidelen + c].push_back(p);
			}
		}
	}

	auto f = [&](vector<vector<int>>& disps, int n){
		int cnt = m_sidelen - 1;
		vector<int> disp(cnt);

		for(int i = 1; i < cnt;){
			disp[0] = 0;
			for(int j = 1; j < cnt; ++j)
				disp[0] += disp[j] == 0 ? 0 : values[n + j];
			
			disps.push_back(disp);
			for(i = 1; i < cnt && ++disp[i] == 2; ++i)
				disp[i] = 0;
		}
	};
	f(m_disps_rows, 0);
	f(m_disps_cols, m_sidelen);
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	int cells(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	int& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
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
	vector<int> m_cells;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	vector<int> disp_id_rows(g.m_disps_rows.size());
	boost::iota(disp_id_rows, 0);
	vector<int> disp_id_cols(g.m_disps_cols.size());
	boost::iota(disp_id_cols, 0);

	for(int i = 1; i < sidelen() - 1; ++i){
		m_matches[i] = disp_id_rows;
		m_matches[sidelen() + i] = disp_id_cols;
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& disp_ids = kv.second;

		vector<int> nums;
		for(auto& p : m_game->m_area2range[area_id])
			nums.push_back(cells(p));

		auto& disps = area_id < sidelen() ? m_game->m_disps_rows : m_game->m_disps_cols;
		boost::remove_erase_if(disp_ids, [&](int id){
			return !boost::equal(nums, disps[id], [](int n1, int n2){
				return n1 == PUZ_SPACE || n1 == n2;
			});
		});

		if(!init)
			switch(disp_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(area_id, disp_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& range = m_game->m_area2range[i];
	auto& disp = (i < sidelen() ? m_game->m_disps_rows : m_game->m_disps_cols)[j];

	for(int k = 0; k < disp.size(); ++k)
		cells(range[k]) = disp[k];

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
	}
	return true;
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
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c){
			int n = cells(Position(r, c));
			if(n == PUZ_SPACE || n == 0)
				out << "   ";
			else
				out << format("%3d") % cells(Position(r, c));
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_kakurasu()
{
	using namespace puzzles::kakurasu;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\kakurasu.xml", "Puzzles\\kakurasu.txt", solution_format::GOAL_STATE_ONLY);
}