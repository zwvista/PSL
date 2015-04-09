#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 12/Fence Sentinels

	Summary
	We used to guard a castle, you know?

	Description
	1. The goal is to draw a single, uninterrupted, closed loop.
	2. The loop goes around all the numbers.
	3. The number tells you how many cells you can see horizontally or
	   vertically from there, including the cell itself.

	Variant
	4. Some levels are marked 'Inside Outside'. In this case some numbers
	   are on the outside of the loop.
*/

namespace puzzles{ namespace FenceSentinels{

#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

const string lineseg_off = "0000";
const string linesegs_all[] = {
	"0011", "0101", "0110", "1001", "1010", "1100",
};

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

typedef pair<Position, int> puz_line_info;
const puz_line_info lines_info[] = {
	{{0, 0}, 1}, {{0, 1}, 3}, 		// n
	{{0, 1}, 2}, {{1, 1}, 0}, 		// e
	{{1, 1}, 3}, {{1, 0}, 1}, 		// s
	{{1, 0}, 0}, {{0, 0}, 2}, 		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_dot_count;
	bool m_inside_outside;
	map<Position, int> m_pos2num;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_inside_outside(attrs.get<int>("InsideOutside", 0) == 1)
	, m_sidelen(strs.size() + 1)
	, m_dot_count(m_sidelen * m_sidelen)
{
	for(int r = 0; r < m_sidelen - 1; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 1; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
		}
	}
}

typedef vector<string> puz_dot;

struct puz_state : vector<puz_dot>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid_point(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	bool is_valid_cell(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() - 1 && p.second >= 0 && p.second < sidelen() - 1;
	}
	const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move_hint(const Position& p, int n);
	bool make_move_hint2(const Position& p, int n);
	bool make_move_dot(const Position& p, int n);
	int find_matches(bool init);
	int check_dots(bool init);
	bool check_loop() const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<vector<int>>> m_matches;
	set<Position> m_finished;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count, {lineseg_off}), m_game(&g)
{
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);
			for(auto& lines : linesegs_all)
				if([&]{
					for(int i = 0; i < 4; ++i)
						if(lines[i] == PUZ_LINE_ON && !is_valid_point(p + offset[i]))
							return false;
					return true;
				}())
					dt.push_back(lines);
		}

	for(auto& kv : g.m_pos2num)
		m_matches[kv.first];

	find_matches(true);
	check_dots(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perms = kv.second;
		perms.clear();

		int sum = m_game->m_pos2num.at(p) - 1;
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			for(auto p2 = p; n <= sum; p2 += os){
				auto f = [&](char ch){
					auto& info = lines_info[i * 2];
					const auto& dt = dots(p2 + info.first);
					return boost::algorithm::all_of(dt, [=](const string& line){
						return line[info.second] == ch;
					});
				};
				if(!is_valid_cell(p2 + os) || f(PUZ_LINE_ON)){
					nums.push_back(n);
					break;
				}
				else if(!m_game->m_inside_outside &&
					p2 == p && m_game->m_pos2num.count(p2 + os) != 0 ||
					f(PUZ_LINE_OFF))
					++n;
				else
					nums.push_back(n++);
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
				return make_move_hint2(p, 0) ? 1 : 0;
			}
	}
	return 2;
}

int puz_state::check_dots(bool init)
{
	int n = 2;
	for(;;){
		set<Position> newly_finished;
		for(int r = 0; r < sidelen(); ++r)
			for(int c = 0; c < sidelen(); ++c){
				Position p(r, c);
				const auto& dt = dots(p);
				if(dt.size() == 1 && m_finished.count(p) == 0)
					newly_finished.insert(p);
			}

		if(newly_finished.empty())
			return n;

		n = 1;
		for(const auto& p : newly_finished){
			const auto& lines = dots(p)[0];
			for(int i = 0; i < 4; ++i){
				auto p2 = p + offset[i];
				if(!is_valid_point(p2))
					continue;
				auto& dt = dots(p2);
				boost::remove_erase_if(dt, [&, i](const string& lines2){
					return lines2[(i + 2) % 4] != lines[i];
				});
				if(!init && dt.empty())
					return 0;
			}
			m_finished.insert(p);
		}
		m_distance += newly_finished.size();
	}
}

