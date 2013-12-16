#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 6/Slanted Maze

	Summary
	Maze of Slants!

	Description
	1. Fill the board with diagonal lines (Slants), following the hints at
	   the intersections.
	2. Every number tells you how many Slants (diagonal lines) touch that
	   point. So, for example, a 4 designates an X pattern around it.
	3. The Mazes or paths the Slants will form will usually branch off many
	   times, but can also end abruptly. Also all the Slants don't need to
	   be all connected.
	4. However, you must ensure that they don't form a closed loop anywhere.
	   This also means very big loops, not just 2*2.
*/

namespace puzzles{ namespace slantedmaze{

#define PUZ_UNKNOWN		5
#define PUZ_SPACE		' '
#define PUZ_SLASH		'/'
#define PUZ_BACKSLASH	'\\'
#define PUZ_BOUNDARY	'B'

const Position offset[] = {
	{0, 0},	// nw
	{0, 1},	// ne
	{1, 1},	// se
	{1, 0},	// sw
};

const string slants = "\\/\\/";

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	vector<vector<string>> m_num2disps;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 1)
, m_num2disps(PUZ_UNKNOWN + 1)
{
	for(int r = 0; r < m_sidelen - 1; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 1; ++c){
			Position p(r, c);
			char ch = str[c];
			m_pos2num[p] = ch == ' ' ? PUZ_UNKNOWN : ch - '0';
		}
	}

	auto disp = slants;
	auto& disps_unknown = m_num2disps[PUZ_UNKNOWN];
	for(int i = 0; i <= 4; ++i){
		auto& disps = m_num2disps[i];
		vector<int> indexes(4, 0);
		fill(indexes.begin() + 4 - i, indexes.end(), 1);
		do{
			for(int i = 0; i < 4; ++i){
				int index = indexes[i];
				char ch = slants[i];
				disp[i] = index == 1 ? ch :
						ch == PUZ_SLASH ? PUZ_BACKSLASH :
						PUZ_SLASH;
			}
			disps.push_back(disp);
			disps_unknown.push_back(disp);
		}while(boost::next_permutation(indexes));
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
	bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
	bool make_move(const Position& p, int n);
	bool make_move2(const Position& p, int n);
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
	map<Position, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(sidelen() * sidelen(), PUZ_SPACE)
{
	for(int i = 0; i < sidelen(); ++i)
		cells({i, 0}) = cells({i, sidelen() - 1}) =
		cells({0, i}) = cells({sidelen() - 1, i}) =
		PUZ_BOUNDARY;

	for(auto& kv : g.m_pos2num){
		auto& disp_ids = m_matches[kv.first];
		disp_ids.resize(g.m_num2disps[kv.second].size());
		boost::iota(disp_ids, 0);
	}

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		auto& p = kv.first;
		auto& disp_ids = kv.second;

		string chars;
		for(auto& os : offset)
			chars.push_back(cells(p + os));

		auto& disps = m_game->m_num2disps[m_game->m_pos2num.at(p)];
		boost::remove_erase_if(disp_ids, [&](int id){
			return !boost::equal(chars, disps[id], [](char ch1, char ch2){
				return ch1 == PUZ_SPACE || ch1 == PUZ_BOUNDARY || ch1 == ch2;
			});
		});

		if(!init)
			switch(disp_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, disp_ids.front()) ? 1 : 0;
			}
	}
	return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
	auto& disp = m_game->m_num2disps[m_game->m_pos2num.at(p)][n];
	for(int k = 0; k < disp.size(); ++k){
		char& ch = cells(p + offset[k]);
		if(ch == PUZ_SPACE)
			ch = disp[k];
	}

	++m_distance;
	m_matches.erase(p);

	// TODO: check the no-closed-loop rule
	return true;
}

bool puz_state::make_move(const Position& p, int n)
{
	m_distance = 0;
	make_move2(p, n);
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
		const pair<const Position, vector<int>>& kv1,
		const pair<const Position, vector<int>>& kv2){
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
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c)
			out << cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_slantedmaze()
{
	using namespace puzzles::slantedmaze;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\slantedmaze.xml", "Puzzles\\slantedmaze.txt", solution_format::GOAL_STATE_ONLY);
}