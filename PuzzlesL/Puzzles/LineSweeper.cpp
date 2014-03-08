#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/LineSweeper

	Summary
	Draw a single loop, minesweeper style

	Description
	1. Draw a single closed looping path that never crosses itself or branches off.
	2. A number in a cell denotes how many of the 8 adjacent cells are passed
	   by the loop.
	3. The loop can only go horizontally or vertically between cells, but
	   not over the numbers.
	4. The loop doesn't need to cover all the board.
*/

namespace puzzles{ namespace LineSweeper{

#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

const string lines_off = string(4, PUZ_LINE_OFF);
const string lines_all[] = {
	"0011", "0101", "0110", "1001", "1010", "1100",
};

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_dot_count;
	map<Position, int> m_pos2num;
	map<int, vector<vector<string>>> m_num2perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen{strs.size()}
	, m_dot_count{m_sidelen * m_sidelen}
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != ' '){
				int n = ch - '0';
				m_num2perms[n];
				m_pos2num[{r, c}] = n;
			}
		}
	}

	for(auto& kv : m_num2perms){
		int n = kv.first;
		auto& perms = kv.second;
		auto indicator = string(8 - n, PUZ_LINE_OFF) + string(n, PUZ_LINE_ON);
		do{
			set<Position> rng;
			for(int i = 0; i < 8; ++i)
				if(indicator[i] == PUZ_LINE_ON)
					rng.insert(offset[i]);

			vector<vector<string>> dir_lines(8);
			for(int i = 0; i < 8; ++i)
				if(indicator[i] == PUZ_LINE_OFF)
					dir_lines[i] = {lines_off};
				else
					for(int j = 0; j < 6; ++j){
						auto& lines = lines_all[j];
						if([&]{
							for(int k = 0; k < 4; ++k){
								if(lines[k] == PUZ_LINE_OFF)
									continue;
								auto p = offset[i] + offset[2 * k];
								if(p == Position{} ||
									boost::algorithm::any_of_equal(offset, p) &&
									rng.count(p) == 0)
									return false;
							}
							return true;
						}())
							dir_lines[i].push_back(lines);
					}
			if(boost::algorithm::any_of(dir_lines, [](const vector<string>& lines){
				return lines.empty();
			}))
				continue;

			vector<int> indexes(8);
			vector<string> perm(8);
			for(int i = 0; i < 8;){
				for(int j = 0; j < 8; ++j)
					perm[j] = dir_lines[j][indexes[j]];
				if([&]{
					for(int j = 0; j < 8; ++j){
						auto& lines = perm[j];
						for(int k = 0; k < 4; ++k){
							if(lines[k] == PUZ_LINE_OFF)
								continue;
							auto p = offset[j] + offset[2 * k];
							int n = boost::find(offset, p) - offset;
							if(n < 8 && perm[n][(k + 2) % 4] == PUZ_LINE_OFF)
								return false;
						}
					}
					return true;
				}())
					perms.push_back(perm);
				for(i = 0; i < 8 && ++indexes[i] == dir_lines[i].size(); ++i)
					indexes[i] = 0;
			}

		}while(boost::next_permutation(indicator));
	}
}

typedef vector<string> puz_dot;

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
	unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
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
: vector<puz_dot>(g.m_dot_count, {lines_off}), m_game(&g)
{
	for(const auto& kv : g.m_pos2num){
		m_finished.insert(kv.first);
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_num2perms.at(kv.second).size());
		boost::iota(perm_ids, 0);
	}

	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			if(m_finished.count(p) != 0)
				continue;

			vector<int> dirs;
			for(int i = 0; i < 4; ++i){
				auto p2 = p + offset[i * 2];
				if(is_valid(p2) && m_finished.count(p2) == 0)
					dirs.push_back(i);
			}

			auto& dt = dots(p);
			for(int i = 0; i < dirs.size() - 1; ++i)
				for(int j = i + 1; j < dirs.size(); ++j){
					auto lines = lines_off;
					lines[dirs[i]] = lines[dirs[j]] = PUZ_LINE_ON;
					dt.push_back(lines);
				}
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
				auto p2 = p + offset[i];
				auto& lines = perm[i];
				if(!is_valid(p2)){
					if(lines != lines_off)
						return true;
				}
				else if(boost::algorithm::none_of_equal(dots(p2), lines))
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
			const auto& lines = dots(p)[0];
			for(int i = 0; i < 4; ++i){
				auto p2 = p + offset[i * 2];
				if(!is_valid(p2))
					continue;
				auto& dt = dots(p2);
				boost::remove_erase_if(dt, [&, i](const string& s){
					return s[(i + 2) % 4] != lines[i];
				});
				if(!init && dt.empty())
					return 0;
			}
			m_finished.insert(p);
		}
	}
}

bool puz_state::make_move2(const Position& p, int n)
{
	auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
	for(int i = 0; i < 8; ++i){
		auto p2 = p + offset[i];
		if(is_valid(p2))
			dots(p2) = {perm[i]};
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
			if(dt.size() == 1 && dt[0] != lines_off)
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
					p2 += offset[(n = i) * 2];
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
	if(!m_matches.empty()){
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
	else{
		// TODO: does such case exist?
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto it = m_game->m_pos2num.find(p);
			out << char(it == m_game->m_pos2num.end() ? ' ' : it->second + '0')
				<< (dots({r, c})[0][1] == PUZ_LINE_ON ? '-' : ' ');
		}
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0; c < sidelen(); ++c)
			// draw vert-lines
			out << (dots({r, c})[0][2] == PUZ_LINE_ON ? "| " : "  ");
		out << endl;
	}
	return out;
}

}}

void solve_puz_LineSweeper()
{
	using namespace puzzles::LineSweeper;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\LineSweeper.xml", "Puzzles\\LineSweeper.txt", solution_format::GOAL_STATE_ONLY);
}