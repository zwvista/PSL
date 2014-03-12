#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Digital Battle Ships

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

const string ship_pieces[][2] = {
	{"o", "o"},
	{"<>", "^v"},
	{"<+>", "^+v"},
	{"<++>", "^++v"},
	{"<+++>", "^+++v"},
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	bool m_has_supertank;
	vector<int> m_ships;
	map<Position, int> m_pos2num;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen(strs.size() - 1)
	, m_has_supertank(attrs.get<int>("SuperTank", 0) == 1)
{
	m_ships = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4};
	if(m_has_supertank)
		m_ships.push_back(5);

	for(int r = 0; r <= m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c <= m_sidelen; ++c){
			auto s = str.substr(c * 2, 2);
			if(r == m_sidelen && c == m_sidelen)
				break;
			int n = atoi(s.c_str());
			if(c == m_sidelen)
				m_piece_counts_rows.push_back(n);
			else if(r == m_sidelen)
				m_piece_counts_cols.push_back(n);
			else
				m_pos2num[{r, c}] = n;
		}
	}
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, int n, bool vert);
	void check_area();
	void find_matches();

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_ships.size(); }
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	vector<int> m_piece_counts_rows, m_piece_counts_cols;
	vector<int> m_ships;
	map<int, vector<pair<Position, bool>>> m_matches;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
, m_piece_counts_rows(g.m_piece_counts_rows)
, m_piece_counts_cols(g.m_piece_counts_cols)
, m_ships(g.m_ships)
{
	check_area();
	find_matches();
}

void puz_state::check_area()
{
	auto f = [](char& ch){
		if(ch == PUZ_SPACE)
			ch = PUZ_EMPTY;
	};

	for(int i = 0; i < sidelen(); ++i){
		if(m_piece_counts_rows[i] == 0)
			for(int j = 0; j < sidelen(); ++j)
				f(cells({i, j}));
		if(m_piece_counts_cols[i] == 0)
			for(int j = 0; j < sidelen(); ++j)
				f(cells({j, i}));
	}
}

void puz_state::find_matches()
{
	m_matches.clear();
	set<int> ships(m_ships.begin(), m_ships.end());
	for(int i : ships)
		for(int j = 0; j < 2; ++j){
			bool vert = j == 1;
			if(i == 1 && vert)
				continue;

			const auto& s = ship_pieces[i - 1][j];
			int len = s.length();
			for(int r = 0; r < sidelen() - (!vert ? 0 : len - 1); ++r){
				for(int c = 0; c < sidelen() - (vert ? 0 : len - 1); ++c){
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
						m_matches[i].push_back({p, vert});
				}
			}
		}
}

bool puz_state::make_move(const Position& p, int n, bool vert)
{
	auto os = vert ? Position(1, 0) : Position(0, 1);
	const auto& s = ship_pieces[n - 1][vert ? 1 : 0];

	auto p2 = p;
	auto f = [&](const set<int>& ids){
		for(int i = 0; i < 8; ++i)
			if(ids.count(i) == 0){
				auto p3 = p2 + offset[i];
				if(is_valid(p3))
					cells(p3) = PUZ_EMPTY;
			}
	};
	for(char ch : s){
		switch(cells(p2) = ch){
		case PUZ_BOAT:
			f({});
			break;
		case PUZ_TOP:
			f({4});
			break;
		case PUZ_BOTTOM:
			f({0});
			break;
		case PUZ_LEFT:
			f({2});
			break;
		case PUZ_RIGHT:
			f({6});
			break;
		case PUZ_MIDDLE:
			f({0, 2, 4, 6});
			break;
		}
		int n = m_game->m_pos2num.at(p2);
		m_piece_counts_rows[p2.first] -= n;
		m_piece_counts_cols[p2.second] -= n;
		p2 += os;
	}

	m_ships.erase(boost::range::find(m_ships, n));
	check_area();
	find_matches();
	return !is_goal_state() && boost::algorithm::all_of(m_ships, [&](int i){
		return m_matches.count(i) != 0;
	}) || is_goal_state() &&
		boost::algorithm::all_of_equal(m_piece_counts_rows, 0) &&
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
	for(int r = 0; r <= sidelen(); ++r){
		for(int c = 0; c <= sidelen(); ++c)
			if(r == sidelen() && c == sidelen())
				break;
			else if(c == sidelen())
				out << format("%2d") % m_game->m_piece_counts_rows[r] << ' ';
			else if(r == sidelen())
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
