#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Light Battle Ships

	Summary
	Please divert your course 15 degrees to avoid collision

	Description
	1. A mix of Battle Ships and Lighthouses, you have to guess the usual
	   piece of ships with the help of Lighthouses.
	2. Each number is a Lighthouse, telling you how many pieces of ship
	   there are in that row and column, summed together.
	3. Ships cannot touch each other OR touch Lighthouses. Not even diagonally.
	4. In each puzzle there are
	   1 Aircraft Carrier (4 squares)
	   2 Destroyers (3 squares)
	   3 Submarines (2 squares)
	   4 Patrol boats (1 square)

	Variant
	5. Some puzzle can also have a:
	   1 Supertanker (5 squares)
*/

namespace puzzles{ namespace LightBattleships{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_LIGHT		'L'
#define PUZ_TOP			'^'
#define PUZ_BOTTOM		'v'
#define PUZ_LEFT		'<'
#define PUZ_RIGHT		'>'
#define PUZ_MIDDLE		'+'
#define PUZ_BOAT		'o'
#define PUZ_BOUNDARY	'B'

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
	bool m_has_supertank;
	map<int, int> m_ship2num;
	map<Position, char> m_pos2piece;
	map<Position, int> m_pos2light;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen(strs.size() + 2)
	, m_has_supertank(attrs.get<int>("SuperTank", 0) == 1)
{
	m_ship2num = map<int, int>{{1, 4}, {2, 3}, {3, 2}, {4, 1}};
	if(m_has_supertank)
		m_ship2num[5] = 1;

	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			Position p(r, c);
			switch(char ch = str[c - 1]){
			case PUZ_SPACE:
			case PUZ_EMPTY:
				m_start.push_back(ch);
				break;
			case PUZ_BOAT:
			case PUZ_TOP:
			case PUZ_BOTTOM:
			case PUZ_LEFT:
			case PUZ_RIGHT:
			case PUZ_MIDDLE:
				m_start.push_back(PUZ_SPACE);
				m_pos2piece[p] = ch;
				break;
			default:
				m_start.push_back(PUZ_LIGHT);
				m_pos2light[p] = ch - '0';
				break;
			};
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.insert(m_start.end(), m_sidelen, PUZ_BOUNDARY);
}

typedef map<Position, vector<tuple<int, Position, bool>>> puz_pos_match;
typedef map<int, vector<pair<Position, bool>>> puz_ship_match;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p_piece, const Position& p, int n, bool vert);
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
	map<int, int> m_ship2num;
	map<Position, char> m_pos2piece;
	map<Position, int> m_pos2light;
	puz_pos_match m_pos_matches;
	puz_ship_match m_ship_matches;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_ship2num(g.m_ship2num), m_pos2piece(g.m_pos2piece)
, m_pos2light(g.m_pos2light)
{
	for(auto& kv : g.m_pos2light)
		for(auto& os : offset){
			char& ch = cells(kv.first + os);
			if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
		}

	check_area();
	find_matches();
}

void puz_state::check_area()
{
	auto f = [](char& ch){
		if(ch == PUZ_SPACE)
			ch = PUZ_EMPTY;
	};

	for(auto& kv : m_pos2light)
		if(kv.second == 0){
			for(int r = 1; r < sidelen() - 1; ++r)
				f(cells({r, kv.first.second}));
			for(int c = 1; c < sidelen() - 1; ++c)
				f(cells({kv.first.first, c}));
		}
}

void puz_state::find_matches()
{
	m_pos_matches.clear();
	m_ship_matches.clear();
	for(const auto& kv : m_ship2num){
		int i = kv.first;
		for(int j = 0; j < 2; ++j){
			bool vert = j == 1;
			if(i == 1 && vert)
				continue;

			auto& s = ship_info[i - 1].m_pieces[j];
			int len = s.length();

			auto f = [&](Position p){
				auto os = vert ? Position(1, 0) : Position(0, 1);
				for(char ch : s){
					if(cells(p) != PUZ_SPACE)
						return false;
					p += os;
				}
				return true;
			};

			if(!m_pos2piece.empty())
				for(const auto& kv : m_pos2piece){
					const auto& p = kv.first;
					int r = p.first, c = p.second;
					if(!vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
						return kv.first.first == r && kv.second < len;
					}) || vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
						return kv.first.second == c && kv.second < len;
					}))
						continue;

					char ch = kv.second;
					for(int k = 1 - len; k <= 0; ++k){
						if(s[-k] != ch)
							continue;
						auto p2 = p + (vert ? Position(k, 0) : Position(0, k));
						if(f(p2))
							m_pos_matches[p].emplace_back(i, p2 + Position(-1, -1), vert);
					}
				}
			else
				for(int r = 1; r < sidelen() - (!vert ? 1 : len); ++r){
					if(!vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
						return kv.first.first == r && kv.second < len;
					}))
						continue;
					for(int c = 1; c < sidelen() - (vert ? 1 : len); ++c){
						if(vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
							return kv.first.second == c && kv.second < len;
						}))
							continue;
						Position p(r, c);
						if(f(p))
							m_ship_matches[i].emplace_back(p + Position(-1, -1), vert);
					}
				}
		}
	}
}

bool puz_state::make_move(const Position& p_piece, const Position& p, int n, bool vert)
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
				for(auto& kv : m_pos2light)
					if(kv.first.first == p2.first || kv.first.second == p2.second)
						--kv.second;
			}
			else if(ch == PUZ_SPACE)
				ch = PUZ_EMPTY;
		}


	if(--m_ship2num[n] == 0)
		m_ship2num.erase(n);
	m_pos2piece.erase(p_piece);
	check_area();
	find_matches();

	if(!m_pos2piece.empty())
		return boost::algorithm::all_of(m_pos2piece, [&](const pair<const Position, char>& kv){
			return m_pos_matches.count(kv.first) != 0;
		});
	else if(!is_goal_state())
		return boost::algorithm::all_of(m_ship2num, [&](const pair<int, int>& kv){
			return m_ship_matches[kv.first].size() >= kv.second;
		});
	else
		return boost::algorithm::all_of(m_pos2light, [](const pair<const Position, int>& kv){
			return kv.second == 0;
		});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_pos2piece.empty()){
		auto& kv = *boost::min_element(m_pos_matches, [](
			const puz_pos_match::value_type& kv1,
			const puz_pos_match::value_type& kv2){
			return kv1.second.size() < kv2.second.size();
		});
		for(auto& tp : m_pos_matches.at(kv.first)){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, get<1>(tp), get<0>(tp), get<2>(tp)))
				children.pop_back();
		}
	}
	else{
		auto& kv = *boost::min_element(m_ship_matches, [](
			const puz_ship_match::value_type& kv1,
			const puz_ship_match::value_type& kv2){
			return kv1.second.size() < kv2.second.size();
		});
		for(auto& kv2 : m_ship_matches.at(kv.first)){
			children.push_back(*this);
			if(!children.back().make_move(kv2.first, kv2.first, kv.first, kv2.second))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_LIGHT)
				out << format("%-2d") % m_game->m_pos2light.at(p);
			else
				out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_LightBattleships()
{
	using namespace puzzles::LightBattleships;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\LightBattleships.xml", "Puzzles\\LightBattleships.txt", solution_format::GOAL_STATE_ONLY);
}