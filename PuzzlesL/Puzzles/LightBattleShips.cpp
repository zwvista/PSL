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
	bool m_has_supertank;
	vector<int> m_ships;
	map<Position, char> m_pos2piece;
	map<Position, int> m_pos2light;
	string m_start;

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

	int n = 0;
	for(int r = 0; r <= m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c <= m_sidelen; ++c){
			Position p(r, c);
			switch(char ch = str[c]){
			case PUZ_SPACE:
			case PUZ_EMPTY:
				if(r != m_sidelen || c != m_sidelen)
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
				n = ch - '0';
				m_pos2light[p] = n;
				break;
			};
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
	vector<int> m_ships;
	map<Position, char> m_pos2piece;
	map<Position, int> m_pos2light;
	map<int, vector<pair<Position, bool>>> m_matches;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_ships(g.m_ships), m_pos2piece(g.m_pos2piece)
, m_pos2light(g.m_pos2light)
{
	for(auto& kv : g.m_pos2light)
		for(auto& os : offset){
			auto p = kv.first + os;
			if(!is_valid(p))
				continue;
			char& ch = cells(p);
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
			for(int r = 0; r < sidelen(); ++r)
				f(cells({r, kv.first.second}));
			for(int c = 0; c < sidelen(); ++c)
				f(cells({kv.first.first, c}));
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

			auto f = [&](Position p){
				auto os = vert ? Position(1, 0) : Position(0, 1);
				for(char ch : s){
					if(!is_valid(p) || cells(p) != PUZ_SPACE)
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
							m_matches[i].push_back({p2, vert});
					}
				}
			else
				for(int r = 0; r < sidelen() - (!vert ? 0 : len - 1); ++r){
					if(!vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
						return kv.first.first == r && kv.second < len;
					}))
						continue;
					for(int c = 0; c < sidelen() - (vert ? 0 : len - 1); ++c){
						if(vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv){
							return kv.first.second == c && kv.second < len;
						}))
							continue;
						Position p(r, c);
						if(f(p))
							m_matches[i].push_back({p, vert});
					}
				}
		}
}

bool puz_state::make_move(const Position& p, int n, bool vert)
{
	auto os = vert ? Position(1, 0) : Position(0, 1);
	const auto& s = ship_pieces[n - 1][vert ? 1 : 0];
	int len = s.length();

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
		for(auto& kv : m_pos2light){
			if(kv.first.first == p2.first)
				kv.second -= !vert ? len : 1;
			if(kv.first.second == p2.second)
				kv.second -= vert ? len : 1;
		}

		m_pos2piece.erase(p2);
		p2 += os;
	}

	m_ships.erase(boost::range::find(m_ships, n));
	check_area();
	find_matches();
	return !m_pos2piece.empty() && !m_matches.empty() || m_pos2piece.empty() && (
		!is_goal_state() && boost::algorithm::all_of(m_ships, [&](int i){
			return m_matches.count(i) != 0;
		}) || is_goal_state() && boost::algorithm::all_of(m_pos2light, [](const pair<const Position, int>& kv){
			return kv.second == 0;
		})
	);
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto f = [&](int n){
		for(auto& kv : m_matches.at(n)){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, n, kv.second))
				children.pop_back();
		}
	};

	if(!m_pos2piece.empty())
		if(m_matches.count(1) != 0)
			f(1);
		else
			for(auto& kv : m_matches)
				f(kv.first);
	else{
		auto& kv = *boost::min_element(m_matches, [](
			const pair<int, vector<pair<Position, bool>>>& kv1,
			const pair<int, vector<pair<Position, bool>>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});
		f(kv.first);
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_EMPTY || ch == PUZ_SPACE)
				out << PUZ_EMPTY << ' ';
			else
				out << ch << m_game->m_pos2light.at(p);
			out << ' ';
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
