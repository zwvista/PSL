#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Puzzle Retreat
*/

namespace puzzles{ namespace PuzzleRetreat{

#define PUZ_SPACE		' '
#define PUZ_ICE			'o'
#define PUZ_BOUNDARY	'#'
#define PUZ_USED		'*'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

enum class puz_block_type
{
	ICE,
};
struct puz_block
{
	puz_block_type m_type;
	int m_num;
	Position m_pos;
	puz_block(puz_block_type t, int n, const Position& p)
		: m_type(t), m_num(n), m_pos(p) {}
};

struct puz_game
{
	string m_id;
	Position m_size;
	string m_start;
	vector<puz_block> m_blocks;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
	int rows() const { return m_size.first; }
	int cols() const { return m_size.second; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_size(strs.size() + 2, strs[0].length() + 2)
{
	m_start.append(string(cols(), PUZ_BOUNDARY));
	for(int r = 1; r < rows() - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < cols() - 1; ++c){
			char ch = str[c - 1];
			m_start.push_back(ch);
			if(ch == PUZ_SPACE) continue;
			Position p(r, c);
			if(isdigit(ch))
				m_blocks.emplace_back(puz_block_type::ICE, ch - '0', p);
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(cols(), PUZ_BOUNDARY));
}

struct puz_step : pair<Position, int>
{
	puz_step(const Position& p, int n) : pair<Position, int>(p, n) {}
};

ostream& operator<<(ostream &out, const puz_step &mi)
{
	static char* dirs = "urdl";
	out << format("move: %1% %2%\n") % mi.first % dirs[mi.second];
	return out;
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int rows() const { return m_game->rows(); }
	int cols() const { return m_game->cols(); }
	char cells(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * cols() + p.second]; }
	bool make_move(int i, int j);
	bool make_move2(const puz_block& blk, Position os, bool is_test);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_blocks.size();
	}
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const { if(m_move) out << *m_move; }
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}
	
	const puz_game* m_game = nullptr;
	boost::optional<puz_step> m_move;
	vector<puz_block> m_blocks;
};

puz_state::puz_state(const puz_game& g)
	: string(g.m_start), m_game(&g), m_blocks(g.m_blocks)
{
}

bool puz_state::make_move(int i, int j)
{
	auto it = m_blocks.begin() + i;
	if(!make_move2(*it, offset[j], false))
		return false;

	m_move = puz_step(it->m_pos, j);
	m_blocks.erase(it);

	return boost::algorithm::all_of(m_blocks, [&](const puz_block& blk){
		return boost::algorithm::any_of(offset, [&](const Position& os){
			return make_move2(blk, os, true);
		});
	});
}

bool puz_state::make_move2(const puz_block& blk, Position os, bool is_test)
{
	int n = blk.m_num;
	if(!is_test)
		cells(blk.m_pos) = PUZ_USED;
	for(auto p = blk.m_pos + os;; p += os)
		switch(char& ch = cells(p)){
		case PUZ_BOUNDARY:
			return false;
		case PUZ_SPACE:
			if(!is_test)
				ch = PUZ_ICE;
			if(--n == 0)
				return true;
		}

	// should not reach here
	return false;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(int i = 0; i < m_blocks.size(); ++i)
		for(int j = 0; j < 4; ++j){
			children.push_back(*this);
			if(!children.back().make_move(i, j))
				children.pop_back();
		}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 1; r < rows() - 1; ++r){
		for(int c = 1; c < cols() - 1; ++c)
			out << cells({r, c}) << " ";
		out << endl;
	}
	return out;
}

}}

void solve_puz_PuzzleRetreat()
{
	using namespace puzzles::PuzzleRetreat;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\PuzzleRetreat.xml", "Puzzles\\PuzzleRetreat.txt");
}