#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	iOS Game: Logic Games/Puzzle Set 1/Digital Battle Ships

	Summary
	Please divert your course 12+1+2 to avoid collision

	Description
	1. Play like Solo Battle Ships, with a difference.
	2. Each number on the outer board tells you the SUM of the ship or
	   ship pieces you're seeing in that row or column.
	3. A ship or ship piece is worth the number it occupies on the board.
	4. Standard rules apply: a ship or piece of ship can't touch another,
	   not even diagonally.
	5. In each puzzle there are
	   1 Aircraft Carrier (4 squares)
	   2 Destroyers (3 squares)
	   3 Submarines (2 squares)
	   4 Patrol boats (1 square)

	Variant
	5. Some puzzle can also have a:
	   1 Supertanker (5 squares)
*/

namespace puzzles{ namespace DigitalBattleships{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_TOP			'^'
#define PUZ_BOTTOM		'v'
#define PUZ_LEFT		'<'
#define PUZ_RIGHT		'>'
#define PUZ_MIDDLE		'+'
#define PUZ_BOAT		'o'
#define PUZ_BOUNDARY	'B'

struct puz_ship_info {
	string m_pieces[2];
	string m_area[3];
};

const puz_ship_info ship_info[] = {
	{{"o", "o"}, {"...", ". .", "..."}},
	{{"<>", "^v"}, {"....", ".  .", "...."}},
	{{"<+>", "^+v"}, {".....", ".   .", "....."}},
	{{"<++>", "^++v"}, {"......", ".    .", "......"}},
	{{"<+++>", "^+++v"}, {".......", ".     .", "......."}},
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	bool m_has_supertank;
	map<int, int> m_ship2num;
	map<Position, int> m_pos2num;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size() + 1)
	, m_has_supertank(attrs.get<int>("SuperTank", 0) == 1)
	, m_piece_counts_rows(m_sidelen)
	, m_piece_counts_cols(m_sidelen)
{
	m_ship2num = map<int, int>{{1, 4}, {2, 3}, {3, 2}, {4, 1}};
	if(m_has_supertank)
		m_ship2num[5] = 1;

	for(int r = 1; r < m_sidelen; ++r){
		auto& str = strs[r - 1];
		for(int c = 1; c < m_sidelen; ++c)
			if(r != m_sidelen - 1 || c != m_sidelen - 1)
				(c == m_sidelen - 1 ? m_piece_counts_rows[r] :
				r == m_sidelen - 1 ? m_piece_counts_cols[c] :
				m_pos2num[{r, c}]) = atoi(str.substr(c * 2 - 2, 2).c_str());
	}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n, bool vert);
	void check_area();
	void find_matches();

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_ship2num, 0, [](int acc, const pair<int, int>& kv){
			return acc + kv.second;
		});
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	map<int, int> m_ship2num;
	map<int, vector<pair<Position, bool>>> m_matches;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
, m_piece_counts_rows(g.m_piece_counts_rows)
, m_piece_counts_cols(g.m_piece_counts_cols)
, m_ship2num(g.m_ship2num)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

	check_area();
	find_matches();
}

void puz_state::check_area()
{
	auto f = [](char& ch){
		if(ch == PUZ_SPACE)
			ch = PUZ_EMPTY;
	};

	for(int i = 1; i < sidelen() - 1; ++i){
		if(m_piece_counts_rows[i] == 0)
			for(int j = 1; j < sidelen() - 1; ++j)
				f(cells({i, j}));
		if(m_piece_counts_cols[i] == 0)
			for(int j = 1; j < sidelen() - 1; ++j)
				f(cells({j, i}));
	}
}

void puz_state::find_matches()
{
	m_matches.clear();
	for(const auto& kv : m_ship2num){
		int i = kv.first;
		for(int j = 0; j < 2; ++j){
			bool vert = j == 1;
			if(i == 1 && vert)
				continue;

			auto& s = ship_info[i - 1].m_pieces[j];
			int len = s.length();
			for(int r = 1; r < sidelen() - (!vert ? 1 : len); ++r){
				for(int c = 1; c < sidelen() - (vert ? 1 : len); ++c){
					Position p(r, c);
					if([&]{
						int nr = 0, nc = 0;
						auto p2 = p;
						auto os = vert ? Position(1, 0) : Position(0, 1);
						for(char ch : s){
							if(cells(p2) != PUZ_SPACE)
								return false;
							int n = m_game->m_pos2num.at(p2);
							if(n > (!vert ? m_piece_counts_cols[p2.second] : m_piece_counts_rows[p2.first]))
								return false;
							(!vert ? nr : nc) += n;
							p2 += os;
						}
						return !vert ? nr <= m_piece_counts_rows[p2.first] :
							nc <= m_piece_counts_cols[p2.second];
					}())
						m_matches[i].emplace_back(p + Position(-1, -1), vert);
				}
			}
		}
	}
}

bool puz_state::make_move(const Position& p, int n, bool vert)
{
	auto& info = ship_info[n - 1];
	int len = info.m_pieces[0].length();
	for(int r2 = 0; r2 < 3; ++r2)
		for(int c2 = 0; c2 < len + 2; ++c2){
			auto p2 = p + Position(!vert ? r2 : c2, !vert ? c2 : r2);
			char& ch = cells(p2);
			char ch2 = info.m_area[r2][c2];
			if(ch2 == ' '){
				ch = info.m_pieces[!vert ? 0 : 1][c2 - 1];
				int n = m_game->m_pos2num.at(p2);
				m_piece_counts_rows[p2.first] -= n;
				m_piece_counts_cols[p2.second] -= n;
			}
			else if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
		}

	if(--m_ship2num[n] == 0)
		m_ship2num.erase(n);
	check_area();
	find_matches();

	if(!is_goal_state())
		return boost::algorithm::all_of(m_ship2num, [&](const pair<int, int>& kv){
			return m_matches[kv.first].size() >= kv.second;
		});
	else
		return boost::algorithm::all_of_equal(m_piece_counts_rows, 0) &&
			boost::algorithm::all_of_equal(m_piece_counts_cols, 0);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<int, vector<pair<Position, bool>>>& kv1,
		const pair<int, vector<pair<Position, bool>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});
	for(auto& kv2 : m_matches.at(kv.first)){
		children.push_back(*this);
		if(!children.back().make_move(kv2.first, kv.first, kv2.second))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen(); ++r){
		for(int c = 1; c < sidelen(); ++c)
			if(r == sidelen() - 1 && c == sidelen() - 1)
				break;
			else if(c == sidelen() - 1)
				out << format("%2d") % m_game->m_piece_counts_rows[r] << ' ';
			else if(r == sidelen() - 1)
				out << format("%2d") % m_game->m_piece_counts_cols[c] << ' ';
			else{
				Position p(r, c);
				char ch = cells(p);
				if(ch == PUZ_EMPTY || ch == PUZ_SPACE)
					out << PUZ_EMPTY << ' ';
				else
					out << ch << m_game->m_pos2num.at(p);
				out << ' ';
			}
		out << endl;
	}
	return out;
}

}}

void solve_puz_DigitalBattleships()
{
	using namespace puzzles::DigitalBattleships;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\DigitalBattleships.xml", "Puzzles\\DigitalBattleships.txt", solution_format::GOAL_STATE_ONLY);
}
