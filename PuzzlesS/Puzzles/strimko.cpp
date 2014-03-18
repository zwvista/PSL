#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace strimko{

#define PUZ_SPACE		' '

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	map<Position, char> m_pos2group;
	vector<vector<Position> > m_groups;
	set<char> m_numbers;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id{attrs.get<string>("id")}
	, m_sidelen(strs.size() / 2)
{
	m_start = accumulate(strs.begin(), strs.begin() + m_sidelen, string());
	m_groups.resize(m_sidelen);
	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r + m_sidelen];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = str[c] - 'a';
			m_pos2group[p] = n;
			m_groups[n].push_back(p);
		}
	}
	for(int i = 0; i < m_sidelen; ++i)
		m_numbers.insert(i + '1');
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g) 
		: string(g.m_start), m_game(&g) {}
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const {return (*this)[p.first * sidelen() + p.second];}
	char& cells(const Position& p) {return (*this)[p.first * sidelen() + p.second];}
	int pos2group(const Position& p) const {return m_game->m_pos2group.at(p);}
	void make_move(const Position& p, char n) {cells(p) = n;}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {return boost::range::count(*this, PUZ_SPACE);}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {out << endl;}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game;
};

void puz_state::gen_children(list<puz_state>& children) const
{
	int sp = find(PUZ_SPACE);
	int r = sp / sidelen(), c = sp % sidelen();
	Position p(r, c);
	set<char> numbers = m_game->m_numbers;
	for(int c2 = 0; c2 < sidelen(); ++c2)
		numbers.erase(cells({r, c2}));
	for(int r2 = 0; r2 < sidelen(); ++r2)
		numbers.erase(cells({r2, c}));
	const vector<Position>& g = m_game->m_groups[pos2group(p)];
	for(int i = 0; i < sidelen(); ++i)
		numbers.erase(cells(g[i]));
	for(char n : numbers){
		children.push_back(*this);
		children.back().make_move(p, n);
	}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << cells({r, c});
		out << endl;
	}
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c)
			out << char(pos2group(Position(r, c)) + 'a');
		out << endl;
	}
	return out;
}

}}

void solve_puz_strimko()
{
	using namespace puzzles::strimko;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\strimko.xml", "Puzzles\\strimko.txt", solution_format::GOAL_STATE_ONLY);
}