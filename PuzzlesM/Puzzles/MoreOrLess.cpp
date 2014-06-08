#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 7/More Or Less

	Summary
	Mr. Futoshiki, meet Mr. Sudoku

	Description
	1. More or Less can be seen as a mix between Sudoku and Futoshiki.
	2. Standard Sudoku rules apply, i.e. you need to fill the board with
	   numbers 1 to 9 in every row, column and area, without repetitions.
	3. The hints however, are given by the Greater Than or Less Than symbols
	   between tiles.

	Variation
	4. Further levels have irregular areas, instead of the classic 3*3 Sudoku.
	   All the other rules are the same.
*/

namespace puzzles{ namespace MoreOrLess{

#define PUZ_SPACE		' '

const string op_walls_gt = "^>v<";
const string op_walls_lt = "v<^>";
const string op_walls_gt2 = "GGLL";
const string op_walls_lt2 = "LLGG";

enum class OP_WALLS_TYPE
{
	NOT_LINE,
	LT,
	GT,
};

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

struct puz_pos_info
{
	int m_area_id;
	vector<Position> m_smallers;
	vector<Position> m_greaters;
	string m_nums = "123456789";
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, char> m_horz_walls, m_vert_walls;
	vector<vector<Position>> m_areas;
	map<Position, puz_pos_info> m_pos2info;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

struct puz_state2 : Position
{
	puz_state2(const map<Position, char>& horz_walls, const map<Position, char>& vert_walls,
		const Position& p_start, OP_WALLS_TYPE op_walls_type)
		: m_horz_walls(horz_walls), m_vert_walls(vert_walls), m_op_walls_type(op_walls_type) {
		make_move(p_start);
	}

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const map<Position, char> &m_horz_walls, &m_vert_walls;
	const OP_WALLS_TYPE m_op_walls_type;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(int i = 0; i < 4; ++i){
		auto p = *this + offset[i];
		auto p_wall = *this + offset2[i];
		auto& walls = i % 2 == 0 ? m_horz_walls : m_vert_walls;
		char ch = walls.at(p_wall);
		if(m_op_walls_type != OP_WALLS_TYPE::LT && ch == op_walls_gt[i] ||
			m_op_walls_type != OP_WALLS_TYPE::GT && ch == op_walls_lt[i] ||
			m_op_walls_type == OP_WALLS_TYPE::GT && ch == op_walls_gt2[i] ||
			m_op_walls_type == OP_WALLS_TYPE::LT && ch == op_walls_lt2[i] ||
			m_op_walls_type == OP_WALLS_TYPE::NOT_LINE && ch == PUZ_SPACE){
			children.push_back(*this);
			children.back().make_move(p);
		}
	}
}

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2)
, m_areas(m_sidelen * 3)
{
	set<Position> rng;
	for(int r = 0;; ++r){
		// horz-walls
		auto& str_h = strs[r * 2];
		for(int c = 0; c < m_sidelen; ++c)
			m_horz_walls[{r, c}] = str_h[c * 2 + 1];
		if(r == m_sidelen) break;
		auto& str_v = strs[r * 2 + 1];
		for(int c = 0;; ++c){
			Position p(r, c);
			// vert-walls
			m_vert_walls[p] = str_v[c * 2];
			if(c == m_sidelen) break;
			rng.insert(p);
			m_areas[p.first].push_back(p);
			m_areas[m_sidelen + p.second].push_back(p);
		}
	}
	for(int n = 0; !rng.empty(); ++n){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls,
			*rng.begin(), OP_WALLS_TYPE::NOT_LINE}, smoves);
		int id = m_sidelen * 2 + n;
		auto& area = m_areas[id];
		for(auto& p : smoves){
			area.push_back(p);
			rng.erase(p);
			m_pos2info[p].m_area_id = id;
		}
		boost::sort(area);
	}
	for(int r = 0; r < m_sidelen; ++r)
		for (int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			auto& info = m_pos2info[p];
			for(int i = 0; i < 2; ++i){
				auto& v = i == 0 ? info.m_smallers : info.m_greaters;
				list<puz_state2> smoves;
				puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls,
					p, i == 0 ? OP_WALLS_TYPE::GT : OP_WALLS_TYPE::LT}, smoves);
				for(auto& p2 : smoves)
					if(p2 != p)
						v.push_back(p2);
				boost::sort(v);
			}
			auto& area = m_areas[info.m_area_id];
			auto f = [&](const vector<Position>& rng){
				return boost::count_if(rng, [&](const Position& p){
					return boost::algorithm::any_of_equal(area, p);
				});
			};
			int sz1 = f(info.m_smallers), sz2 = f(info.m_greaters);
			info.m_nums = info.m_nums.substr(sz1, 9 - sz1 - sz2);
		}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, char ch);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, string> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
	for(auto& kv : g.m_pos2info)
		m_pos2nums[kv.first] = kv.second.m_nums;
}

bool puz_state::make_move(const Position& p, char ch)
{
	cells(p) = ch;
	auto& areas = m_game->m_areas;
	auto& info = m_game->m_pos2info.at(p);

	auto f = [&](const Position& p2, function<bool(char)> g){
		auto it = m_pos2nums.find(p2);
		if(it != m_pos2nums.end())
			boost::remove_erase_if(it->second, g);
	};
	for(auto& area : {areas[p.first], areas[sidelen() + p.second], areas[info.m_area_id]})
		for(auto& p2 : area)
			if(p2 != p)
				f(p2, [ch](char ch2){ return ch2 == ch; });
	for(auto& p2 : info.m_smallers)
		f(p2, [ch](char ch2){ return ch2 > ch; });
	for(auto& p2 : info.m_greaters)
		f(p2, [ch](char ch2){ return ch2 < ch; });
	m_pos2nums.erase(p);
	return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, string>& kv){
		return kv.second.empty();
	}) && boost::algorithm::all_of(areas, [&](const vector<Position>& area){
		set<char> nums;
		for(auto& p2 : area){
			char ch2 = cells(p2);
			if(ch2 == PUZ_SPACE)
				for(char ch3 : m_pos2nums.at(p2))
					nums.insert(ch3);
			else
				nums.insert(ch2);
		}
		return nums.size() == 9;
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_pos2nums, [](
		const pair<const Position, string>& kv1,
		const pair<const Position, string>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(char ch : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, ch))
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

void solve_puz_MoreOrLess()
{
	using namespace puzzles::MoreOrLess;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\MoreOrLess.xml", "Puzzles\\MoreOrLess.txt", solution_format::GOAL_STATE_ONLY);
}