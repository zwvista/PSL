#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/SlitherLink

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
	map<Position, int> m_pos2num;
	map<int, vector<string>> m_num2perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 1)
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
		auto perm = string(4 - i, PUZ_LINE_OFF) + string(i, PUZ_LINE_ON);
		do{
			perms.push_back(perm);
			perms_unknown.push_back(perm);
		}while(boost::next_permutation(perm));
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
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_sidelen * g.m_sidelen), m_game(&g)
{
	auto lines_off = string(4, PUZ_LINE_OFF);
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& dt = dots(p);
			dt.push_back(lines_off);

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

	for(const auto& kv : g.m_pos2num){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_num2perms.at(kv.second).size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
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
				char line = perm[i / 2];
				int j = info.second;
				if(boost::algorithm::none_of(dt, [=](const string& s){
					return s[j] == line;
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

bool puz_state::make_move2(const Position& p, int n)
{
	auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
	for(int i = 0; i < 8; ++i){
		auto& info = lines_info[i];
		auto& dt = dots(p + info.first);
		char line = perm[i / 2];
		int j = info.second;
		boost::remove_erase_if(dt, [=](const string& s){
			return s[j] != line;
		});
	}

	++m_distance;
	m_matches.erase(p);

	// TODO: check the loop
	return true;
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move2(p, n))
		return false;
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
			out << (dots({r, c})[0][1] == PUZ_LINE_ON ? " -" : "  ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-lines
			out << (dots(p)[0][2] == PUZ_LINE_ON ? '|' : ' ');
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