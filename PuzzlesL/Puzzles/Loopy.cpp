#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/Loopy

	Summary
	Loop a loop! And touch all the dots!

	Description
	1. Draw a single looping path. You have to touch all the dots. As usual,
	   the path cannot have branches or cross itself.
*/

namespace puzzles{ namespace Loopy{

#define PUZ_LINE_UNKNOWN	"01"
#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

typedef pair<Position, int> puz_line_info;
const puz_line_info lines_info[] = {
	{{0, 0}, 1}, {{0, 1}, 3}, 		// horz-line
	{{0, 0}, 2}, {{1, 0}, 0}, 		// vert-line
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_dot_count;
	set<Position> m_horz_lines, m_vert_lines;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() / 2 + 1)
, m_dot_count(m_sidelen * m_sidelen)
{
	for(int r = 0;; ++r){
		auto& str_horz = strs[2 * r];
		// horz-lines
		for(int c = 0; c < m_sidelen - 1; ++c)
			if(str_horz[2 * c + 1] == '-')
				m_horz_lines.emplace(r, c);
		if(r == m_sidelen - 1) break;
		auto& str_vert = strs[2 * r + 1];
		// vert-lines
		for(int c = 0; c <= m_sidelen - 1; ++c)
			if(str_vert[2 * c] == '|')
				m_vert_lines.emplace(r, c);
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
	bool make_move(const Position& p, bool is_vert, char ch);
	bool make_move2(const Position& p, bool is_vert, char ch);
	int find_matches(bool init);
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
	map<pair<Position, bool>, string> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count), m_game(&g)
{
	auto lines_off = string(4, PUZ_LINE_OFF);
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);

			vector<int> dirs;
			for(int i = 0; i < 4; ++i)
				if(is_valid(p + offset[i]))
					dirs.push_back(i);

			for(int i = 0; i < dirs.size() - 1; ++i)
				for(int j = i + 1; j < dirs.size(); ++j){
					auto lines = lines_off;
					lines[dirs[i]] = lines[dirs[j]] = PUZ_LINE_ON;
					dt.push_back(lines);
				}
		}

	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			if(c < sidelen() - 1)
				m_matches[{p, false}] = PUZ_LINE_UNKNOWN;
			if(r < sidelen() - 1)
				m_matches[{p, true}] =  PUZ_LINE_UNKNOWN;
		}

	for(auto& p : g.m_horz_lines)
		make_move2(p, false, PUZ_LINE_ON);
	for(auto& p : g.m_vert_lines)
		make_move2(p, true, PUZ_LINE_ON);

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first.first;
		bool is_vert = kv.first.second;
		auto& str = kv.second;

		int index = is_vert ? 2 : 0;
		auto &info1 = lines_info[index], &info2 = lines_info[index + 1];
		auto f = [&](const puz_line_info& info, char ch){
			return boost::algorithm::any_of(dots(p + info.first),
				[&](const string& s) { return s[info.second] == ch; });
		};
		str.clear();
		if(f(info1, PUZ_LINE_ON) && f(info2, PUZ_LINE_ON))
			str.push_back(PUZ_LINE_ON);
		if(f(info1, PUZ_LINE_OFF) && f(info2, PUZ_LINE_OFF))
			str.push_back(PUZ_LINE_OFF);

		if(!init)
			switch(str.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, is_vert, str[0]) ? 1 : 0;
		}
	}
	return 2;
}

bool puz_state::make_move2(const Position& p, bool is_vert, char ch)
{
	int index = is_vert ? 2 : 0;
	auto &info1 = lines_info[index], &info2 = lines_info[index + 1];
	auto f = [&, ch](const puz_line_info& info){
		boost::remove_erase_if(dots(p + info.first),
			[&](const string& s) { return s[info.second] != ch; });
	};
	f(info1), f(info2);

	++m_distance;
	m_matches.erase({p, is_vert});

	return check_loop();
}

bool puz_state::check_loop() const
{
	set<Position> rng;
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			if(dots(p).size() == 1)
				rng.insert(p);
		}
	if(m_matches.empty() && rng.size() != m_game->m_dot_count)
		return false;

	while(!rng.empty()){
		auto p = *rng.begin(), p2 = p;
		for(int cnt = 1, n = -1;; ++cnt){
			rng.erase(p2);
			auto& str = dots(p2)[0];
			for(int i = 0; i < 4; ++i)
				if(str[i] == PUZ_LINE_ON && (i + 2) % 4 != n){
					p2 += offset[n = i];
					break;
				}
			if(p2 == p)
				return cnt == m_game->m_dot_count;
			if(rng.count(p2) == 0)
				break;
		}
	}
	return true;
}

bool puz_state::make_move(const Position& p, bool is_vert, char ch)
{
	m_distance = 0;
	if(!make_move2(p, is_vert, ch))
		return false;
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<const pair<Position, bool>, string>& kv1,
		const pair<const pair<Position, bool>, string>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(char ch : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first.first, kv.first.second, ch))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen() - 1; ++c)
			out << (dots({r, c})[0][1] == PUZ_LINE_ON ? " -" : "  ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			// draw vert-lines
			out << (dots(p)[0][2] == PUZ_LINE_ON ? "| " : "  ");
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_Loopy()
{
	using namespace puzzles::Loopy;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Loopy.xml", "Puzzles\\Loopy.txt", solution_format::GOAL_STATE_ONLY);
}