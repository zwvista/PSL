#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Calcudoku

	Summary
	Mathematical Sudoku

	Description
	1. Write numbers ranging from 1 to board size respecting the calculation
	   hint.
	2. The tiny numbers and math signs in the corner of an area give you the
	   hint about what's happening inside that area.
	3. For example a '3+' means that the sum of the numbers inside that area
	   equals 3. In that case you would have to write the numbers 1 and 2
	   there.
	4. Another example: '12*' means that the multiplication of the numbers
	   in that area gives 12, so it could be 3 and 4 or even 3, 4 and 1,
	   depending on the area size.
	5. Even where the order of the operands matter (in subtraction and division)
	   they can appear in any order inside the area (ie.e. 2/ could be done
	   with 4 and 2 or 2 and 4.
	6. All the numbers appear just one time in each row and column, but they
	   could be repeated in non-straight areas, like the L-shaped one.
*/

namespace puzzles{ namespace Calcudoku{

#define PUZ_SPACE	0
#define PUZ_ADD		'+'
#define PUZ_SUB		'-'
#define PUZ_MUL		'*'
#define PUZ_DIV		'/'

struct puz_area_info
{
	vector<Position> m_range;
	char m_operator;
	int m_result;
	vector<vector<int>> m_disps;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	vector<puz_area_info> m_area_info;
	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			auto s = str.substr(c * 4, 4);
			int n = s[0] - 'a';

			if(n >= m_area_info.size())
				m_area_info.resize(n + 1);
			auto& info = m_area_info[n];
			info.m_range.push_back(p);
			if(s.substr(1) == "   ") continue;
			info.m_operator = s[3];
			info.m_result = atoi(s.substr(1, 2).c_str());
		}
	}

	auto f = [](const vector<Position>& rng){
		vector<vector<int>> groups;
		map<int, vector<int>> grp_rows, grp_cols;
		for(int i = 0; i < rng.size(); ++i){
			auto& p = rng[i];
			grp_rows[p.first].push_back(i);
			grp_cols[p.second].push_back(i);
		}
		for(const auto& m : {grp_rows, grp_cols})
			for(auto& kv : m)
				if(kv.second.size() > 1)
					groups.push_back(kv.second);
		return groups;
	};

	for(auto& info : m_area_info){
		int cnt = info.m_range.size();

		vector<vector<int>> disps;
		vector<int> disp(cnt, 1);
		for(int i = 0; i < cnt;){
			auto nums = disp;
			boost::sort(nums, greater<>());
			if(accumulate(next(nums.begin()), nums.end(),
				nums.front(), [&](int acc, int v){
				switch(info.m_operator){
				case PUZ_ADD:
					return acc + v;
				case PUZ_SUB:
					return acc - v;
				case PUZ_MUL:
					return acc * v;
				case PUZ_DIV:
					return acc % v == 0 ? acc / v : 0;
				default:
					return 0;
				}
			}) == info.m_result)
				disps.push_back(disp);
			for(i = 0; i < cnt && ++disp[i] == m_sidelen + 1; ++i)
				disp[i] = 1;
		}

		auto groups = f(info.m_range);
		for(const auto& disp : disps)
			if(boost::algorithm::all_of(groups, [&](const vector<int>& grp){
				set<int> nums;
				for(int i : grp)
					nums.insert(disp[i]);
				return nums.size() == grp.size();
			}))
				info.m_disps.push_back(disp);
	}

	boost::sort(m_area_info, [this](const puz_area_info& info1, const puz_area_info& info2){
		return info1.m_disps.size() < info2.m_disps.size();
	});
}

struct puz_state : vector<int>
{
	puz_state() {}
	puz_state(const puz_game& g)
		: vector<int>(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
		, m_game(&g) {}

	int sidelen() const {return m_game->m_sidelen;}
	int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_game->m_area_info.size() - m_area_index; 
	}
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	int m_area_index = 0;
};

bool puz_state::make_move(int i)
{
	auto& info = m_game->m_area_info[m_area_index++];
	auto& area = info.m_range;
	auto& disp = info.m_disps[i];

	for(int k = 0; k < disp.size(); ++k)
		cells(area[k]) = disp[k];

	auto f = [&](Position p, const Position& os){
		set<int> nums;
		for(int cr = 0; cr < sidelen(); ++cr){
			int n = cells(p);
			if(n == PUZ_SPACE)
				return true;
			nums.insert(n);
			p += os;
		}
		return nums.size() == sidelen();
	};

	for(int rc = 0; rc < sidelen(); ++rc)
		if(!f({rc, 0}, {0, 1}) ||	// e
			!f({0, rc}, {1, 0}))	// s
			return false;
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	int sz = m_game->m_area_info[m_area_index].m_disps.size();
	for(int i = 0; i < sz; ++i){
		children.push_back(*this);
		if(!children.back().make_move(i))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << cells({r, c}) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_Calcudoku()
{
	using namespace puzzles::Calcudoku;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Calcudoku.xml", "Puzzles\\Calcudoku.txt", solution_format::GOAL_STATE_ONLY);
}