bool puz_state::make_move_hint2(const Position& p, int n)
{
	auto& perm = m_matches.at(p)[n];
	for(int i = 0; i < 4; ++i){
		int m = perm[i];
		auto p2 = p;
		auto& os = offset[i];
		for(int j = 0; j <= m; ++j, p2 += os){
			if(j == m && m_game->m_inside_outside && !is_valid_cell(p2 + os))
				break;
			for(int k = 0; k < 2; ++k){
				auto& info = lines_info[i * 2 + k];
				auto& dt = dots(p2 + info.first);
				char line = j < m ? PUZ_LINE_OFF : PUZ_LINE_ON;
				boost::remove_erase_if(dt, [&](const string& lines){
					return lines[info.second] != line;
				});
				if(dt.empty())
					return 0;
			}
		}
	}

	m_matches.erase(p);
	return check_loop();
}

bool puz_state::check_loop() const
{
	set<Position> rng;
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);
			if(dt.size() == 1 && dt[0] != lineseg_off)
				rng.insert(p);
		}

	bool has_loop = false;
	while(!rng.empty()){
		auto p = *rng.begin(), p2 = p;
		for(int n = -1;;){
			rng.erase(p2);
			auto& str = dots(p2)[0];
			for(int i = 0; i < 4; ++i)
				if(str[i] == PUZ_LINE_ON && (i + 2) % 4 != n){
					p2 += offset[n = i];
					break;
				}
			if(p2 == p)
				if(has_loop)
					return false;
				else{
					has_loop = true;
					break;
				}
			if(rng.count(p2) == 0)
				break;
		}
	}
	return true;
}

bool puz_state::make_move_hint(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move_hint2(p, n))
		return false;
	for(;;){
		int m;
		while((m = find_matches(false)) == 1);
		if(m == 0)
			return false;
		m = check_dots(false);
		if(m != 1)
			return m == 2;
		if(!check_loop())
			return false;
	}
}

bool puz_state::make_move_dot(const Position& p, int n)
{
	m_distance = 0;
	auto& dt = dots(p);
	dt = {dt[n]};
	int m = check_dots(false);
	return m == 1 ? check_loop() : m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_matches.empty()){
		auto& kv = *boost::min_element(m_matches, [](
			const pair<const Position, vector<vector<int>>>& kv1,
			const pair<const Position, vector<vector<int>>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});

		for(int i = 0; i < kv.second.size(); ++i){
			children.push_back(*this);
			if(!children.back().make_move_hint(kv.first, i))
				children.pop_back();
		}
	}
	else{
		int i = boost::min_element(*this, [&](const puz_dot& dt1, const puz_dot& dt2){
			auto f = [](const puz_dot& dt){
				int sz = dt.size();
				return sz == 1 ? 100 : sz;
			};
			return f(dt1) < f(dt2);
		}) - begin();
		auto& dt = (*this)[i];
		Position p(i / sidelen(), i % sidelen());
		for(int n = 0; n < dt.size(); ++n){
			children.push_back(*this);
			if(!children.back().make_move_dot(p, n))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen(); ++c)
			out << (dots({r, c})[0][1] == PUZ_LINE_ON ? " --" : "   ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-lines
			out << (dots(p)[0][2] == PUZ_LINE_ON ? '|' : ' ');
			if(c == sidelen() - 1) break;
			auto it = m_game->m_pos2num.find(p);
			if(it == m_game->m_pos2num.end())
				out << "  ";
			else
				out << format("%2d") % it->second;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_FenceSentinels()
{
	using namespace puzzles::FenceSentinels;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\FenceSentinels.xml", "Puzzles\\FenceSentinels.txt", solution_format::GOAL_STATE_ONLY);
}