#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 3/SlitherLink

	Summary
	Draw a loop a-la-minesweeper!

	Description
	1. Draw a single looping path with the aid of the numbered hints. The path
	   cannot have branches or cross itself.
	2. Each number in a tile tells you on how many of its four sides are touched
	   by the path.
	3. For example:
	4. A 0 tells you that the path doesn't touch that square at all.
	5. A 1 tells you that the path touches that square ONLY one-side.
	6. A 3 tells you that the path does a U-turn around that square.
	7. There can't be tiles marked with 4 because that would form a single
	   closed loop in it.
	8. Empty tiles can have any number of sides touched by that path.
*/

namespace puzzles{ namespace SlitherLink{

#define PUZ_UNKNOWN			-1
#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
	// ┐  ─  ┌  ┘  │  └
	12, 10, 6, 9, 5, 3,
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
	map<Position, int> m_pos2num;
	map<int, vector<int>> m_num2perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 1)
, m_dot_count(m_sidelen * m_sidelen)
{
	for(int r = 0; r < m_sidelen - 1; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 1; ++c){
			char ch = str[c];
			m_pos2num[{r, c}] = ch != ' ' ? ch - '0' : PUZ_UNKNOWN;
		}
	}

	auto& perms_unknown = m_num2perms[PUZ_UNKNOWN];
	for(int i = 0; i < 4; ++i){
		auto& perms = m_num2perms[i];
		auto indicator = string(4 - i, PUZ_LINE_OFF) + string(i, PUZ_LINE_ON);
		do{
			int perm = 0;
			for(int j = 0; j < 4; ++j)
				if(indicator[j] == PUZ_LINE_ON)
					perm += (1 << j);
			perms.push_back(perm);
			perms_unknown.push_back(perm);
		}while(boost::next_permutation(indicator));
	}
}

typedef vector<int> puz_dot;

struct puz_state : vector<puz_dot>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
	int find_matches(bool init);
	int check_dots(bool init);
	bool check_loop() const;

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
	map<Position, vector<int>> m_matches;
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
			for(int lineseg : linesegs_all)
				if([&]{
					for(int i = 0; i < 4; ++i)
						// The line segment cannot lead to a position outside the board
						if(is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
							return false;
					return true;
				}())
					dt.push_back(lineseg);
		}

	for(auto& kv : g.m_pos2num){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_num2perms.at(kv.second).size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
	check_dots(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perm_ids = kv.second;

		auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
		boost::remove_erase_if(perm_ids, [&](int id){
			auto& perm = perms[id];
			for(int i = 0; i < 8; ++i){
				auto& info = lines_info[i];
				const auto& dt = dots(p + info.first);
				bool is_on = is_lineseg_on(perm, i / 2);
				int j = info.second;
				if(boost::algorithm::none_of(dt, [=](int lineseg){
					return is_lineseg_on(lineseg, j) == is_on;
				}))
					return true;
			}
			return false;
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, perm_ids.front()) ? 1 : 0;
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
			int lineseg = dots(p)[0];
			for(int i = 0; i < 4; ++i){
				auto p2 = p + offset[i];
				if(!is_valid(p2))
					continue;
				auto& dt = dots(p2);
				boost::remove_erase_if(dt, [&, i](int lineseg2){
					return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
				});
				if(!init && dt.empty())
					return 0;
			}
			m_finished.insert(p);
		}
		//m_distance += newly_finished.size();
	}
}

bool puz_state::make_move2(const Position& p, int n)
{
	auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
	for(int i = 0; i < 8; ++i){
		auto& info = lines_info[i];
		auto& dt = dots(p + info.first);
		bool is_on = is_lineseg_on(perm, i / 2);
		int j = info.second;
		boost::remove_erase_if(dt, [=](int lineseg){
			return is_lineseg_on(lineseg, j) != is_on;
		});
	}

	++m_distance;
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
			if(dt.size() != 1 && m_matches.empty())
				return false;
			if(dt.size() == 1 && dt[0] != lineseg_off)
				rng.insert(p);
		}

	while(!rng.empty()){
		auto p = *rng.begin(), p2 = p;
		for(int n = -1;;){
			rng.erase(p2);
			int lineseg = dots(p2)[0];
			for(int i = 0; i < 4; ++i)
				// go ahead if the line segment does not lead a way back
				if(is_lineseg_on(lineseg, i) && (i + 2) % 4 != n){
					p2 += offset[n = i];
					break;
				}
			if(p2 == p)
				// we have a loop here,
				// so we should have exhausted the line segments 
				return rng.empty();
			if(rng.count(p2) == 0)
				break;
		}
	}
	return true;
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move2(p, n))
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

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const Position, vector<int>>& kv1,
		const pair<const Position, vector<int>>& kv2){
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
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen(); ++c)
			out << (is_lineseg_on(dots({r, c})[0], 1) ? " -" : "  ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-lines
			out << (is_lineseg_on(dots(p)[0], 2) ? '|' : ' ');
			if(c == sidelen() - 1) break;
			int n = m_game->m_pos2num.at(p);
			if(n == PUZ_UNKNOWN)
				out << ' ';
			else
				out << n;
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_SlitherLink()
{
	using namespace puzzles::SlitherLink;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\SlitherLink.xml", "Puzzles\\SlitherLink.txt", solution_format::GOAL_STATE_ONLY);
}