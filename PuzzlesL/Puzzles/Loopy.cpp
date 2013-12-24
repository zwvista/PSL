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

#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	set<Position> m_horz_lines, m_vert_lines;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2)
{
	for(int r = 0;; ++r){
		auto& str_horz = strs[2 * r];
		// horz-lines
		for(int c = 0; c < m_sidelen; ++c)
			if(str_horz[2 * c + 1] == '-')
				m_horz_lines.emplace(r, c);
		if(r == m_sidelen) break;
		auto& str_vert = strs[2 * r + 1];
		// vert-lines
		for(int c = 0; c <= m_sidelen; ++c)
			if(str_vert[2 * c] == '|')
				m_vert_lines.emplace(r, c);
	}
}

typedef vector<string> puz_point;

struct puz_state : vector<puz_point>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	const puz_point& points(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	puz_point& points(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
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
: vector<puz_point>(g.m_sidelen * g.m_sidelen), m_game(&g)
{
	auto lines_off = string(4, PUZ_LINE_OFF);
	for(int r = 0; r < sidelen(); ++r)
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			auto& pt = points(p);
			pt.push_back(lines_off);

			vector<int> dirs;
			for(int i = 0; i < 4; ++i)
				if(is_valid(p + offset[i]))
					dirs.push_back(i);

			for(int i = 0; i < dirs.size() - 1; ++i)
				for(int j = i + 1; j < dirs.size(); ++j){
					auto lines = lines_off;
					lines[dirs[i]] = lines[dirs[j]] = PUZ_LINE_ON;
					pt.push_back(lines);
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
				const auto& pt = points(p + info.first);
				char line = perm[i / 2];
				int j = info.second;
				if(boost::algorithm::none_of(pt, [=](const string& s){
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
		auto& pt = points(p + info.first);
		char line = perm[i / 2];
		int j = info.second;
		boost::remove_erase_if(pt, [=](const string& s){
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
			out << (points({r, c})[0][1] == PUZ_LINE_ON ? " -" : "  ");
		out << endl;
		if(r == sidelen() - 1) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-lines
			out << (points(p)[0][2] == PUZ_LINE_ON ? '|' : ' ');
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

void solve_puz_Loopy()
{
	using namespace puzzles::Loopy;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Loopy.xml", "Puzzles\\Loopy.txt", solution_format::GOAL_STATE_ONLY);
}