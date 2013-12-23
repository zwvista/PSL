#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/Kropki

	Summary
	Fill the rows and columns with numbers, respecting the relations

	Description
	1. The Goal is to enter numbers 1 to board size once in every row and
	   column.
	2. A Dot between two tiles give you hints about the two numbers:
	3. Black Dot - one number is twice the other.
	4. White Dot - the numbers are consecutive.
	5. Where the numbers are 1 and 2, there can be either a Black Dot(2 is
	   1*2) or a White Dot(1 and 2 are consecutive).
	6. Please note that when two numbers are either consecutive or doubles,
	   there MUST be a Dot between them!

	Variant
	7. In later 9*9 levels you will also have bordere and coloured areas,
	   which must also contain all the numbers 1 to 9.
*/

namespace puzzles{ namespace kropki{

#define PUZ_SPACE		' '
#define PUZ_BLACK		'B'
#define PUZ_WHITE		'W'
#define PUZ_NOT_BH		'.'

struct puz_game
{
	string m_id;
	int m_sidelen;
	string m_start;
	vector<vector<Position>> m_area2range;
	vector<string> m_perms;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
	m_start = boost::accumulate(strs, string());

	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			m_area2range[r].push_back(p);
			m_area2range[m_sidelen + c].push_back(p);
		}
	}

	vector<int> perm(m_sidelen / 2 + 1);
	string perm2(m_sidelen, PUZ_SPACE);

	boost::iota(perm, 1);
	do{
		int i01 = 0;
		for(int i = m_sidelen - 1, j = i / 2;; i -= 2, --j){
			perm2[i] = perm[j] + '0';
			if(i == 0) break;
			int n1 = perm[j - 1], n2 = perm[j];
			if(n1 > n2) ::swap(n1, n2);
			perm2[i - 1] = n2 - n1 == 1 ? PUZ_WHITE :
				n2 == n1 * 2 ? PUZ_BLACK : PUZ_NOT_BH;
			if(n1 == 1 && n2 == 2)
				i01 = i - 1;
		}
		m_perms.push_back(perm2);
		if(i01 > 0){
			perm2[i01] = PUZ_BLACK;
			m_perms.push_back(perm2);
		}
	}while(boost::next_permutation(perm));
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells.at(p.first * sidelen() + p.second); }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(int i, int j);
	void make_move2(int i, int j);
	int find_matches(bool init);

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	string m_cells;
	map<int, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
	vector<int> perm_ids(g.m_perms.size());
	boost::iota(perm_ids, 0);

	for(int i = 0; i < sidelen(); i += 2)
		m_matches[i] = m_matches[sidelen() + i] = perm_ids;

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	auto& perms = m_game->m_perms;
	for(auto& kv : m_matches){
		int area_id = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(const auto& p : m_game->m_area2range[area_id])
			chars.push_back(cells(p));

		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE && ch2 != PUZ_BLACK && ch2 != PUZ_WHITE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(area_id, perm_ids.front()), 1;
			}
	}
	return 2;
}

void puz_state::make_move2(int i, int j)
{
	auto& range = m_game->m_area2range[i];
	auto& perm = m_game->m_perms[j];

	for(int k = 0; k < perm.size(); ++k)
		cells(range[k]) = perm[k];

	++m_distance;
	m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
	m_distance = 0;
	make_move2(i, j);
	for(;;)
		switch(find_matches(false)){
		case 0:
			return false;
		case 2:
			return true;
		}
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto& kv = *boost::min_element(m_matches, [](
		const pair<int, vector<int>>& kv1, 
		const pair<int, vector<int>>& kv2){
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
			char ch = cells({r, c});
			out << ch << ' ';
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_kropki()
{
	using namespace puzzles::kropki;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\kropki.xml", "Puzzles\\kropki.txt", solution_format::GOAL_STATE_ONLY);
}