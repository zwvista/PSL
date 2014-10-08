#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/Masyu

	Summary
	Draw a Necklace that goes through every Pearl

	Description
	1. The goal is to draw a single Loop(Necklace) through every circle(Pearl)
	   that never branches-off or crosses itself.
	2. The rules to pass Pears are:
	3. Lines passing through White Pearls must go straight through them.
	   However, at least at one side of the White Pearl(or both), they must
	   do a 90 degree turn.
	4. Lines passing through Black Pearls must do a 90 degree turn in them.
	   Then they must go straight in the next tile in both directions.
	5. Lines passing where there are no Pearls can do what they want.
*/

namespace puzzles{ namespace Masyu{

#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'
#define PUZ_BLACK_PEARL		'B'
#define PUZ_WHITE_PEARL		'W'

const string lines_off = "0000";
const vector<string> lines_all = {
	"0011", "0101", "0110", "1001", "1010", "1100",
};
const vector<string> lines_all_black = {
	"1100", "0110", "0011", "1001",
};
const vector<string> lines_all_white = {
	"1010", "0101",
};

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

typedef pair<Position, string> puz_dot_info;
const puz_dot_info dot_info_black[][3] = {
	{{{0, 0}, "1100"}, {{-1, 0}, "1010"}, {{0, 1}, "0101"}},		// n & e
	{{{0, 0}, "0110"}, {{0, 1}, "0101"}, {{1, 0}, "1010"}},		// e & s
	{{{0, 0}, "0011"}, {{1, 0}, "1010"}, {{0, -1}, "0101"}},		// s & w
	{{{0, 0}, "1001"}, {{0, -1}, "0101"}, {{-1, 0}, "1010"}},	// w & n
};
const puz_dot_info dot_info_white[][3] = {
	{{{0, 0}, "1010"}, {{-1, 0}, "1010"}, {{1, 0}, "1100"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "1010"}, {{1, 0}, "1001"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0110"}, {{1, 0}, "1100"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0110"}, {{1, 0}, "1010"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0110"}, {{1, 0}, "1001"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0011"}, {{1, 0}, "1100"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0011"}, {{1, 0}, "1010"}},		// n & s
	{{{0, 0}, "1010"}, {{-1, 0}, "0011"}, {{1, 0}, "1001"}},		// n & s

	{{{0, 0}, "0101"}, {{0, 1}, "1001"}, {{0, -1}, "1100"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "1001"}, {{0, -1}, "0110"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "1001"}, {{0, -1}, "0101"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "0101"}, {{0, -1}, "1100"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "0101"}, {{0, -1}, "0110"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "0011"}, {{0, -1}, "1100"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "0011"}, {{0, -1}, "0110"}},		// e & w
	{{{0, 0}, "0101"}, {{0, 1}, "0011"}, {{0, -1}, "0101"}},		// e & w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_dot_count;
	map<Position, char> m_pos2pearl;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
	}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
	, m_dot_count(m_sidelen * m_sidelen)
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_pos2pearl[{r, c}] = ch;
		}
	}
}

typedef vector<string> puz_dot;

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
	puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const {
		return make_pair(m_dots, m_matches) < make_pair(x.m_dots, x.m_matches); 
	}
	bool make_move_pearl(const Position& p, int n);
	bool make_move_pearl2(const Position& p, int n);
	bool make_move_line(const Position& p, int n);
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
	vector<puz_dot> m_dots;
	map<Position, vector<int>> m_matches;
	set<Position> m_finished;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count), m_game(&g)
{
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);
			auto it = g.m_pos2pearl.find(p);
			if(it == g.m_pos2pearl.end())
				dt.push_back(lines_off);

			auto& lines_all2 = 
				it == g.m_pos2pearl.end() ? lines_all :
				it->second == PUZ_BLACK_PEARL ? lines_all_black :
				lines_all_white;
			for(auto& lines : lines_all2)
				if([&]{
					for(int i = 0; i < 4; ++i)
						if(lines[i] == PUZ_LINE_ON && !is_valid(p + offset[i]))
							return false;
					return true;
				}())
					dt.push_back(lines);
		}

	for(const auto& kv : g.m_pos2pearl){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(kv.second == PUZ_BLACK_PEARL ? 4 : 16);
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

		auto info = m_game->m_pos2pearl.at(p) == PUZ_BLACK_PEARL ?
			dot_info_black : dot_info_white;
		boost::remove_erase_if(perm_ids, [&](int id){
			auto info2 = info[id];
			for(int i = 0; i < 3; ++i){
				auto& info3 = info2[i];
				if(boost::algorithm::none_of_equal(dots(p + info3.first), info3.second))
					return true;
			}
			return false;
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move_pearl2(p, perm_ids.front()) ? 1 : 0;
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
		m_distance += newly_finished.size();
	}
}

bool puz_state::make_move_pearl2(const Position& p, int n)
{
	auto info = m_game->m_pos2pearl.at(p) == PUZ_BLACK_PEARL ?
		dot_info_black : dot_info_white;
	auto info2 = info[n];
	for(int i = 0; i < 3; ++i){
		auto& info3 = info2[i];
		dots(p + info3.first) = {info3.second};
	}
	m_matches.erase(p);
	return check_loop();
}

bool puz_state::make_move_pearl(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move_pearl2(p, n))
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

bool puz_state::make_move_line(const Position& p, int n)
{
	m_distance = 0;
	auto& dt = dots(p);
	dt = {dt[n]};
	return check_dots(false) != 0 && check_loop();
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
			if(!children.back().make_move_pearl(kv.first, n))
				children.pop_back();
		}
	}
	else{
		int n = boost::min_element(m_dots, [](const puz_dot& dt1, const puz_dot& dt2){
			auto f = [](int sz){return sz == 1 ? 1000 : sz;};
			return f(dt1.size()) < f(dt2.size());
		}) - m_dots.begin();
		Position p(n / sidelen(), n % sidelen());
		auto& dt = dots(p);
		for(int i = 0; i < dt.size(); ++i){
			children.push_back(*this);
			if(!children.back().make_move_line(p, i))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);
			auto it = m_game->m_pos2pearl.find(p);
			out << (it != m_game->m_pos2pearl.end() ? it->second : ' ')
				<< (dt[0][1] == PUZ_LINE_ON ? '-' : ' ');
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

void solve_puz_Masyu()
{
	using namespace puzzles::Masyu;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Masyu.xml", "Puzzles\\Masyu.txt", solution_format::GOAL_STATE_ONLY);
}