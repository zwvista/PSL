#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 3/SlitherLink

	Summary
	Draw a loop a-la-minesweeper!

	Description
	1. Draw a single looping path with the aid of the numbered hints. The path
	   cannot have branches or cross itself.
	2. Each number in a tile tells you on how many of its four sides are touched
	   by the path.
	3. For example:
	4. A 0 tells you that the path doesn't touch that square at all.
	5. A 1 tells you that the path touches that square ONLY one-side.
	6. A 3 tells you that the path does a U-turn around that square.
	7. There can't be tiles marked with 4 because that would form a single
	   closed loop in it.
	8. Empty tiles can have any number of sides touched by that path.
*/

namespace puzzles{ namespace SlitherLink{

#define PUZ_SPACE			' '
#define PUZ_LINE_UNKNOWN	'0'
#define PUZ_LINE_OFF		'1'
#define PUZ_LINE_ON			'2'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const Position line_offset[] = {
	{0, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, 0},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2num;
	string m_cells;
	vector<vector<string>> m_num2disps;

	char cells(const Position& p) const { return m_cells.at(p.first * m_sidelen + p.second); }
	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
, m_num2disps(4)
{
	m_cells = boost::accumulate(strs, string());
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_pos2num[{r, c}] = ch - '0';
		}
	}
	for(int i = 0; i < 4; ++i){
		auto& disps = m_num2disps[i];
		auto disp = string(4 - i, PUZ_LINE_OFF) + string(i, PUZ_LINE_ON);
		do 
			disps.push_back(disp);
		while(boost::next_permutation(disp));
	}
}

struct puz_state
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char get_line_status(const Position& p, int i) const {
		auto& lines = i % 2 == 0 ? m_horz_lines : m_vert_lines;
		return lines.at(p + line_offset[i]);
	}
	void set_line_status(const Position& p, int i, char status){
		auto& lines = i % 2 == 0 ? m_horz_lines : m_vert_lines;
		lines.at(p + line_offset[i]) = status;
	}
	bool operator<(const puz_state& x) const { 
		return std::tie(m_horz_lines, m_vert_lines) < std::tie(x.m_horz_lines, x.m_vert_lines);
	}
	bool make_move(const Position& p, const vector<int>& disp);
	bool make_move2(const Position& p, const vector<int>& disp);
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
	unsigned int m_distance = 0;
	map<Position, vector<vector<int>>> m_matches;
	map<Position, char> m_horz_lines, m_vert_lines;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
{
	for(int r = 0; r <= sidelen(); ++r)
		for(int c = 0; c <= sidelen(); ++c){
			if(c < sidelen())
				m_horz_lines[{r, c}] = PUZ_LINE_UNKNOWN;
			if(r < sidelen())
				m_vert_lines[{r, c}] = PUZ_LINE_UNKNOWN;
		}

	for(const auto& kv : g.m_pos2num)
		m_matches[kv.first];

	find_matches(true);
}

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& disps = kv.second;
		disps.clear();

		int sum = m_game->m_pos2num.at(p);
		vector<vector<int>> dir_nums(4);
		for(int i = 0; i < 4; ++i){
			auto& os = offset[i];
			int n = 0;
			auto& nums = dir_nums[i];
			[&](){
				for(auto p2 = p; n <= sum; p2 += os)
					switch(get_line_status(p2, i)){
					case PUZ_LINE_UNKNOWN:
						nums.push_back(n++);
						break;
					case PUZ_LINE_OFF:
						++n;
						break;
					case PUZ_LINE_ON:
						nums.push_back(n);
						return;
					}
			}();
		}

		for(int n0 : dir_nums[0])
			for(int n1 : dir_nums[1])
				for(int n2 : dir_nums[2])
					for(int n3 : dir_nums[3])
						if(n0 + n1 + n2 + n3 == sum)
							disps.push_back({n0, n1, n2, n3});

		if(!init)
			switch(disps.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(p, disps.front()) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
	make_move({});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(int i = 0; i < 4; ++i)
		if(m_state->get_line_status(*this, i) != PUZ_LINE_ON){
			children.push_back(*this);
			children.back().make_move(*this + offset[i]);
		}
}

bool puz_state::make_move2(const Position& p, const vector<int>& disp)
{
	for(int i = 0; i < 4; ++i){
		auto& os = offset[i];
		int n = disp[i];
		auto p2 = p;
		for(int j = 0; j < n; ++j){
			set_line_status(p2, i, PUZ_LINE_OFF);
			p2 += os;
		}
		set_line_status(p2, i, PUZ_LINE_ON);
	}

	++m_distance;
	m_matches.erase(p);

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	return smoves.size() == sidelen() * sidelen();
}

bool puz_state::make_move(const Position& p, const vector<int>& disp)
{
	m_distance = 0;
	if(!make_move2(p, disp))
		return false;
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
		const pair<const Position, vector<vector<int>>>& kv1,
		const pair<const Position, vector<vector<int>>>& kv2){
		return kv1.second.size() < kv2.second.size();
	});

	for(const auto& disp : kv.second){
		children.push_back(*this);
		if(!children.back().make_move(kv.first, disp))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0;; ++r){
		// draw horz-lines
		for(int c = 0; c < sidelen(); ++c)
			out << (m_horz_lines.at({r, c}) == PUZ_LINE_ON ? " -" : "  ");
		out << endl;
		if(r == sidelen()) break;
		for(int c = 0;; ++c){
			Position p(r, c);
			// draw vert-lines
			out << (m_vert_lines.at(p) == PUZ_LINE_ON ? '|' : ' ');
			if(c == sidelen()) break;
			out << m_game->cells(p);
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_SlitherLink()
{
	using namespace puzzles::SlitherLink;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\SlitherLink.xml", "Puzzles\\SlitherLink.txt", solution_format::GOAL_STATE_ONLY);
}