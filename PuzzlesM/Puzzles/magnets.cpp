#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 2/Magnets

	Summary
	Place Magnets on the board, respecting the orientation of poles

	Description
	Each Magnet has a positive(+) and a negative(-) pole.
	1. Every rectangle can either contain a Magnet or be empty.
	2. The numbers on the board tells you how many positive and negative poles
	   you can see from there in a straight line.
	3. When placing a Magnet, you have to respect the rule that the same pole
	   (+ and + / - and -)can't be adjacent horizontally or vertically.
*/

namespace puzzles{ namespace magnets{

#define PUZ_HORZ		'H'
#define PUZ_VERT		'V'
#define PUZ_SPACE		' '
#define PUZ_POSITIVE	'+'
#define PUZ_NEGATIVE	'-'
#define PUZ_EMPTY		'.'

const Position offset[] = {
	{0, 1},
	{1, 0},
	{0, -1},
	{-1, 0},
};

typedef set<char> puz_chars;

struct puz_game
{
	string m_id;
	int m_sidelen;
	int m_num_recs;
	vector<set<Position>> m_area_pos;
	map<Position, int> m_pos2rec;
	vector<array<int, 2>> m_num_poles_rows, m_num_poles_cols;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size() - 2)
	, m_num_recs(m_sidelen * m_sidelen / 2)
	, m_area_pos(m_num_recs + m_sidelen + m_sidelen)
	, m_num_poles_rows(m_sidelen)
	, m_num_poles_cols(m_sidelen)
{
	m_start = boost::accumulate(strs, string());
	for(int r = 0, n = 0; r < m_sidelen + 2; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen + 2; ++c){
			Position p(r, c);
			switch(char ch = str[c]){
			case PUZ_SPACE:
			case PUZ_POSITIVE:
			case PUZ_NEGATIVE:
				break;
			case PUZ_HORZ:
			case PUZ_VERT:
				for(int d = 0; d < 2; ++d){
					Position p = 
						ch == PUZ_HORZ ? Position(r, c + d)
						: Position(r + d, c);
					m_pos2rec[p] = n;
					m_area_pos[n].insert(p);
					m_area_pos[m_num_recs + p.first].insert(p);
					m_area_pos[m_num_recs + m_sidelen + p.second].insert(p);
				}
				++n;
				break;
			default:
				if(c >= m_sidelen)
					m_num_poles_rows[r][c - m_sidelen] = ch - '0';
				else if(r >= m_sidelen)
					m_num_poles_cols[c][r - m_sidelen] = ch - '0';
				break;
			}
		}
	}
}

// second.key : all chars used to fill a position
// second.value : the number of remaining times that the key char can be used in the area
struct puz_area : pair<int, map<char, int>>
{
	puz_area() {}
	puz_area(int index, int num_positive, int num_negative, int num_empty)
	: pair<int, map<char, int>>(index, map<char, int>()){
		second.emplace(PUZ_POSITIVE, num_positive);
		second.emplace(PUZ_NEGATIVE, num_negative);
		second.emplace(PUZ_EMPTY, num_empty);
	}
	void fill_cell(const Position& p, char ch){ --second.at(ch); }
	int cant_use(char ch) const { return second.at(ch) == 0; }
};

using puz_group = vector<puz_area>;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(const Position& p, char ch);
	bool make_move2(const Position& p, char ch);
	void check_area(const puz_area& a, char ch){
		if(a.cant_use(ch))
			for(const auto& p : m_game->m_area_pos[a.first])
				remove_pair(p, ch);
	}
	void remove_pair(const Position& p, char ch){
		auto i = m_pos2chars.find(p);
		if(i != m_pos2chars.end())
			i->second.erase(ch);
	}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {return boost::count(*this, PUZ_SPACE);}
	unsigned int get_distance(const puz_state& child) const {return 2;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
	puz_group m_grp_rows;
	puz_group m_grp_cols;
	map<Position, puz_chars> m_pos2chars;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
	puz_chars all_chars = { PUZ_POSITIVE, PUZ_NEGATIVE, PUZ_EMPTY };
	for(int r = 0; r < g.m_sidelen; ++r)
		for(int c = 0; c < g.m_sidelen; ++c)
			m_pos2chars[Position(r, c)] = all_chars;

	for(int i = 0; i < g.m_sidelen; i++){
		const auto& np = g.m_num_poles_rows[i];
		m_grp_rows.emplace_back(i + g.m_num_recs, np[0], np[1], g.m_sidelen - np[0] - np[1]);
		for(char ch : all_chars)
			check_area(m_grp_rows.back(), ch);
	}
	for(int i = 0; i < g.m_sidelen; i++){
		const auto& np = g.m_num_poles_cols[i];
		m_grp_cols.emplace_back(i + g.m_num_recs + g.m_sidelen, np[0], np[1], g.m_sidelen - np[0] - np[1]);
		for(char ch : all_chars)
			check_area(m_grp_cols.back(), ch);
	}
}

bool puz_state::make_move(const Position& p, char ch)
{
	if(!make_move2(p, ch))
		return false;

	auto ps = m_game->m_area_pos.at(m_game->m_pos2rec.at(p));
	ps.erase(p);
	const auto& p2 = *ps.cbegin();

	char ch2 =
		ch == PUZ_EMPTY ? PUZ_EMPTY :
		ch == PUZ_POSITIVE ? PUZ_NEGATIVE :
		PUZ_POSITIVE;
	return make_move2(p2, ch2);
}

bool puz_state::make_move2(const Position& p, char ch)
{
	cell(p) = ch;
	m_pos2chars.erase(p);

	for(auto a : {&m_grp_rows[p.first], &m_grp_cols[p.second]}){
		a->fill_cell(p, ch);
		check_area(*a, ch);
	}

	// respect the rule of poles
	if(ch != PUZ_EMPTY)
		for(auto& os : offset){
			const auto& p2 = p + os;
			if(is_valid(p2))
				remove_pair(p2, ch);
		}

	return boost::algorithm::none_of(m_pos2chars, [](const pair<const Position, puz_chars>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(m_pos2chars,
		[](const pair<const Position, puz_chars>& kv1, const pair<const Position, puz_chars>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	const auto& p = kv.first;
	for(char ch : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(p, ch))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 0; r < sidelen() + 2; ++r) {
		for(int c = 0; c < sidelen() + 2; ++c)
			out << (r < sidelen() && c < sidelen() ?
				cell(Position(r, c)) :
				m_game->m_start[r * (sidelen() + 2) + c]);
		out << endl;
	}
	for(int r = 0; r < sidelen() + 2; ++r) {
		for(int c = 0; c < sidelen() + 2; ++c)
			out << (r < sidelen() && c < sidelen() ? 
				char(m_game->m_pos2rec.at(Position(r, c)) + 'a') :
				m_game->m_start[r * (sidelen() + 2) + c]);
		out << endl;
	}
	return out;
}

}}

void solve_puz_magnets()
{
	using namespace puzzles::magnets;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
		("Puzzles\\magnets.xml", "Puzzles\\magnets.txt", solution_format::GOAL_STATE_ONLY);
}