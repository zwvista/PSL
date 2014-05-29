#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 12/Carpenter's Wall

	Summary
	Angled Walls

	Description
	1. In this game you have to create a valid Nurikabe following a different
	   type of hints.
	2. In the end, the empty spaces left by the Nurikabe will form many Capenter's
	   Squares (L shaped tools) of different size.
	3. The circled numbers on the board indicate the corner of the L.
	4. When a number is inside the circle, that indicates the total number of
	   squares occupied by the L.
	5. The arrow always sits at the end of an arm and points to the corner of
	   an L.
	6. Not all the Carpenter's Squares might be indicated: some could be hidden
	   and no hint given.
*/

namespace puzzles{ namespace CarpentersWall{

#define PUZ_SPACE		' '
#define PUZ_WALL		'W'
#define PUZ_BOUNDARY	'+'
#define PUZ_CORNER		'O'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

const vector<vector<int>> tool_dirs2 = {
	{0, 1}, {0, 3}, {1, 2}, {2, 3}
};

const string tool_dirs = "^>v<";

enum class tool_hint_type {
	CORNER,
	NUMBER,
	ARM_END
};

struct puz_tool
{
	Position m_hint_pos;
	char m_hint = PUZ_CORNER;
	puz_tool() {}
	puz_tool(const Position& p, char ch) : m_hint_pos(p), m_hint(ch) {}
	tool_hint_type hint_type() const {
		return m_hint == PUZ_CORNER ? tool_hint_type::CORNER :
			isdigit(m_hint) ? tool_hint_type::NUMBER : tool_hint_type::ARM_END;
	}
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, char> m_pos2ch;
	map<char, puz_tool> m_ch2tool;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	char ch_t = 'a';
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			if(ch == PUZ_SPACE)
				m_start.push_back(PUZ_SPACE);
			else{
				Position p(r, c);
				m_pos2ch[p] = ch;
				m_ch2tool[ch_t] = {p, ch};
				m_start.push_back(ch_t++);
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(char ch, int n);
	bool make_move2(char ch, int n);
	bool make_move_hidden(char ch, int n);
	int adjust_area(bool init);
	void check_board();
	bool is_continuous() const;
	const puz_tool& get_tool(char ch) const {
		auto it = m_game->m_ch2tool.find(ch);
		return it == m_game->m_ch2tool.end() ? m_next_tool : it->second;
	}
	void add_tool(const Position& p) {
		m_next_tool.m_hint_pos = p;
		m_matches[m_next_ch];
		adjust_area(true);
	}

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0 && m_2by2walls.empty();}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return m_matches.size(); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<char, vector<vector<Position>>> m_matches;
	vector<set<Position>> m_2by2walls;
	puz_tool m_next_tool;
	char m_next_ch;
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_next_ch('a' + g.m_ch2tool.size())
{
	for(int r = 1; r < g.m_sidelen - 2; ++r)
		for(int c = 1; c < g.m_sidelen - 2; ++c){
			set<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
			if(boost::algorithm::all_of(rng, [&](const Position& p){
				return cells(p) == PUZ_SPACE;
			}))
				m_2by2walls.push_back(rng);
		}

	for(auto& kv : g.m_ch2tool)
		m_matches[kv.first];

	check_board();
	adjust_area(true);
}

void puz_state::check_board()
{
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			if(cells(p) != PUZ_SPACE) continue;
			set<char> chars;
			for(auto& os : offset){
				char ch = cells(p + os);
				if(ch != PUZ_BOUNDARY && ch != PUZ_SPACE && ch != PUZ_WALL)
					chars.insert(ch);
			}
			if(chars.size() > 1)
				cells(p) = PUZ_WALL;
		}
}

int puz_state::adjust_area(bool init)
{
	for(auto& kv : m_matches){
		char ch = kv.first;
		auto& ranges = kv.second;
		ranges.clear();
		auto& t = get_tool(ch);
		auto& p = t.m_hint_pos;

		auto f = [&](const Position& p2){
			return cells(p2) == PUZ_SPACE &&
				boost::algorithm::none_of(offset, [&](const Position& os2){
				char ch2 = this->cells(p2 + os2);
				return ch2 != PUZ_BOUNDARY && ch2 != PUZ_SPACE &&
					ch2 != PUZ_WALL && ch2 != ch;
			});
		};

		auto g1 = [&](int cnt){
			vector<vector<Position>> arms(4);
			for(int i = 0; i < 4; ++i){
				auto& os = offset[i];
				for(auto p2 = p + os; f(p2); p2 += os)
					arms[i].push_back(p2);
			}
			for(auto& dirs : tool_dirs2){
				auto &a0 = arms[dirs[0]], &a1 = arms[dirs[1]];
				if(a0.empty() || a1.empty()) continue;
				for(int i = 1; i <= a0.size(); ++i)
					for(int j = 1; j <= a1.size(); ++j){
						if(cnt != -1 && i + j + 1 != cnt) continue;
						vector<Position> rng;
						for(int k = 0; k < i; ++k)
							rng.push_back(a0[k]);
						for(int k = 0; k < j; ++k)
							rng.push_back(a1[k]);
						rng.push_back(p);
						boost::sort(rng);
						ranges.push_back(rng);
					}
			}
		};

		auto g2 = [&](int n){
			vector<Position> a0, a1;
			auto& os = offset[n];
			int n21 = (n + 3) % 4, n22 = (n + 1) % 4;
			for(auto p2 = p + os; f(p2); p2 += os){
				a0.push_back(p2);
				for(int n2 : {n21, n22}){
					auto& os2 = offset[n2];
					a1.clear();
					for(auto p3 = p2 + os2; f(p3); p3 += os2)
						a1.push_back(p3);
					if(a1.empty()) continue;
					for(int j = 1; j <= a1.size(); ++j){
						vector<Position> rng;
						for(int k = 0; k < a0.size(); ++k)
							rng.push_back(a0[k]);
						for(int k = 0; k < j; ++k)
							rng.push_back(a1[k]);
						rng.push_back(p);
						boost::sort(rng);
						ranges.push_back(rng);
					}
				}
			}
		};

		switch(t.hint_type()){
		case tool_hint_type::CORNER:
			g1(-1);
			break;
		case tool_hint_type::NUMBER:
			g1(t.m_hint - '0');
			break;
		case tool_hint_type::ARM_END:
			g2(tool_dirs.find(t.m_hint));
			break;
		}

		if(!init)
			switch(ranges.size()){
			case 0:
				return 0;
			case 1:
				return make_move2(ch, 0) ? 1 : 0;
			}
	}
	return 2;
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(m_rng->count(p2) != 0){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::is_continuous() const
{
	set<Position> a;
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_SPACE || ch == PUZ_WALL)
				a.insert(p);
		}

	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(a, smoves);
	return smoves.size() == a.size();
}

bool puz_state::make_move2(char ch, int n)
{
	auto& t = get_tool(ch);
	auto& rng = m_matches.at(ch)[n];
	for(auto& p : rng){
		cells(p) = ch;
		for(auto& os : offset){
			char& ch2 = cells(p + os);
			if(ch2 == PUZ_SPACE)
				ch2 = PUZ_WALL;
		}
		boost::remove_erase_if(m_2by2walls, [&](const set<Position>& rng2){
			return rng2.count(p) != 0;
		});
	}
	m_matches.erase(ch);
	++m_distance;

	check_board();
	return is_continuous();
}

bool puz_state::make_move(char ch, int n)
{
	m_distance = 0;
	if(!make_move2(ch, n))
		return false;
	int m;
	while((m = adjust_area(false)) == 1);
	return m == 2;
}

bool puz_state::make_move_hidden(char ch, int n)
{
	if(!make_move2(ch, n))
		return false;
	m_next_ch++;
	return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_matches.empty()){
		auto& kv = *boost::min_element(m_matches, [](
			const pair<char, vector<vector<Position>>>& kv1,
			const pair<char, vector<vector<Position>>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});
		for(int i = 0; i < kv.second.size(); ++i){
			children.push_back(*this);
			if(!children.back().make_move(kv.first, i))
				children.pop_back();
		}
	}
	else
		for(int r = 1; r < sidelen() - 1; ++r)
			for(int c = 1; c < sidelen() - 1; ++c){
				Position p(r, c);
				char ch = cells(p);
				if(ch != PUZ_SPACE) continue;
				auto s = *this;
				s.add_tool(p);
				auto& kv = *s.m_matches.begin();
				for(int i = 0; i < kv.second.size(); ++i){
					children.push_back(s);
					if(!children.back().make_move_hidden(kv.first, i))
						children.pop_back();
				}
			}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_SPACE || ch == PUZ_WALL)
				out << PUZ_WALL << ' ';
			else{
				auto it = m_game->m_pos2ch.find(p);
				if(it == m_game->m_pos2ch.end())
					out << ". ";
				else
					out << it->second << ' ';
			}

		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_CarpentersWall()
{
	using namespace puzzles::CarpentersWall;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\CarpentersWall.xml", "Puzzles\\CarpentersWall.txt", solution_format::GOAL_STATE_ONLY);
}