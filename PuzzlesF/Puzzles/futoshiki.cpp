#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Abc

	Summary
	Fill the rows and columns with numbers, respecting the relations

	Description
	1. You have to put in each row and column numbers ranging from 1 to N,
	   where N is the puzzle board size.
	2. The hints you have are the less than/greater than signs between tiles.
	3. Remember you can't repeat the same number in a row or column.

	Variation
	4. Some boards, instead of having less/greater signs, have just a line
	   separating the tiles.
	5. That separator hints at two tiles with consecutive numbers.
	6. Please note that in this variation consecutive numbers MUST have a
	   line separating the tiles. Otherwise they're not consecutive.
*/

namespace puzzles{ namespace futoshiki{

#define PUZ_SPACE		' '
#define PUZ_ROW_LT		'<'
#define PUZ_ROW_GT		'>'
#define PUZ_ROW_CS		'|'
#define PUZ_COL_LT		'^'
#define PUZ_COL_GT		'v'
#define PUZ_COL_CS		'-'
#define PUZ_NOT_CS		'.'

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	vector<vector<Position>> m_area_pos;
	vector<string> m_perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area_pos(m_sidelen * 2)
{
	m_start = boost::accumulate(strs, string());
	boost::replace(m_start, PUZ_COL_LT, PUZ_ROW_LT);
	boost::replace(m_start, PUZ_COL_GT, PUZ_ROW_GT);
	boost::replace(m_start, PUZ_COL_CS, PUZ_ROW_CS);
	bool ltgt_mode = m_start.find(PUZ_ROW_CS) == -1;

	for(int r = 0; r < m_sidelen; ++r){
		const string& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area_pos[r].push_back(p);
			m_area_pos[m_sidelen + c].push_back(p);
		}
	}

	string perm(m_sidelen / 2 + 1, PUZ_SPACE);
	string perm2(m_sidelen, PUZ_SPACE);

	boost::iota(perm, '1');
	do{
		for(int i = m_sidelen - 1, j = i / 2;; i -= 2, --j){
			perm2[i] = perm[j];
			if(i == 0) break;
			perm2[i - 1] = ltgt_mode ? 
				perm[j - 1] < perm[j] ? PUZ_ROW_LT : PUZ_ROW_GT :
				myabs(perm[j - 1] - perm[j]) == 1 ? PUZ_ROW_CS : PUZ_NOT_CS;
		}
		m_perms.push_back(perm2);
	}while(boost::next_permutation(perm));
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cell(const Position& p) const { return at(p.first * sidelen() + p.second); }
	char& cell(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i, int j);
	void find_matches();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { 
		return boost::count(*this, PUZ_SPACE) - (sidelen() / 2) * (sidelen() / 2);
	}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(int i = 0; i < sidelen(); i += 2)
		m_matches[i], m_matches[sidelen() + i];

	find_matches();
}

void puz_state::find_matches()
{
	vector<int> filled;

	for(auto& kv : m_matches){
		string area;
		for(const auto& p : m_game->m_area_pos.at(kv.first))
			area.push_back(cell(p));
		if(boost::algorithm::none_of(area, [](char ch){return ch == PUZ_SPACE;})){
			filled.push_back(kv.first);
			continue;
		}

		kv.second.clear();
		for(int i = 0; i < m_game->m_perms.size(); i++)
			if(boost::equal(area, m_game->m_perms.at(i), [](char ch1, char ch2){
				return ch1 == PUZ_SPACE && ch2 != PUZ_ROW_CS || ch1 == ch2; }))
				kv.second.push_back(i);
	}

	for(int n : filled)
		m_matches.erase(n);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	const auto& area = m_game->m_area_pos.at(i);
	const auto& perm = m_game->m_perms.at(j);

	for(int k = 0; k < sidelen(); ++k){
		char& ch = cell(area[k]);
		if(ch == PUZ_SPACE)
			ch = perm[k], ++m_distance;
	}

	find_matches();
	return boost::algorithm::none_of(m_matches, [](const pair<const int, vector<int>>& kv){
		return kv.second.empty();
	});
}

void puz_state::gen_children(list<puz_state> &children) const
{
	const auto& kv = *boost::min_element(m_matches, [](const pair<const int, vector<int>>& kv1, const pair<const int, vector<int>>& kv2){
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
	for(int r = 0; r < sidelen(); ++r) {
		for(int c = 0; c < sidelen(); ++c){
			char ch = cell(Position(r, c));
			if(r % 2 == 1)
				ch = ch == PUZ_ROW_LT ? PUZ_COL_LT :
					ch == PUZ_ROW_GT ? PUZ_COL_GT :
					ch == PUZ_ROW_CS ? PUZ_COL_CS : ch;
			out << ch << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_futoshiki()
{
	using namespace puzzles::futoshiki;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\futoshiki.xml", "Puzzles\\futoshiki.txt", solution_format::GOAL_STATE_ONLY);
}