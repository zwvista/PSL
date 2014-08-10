#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Puzzle Retreat
*/

namespace puzzles{ namespace PuzzleRetreat{

#define PUZ_HOLE		' '
#define PUZ_BLOCK_STOP	'S'
#define PUZ_BOUNDARY	'#'

#define PUZ_BLOCK_ICE	'N'
#define PUZ_ICE			'o'
#define PUZ_STOP		's'
#define PUZ_ARROW		'A'
#define PUZ_USED		'*'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};
const string arrows = "^>v<";
const string dirs = "urdl";

enum class puz_block_type
{
	ICE,
	STOP,
	FIRE,
};
struct puz_block
{
	puz_block_type m_type;
	int m_num;
	puz_block(){}
	puz_block(puz_block_type t, int n)
		: m_type(t), m_num(n) {}
};

struct puz_game
{
	string m_id;
	Position m_size;
	string m_start;
	map<Position, puz_block> m_pos2block;
	map<Position, int> m_pos2dir;

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
			Position p(r, c);
			switch(char ch = str[c - 1]){
			case PUZ_HOLE:
			case PUZ_BOUNDARY:
				m_start.push_back(ch);
				break;
			case PUZ_BLOCK_STOP:
				m_start.push_back(ch);
				m_pos2block[p] = {puz_block_type::STOP, 1};
				break;
			default:
				if(isdigit(ch)){
					m_pos2block[p] = {puz_block_type::ICE, ch - '0'};
					m_start.push_back(PUZ_BLOCK_ICE);
				}
				else{
					int n = arrows.find(ch);
					if(n != -1){
						m_pos2dir[p] = n;
						m_start.push_back(PUZ_ARROW);
					}
				}
			}
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
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, Position os, bool is_test);

	//solve_puzzle interface
	bool is_goal_state() const {
		return get_heuristic() == 0 && boost::count(*this, PUZ_HOLE) == 0;
	}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_pos2block.size(); }
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const { if(m_move) out << *m_move; }
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}
	
	const puz_game* m_game = nullptr;
	boost::optional<puz_step> m_move;
	map<Position, puz_block> m_pos2block;
};

puz_state::puz_state(const puz_game& g)
	: string(g.m_start), m_game(&g), m_pos2block(g.m_pos2block)
{
}

bool puz_state::make_move(const Position& p, int n)
{
	if(!make_move2(p, offset[n], false))
		return false;

	m_move = puz_step(p, n);
	m_pos2block.erase(p);

	return boost::algorithm::all_of(m_pos2block, [&](const pair<const Position, puz_block>& kv){
		return boost::algorithm::any_of(offset, [&](const Position& os){
			return make_move2(kv.first, os, true);
		});
	});
}

bool puz_state::make_move2(const Position& p, Position os, bool is_test)
{
	auto& blk = m_pos2block.at(p);
	int n = blk.m_num;
	if(!is_test)
		cells(p) = PUZ_USED;
	set<Position> p_arrows;
	for(auto p2 = p + os;; p2 += os)
		switch(char& ch = cells(p2)){
		case PUZ_ARROW:
			if(p_arrows.count(p2) != 0)
				return false;
			os = offset[m_game->m_pos2dir.at(p2)];
			p_arrows.insert(p2);
			break;
		case PUZ_HOLE:
			if(!is_test)
				ch = blk.m_type == puz_block_type::ICE ? PUZ_ICE :
					PUZ_STOP;
			if(--n == 0)
				return true;
			break;
		case PUZ_ICE:
			break;
		default:
			return false;
	}

	// should not reach here
	return false;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	for(auto& kv : m_pos2block)
		for(int n = 0; n < 4; ++n){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, n))
				children.pop_back();
		}
}

ostream& puz_state::dump(ostream& out) const
{
	dump_move(out);
	for(int r = 1; r < rows() - 1; ++r){
		for(int c = 1; c < cols() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			out << (ch == PUZ_BLOCK_ICE ? char(m_game->m_pos2block.at(p).m_num + '0') :
				ch == PUZ_ARROW ? arrows[m_game->m_pos2dir.at(p)] :
				ch == PUZ_HOLE ? '.' : ch) << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_PuzzleRetreat()
{
	using namespace puzzles::PuzzleRetreat;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\PuzzleRetreat_Welcome.xml", "Puzzles\\PuzzleRetreat_Welcome.txt");
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\PuzzleRetreat_Morning.xml", "Puzzles\\PuzzleRetreat_Morning.txt");
}