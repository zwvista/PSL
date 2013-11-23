#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace sokoban{

#define PUZ_MAN			'@'
#define PUZ_MAN_GOAL	'+'
#define PUZ_FLOOR		' '
#define PUZ_GOAL		'.'
#define PUZ_BOX			'$'
#define PUZ_BOX_GOAL	'*'

const Position offset[] = {
	{0, -1},
	{0, 1},
	{-1, 0},
	{1, 0},
};

typedef unordered_map<char, vector<int> > group_map;

struct puz_game
{
	string m_id;
	Position m_size;
	string m_cells;
	Position m_man;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int rows() const {return m_size.first;}
	int cols() const {return m_size.second;}
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_size(strs.size(), strs[0].length())
{
	m_cells = boost::accumulate(strs, string());
	size_t i = m_cells.find(PUZ_MAN);
	m_man = Position(i / cols(), i % cols());
	m_cells[i] = ' ';
}

struct puz_state_base
{
	int rows() const {return m_game->rows();}
	int cols() const {return m_game->cols();}

	const puz_game* m_game;
	Position m_man;
	string m_move;
};

struct puz_state2;

struct puz_state : puz_state_base
{
	puz_state() {}
	puz_state(const puz_game& g) : m_cells(g.m_cells){
		m_game = &g, m_man = g.m_man;
	}
	puz_state(const puz_state2& x2);
	char cell(const Position& p) const {return m_cells.at(p.first * cols() + p.second);}
	char& cell(const Position& p) {return m_cells[p.first * cols() + p.second];}
	bool operator<(const puz_state& x) const {
		return m_cells < x.m_cells || m_cells == x.m_cells && m_man < x.m_man ||
			m_cells == x.m_cells && m_man == x.m_man && m_move < x.m_move;
	}
	void make_move(const Position& p1, const Position& p2, char dir){
		cell(p1) = cell(p1) == PUZ_BOX ? PUZ_FLOOR : PUZ_GOAL;
		cell(p2) = cell(p2) == PUZ_FLOOR ? PUZ_BOX : PUZ_BOX_GOAL;
		m_man = p1;
		m_move += dir;
	}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const;
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {if(!m_move.empty()) out << m_move;}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	string m_cells;
};

struct puz_state2 : puz_state_base
{
	puz_state2(const puz_state& s) : m_cells(s.m_cells){
		m_game = s.m_game, m_man = s.m_man;
	}
	char cell(const Position& p) const {return m_cells.at(p.first * cols() + p.second);}
	bool operator<(const puz_state2& x) const {
		return m_man < x.m_man;
	}
	void make_move(const Position& p, char dir){
		m_man = p, m_move += dir;
	}
	void gen_children(list<puz_state2>& children) const;

	const string& m_cells;
};

void puz_state2::gen_children(list<puz_state2> &children) const
{
	static char* dirs = "lrud";
	for(int i = 0; i < 4; ++i){
		Position p = m_man + offset[i];
		char ch = cell(p);
		if(ch == PUZ_FLOOR || ch == PUZ_GOAL){
			children.push_back(*this);
			children.back().make_move(p, dirs[i]);
		}
	}
}

puz_state::puz_state(const puz_state2& x2)
	: m_cells(x2.m_cells)
{
	m_game = x2.m_game, m_man = x2.m_man, m_move = x2.m_move;
}

void puz_state::gen_children(list<puz_state> &children) const
{
	static char* dirs = "LRUD";
	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	for(const puz_state2& s : smoves)
		for(int i = 0; i < 4; ++i){
			Position p1 = s.m_man + offset[i];
			char ch = cell(p1);
			if(ch != PUZ_BOX && ch != PUZ_BOX_GOAL) continue;
			Position p2 = p1 + offset[i];
			ch = cell(p2);
			if(ch != PUZ_FLOOR && ch != PUZ_GOAL) continue;
			children.push_back(s);
			children.back().make_move(p1, p2, dirs[i]);
		}
}

unsigned int puz_state::get_heuristic() const
{
	unsigned int md = 0;
	int i = -1, j = -1;
	for(;;){
		i = m_cells.find(PUZ_BOX, i + 1);
		if(i == -1) break;
		j = m_cells.find(PUZ_GOAL, j + 1);
		md += myabs(i / cols() - j / cols()) + myabs(i % cols() - j % cols());
	}
	return md;
}

ostream& puz_state::dump(ostream& out) const
{
	if(!m_move.empty())
		out << "move: " << m_move << endl;
	for(int r = 0; r < rows(); ++r) {
		for(int c = 0; c < cols(); ++c){
			Position p(r, c);
			char ch = cell(Position(r, c));
			out << (p == m_man ? 
				ch == PUZ_FLOOR ? PUZ_MAN : PUZ_MAN_GOAL : ch);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_sokoban()
{
	using namespace puzzles::sokoban;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\sokoban.xml", "Puzzles\\sokoban.txt");
}