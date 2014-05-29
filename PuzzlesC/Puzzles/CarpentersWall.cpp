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

enum class tool_hint_type {
	CORNER,
	NUMBER,
	ARM_END
};

struct puz_tool
{
	Position m_hint_pos;
	char m_hint;
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
				m_ch2tool[ch_t] = {{r, c}, ch};
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
	int adjust_area(bool init);
	bool is_continuous() const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
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
	unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
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

	adjust_area(false);
}

int puz_state::adjust_area(bool init)
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

	for(auto& kv : m_matches){
		char ch = kv.first;
		auto& ranges = kv.second;
		ranges.clear();

		auto& t = m_game->m_ch2tool.at(ch);
		switch(t.hint_type())
		{
		case tool_hint_type::CORNER:
			break;
		case tool_hint_type::NUMBER:
			break;
		case tool_hint_type::ARM_END:
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
	//auto& g = m_ch2tool.at(ch);
	//cells(p) = ch;
	//g.m_inner.insert(p);
	//if(--g.m_remaining == 0){
	//	for(auto& p2 : g.m_inner)
	//		for(auto& os : offset){
	//			char& ch2 = cells(p2 + os);
	//			if(ch2 == PUZ_SPACE)
	//				ch2 = PUZ_WALL;
	//		}
	//	m_ch2tool.erase(ch);
	//}
	//++m_distance;

	//boost::remove_erase_if(m_2by2walls, [&](const set<Position>& rng){
	//	return rng.count(p) != 0;
	//});

	//// pruning
	//if(boost::algorithm::any_of(m_2by2walls, [&](const set<Position>& rng){
	//	return boost::algorithm::none_of(rng, [&](const Position& p2){
	//		return boost::algorithm::any_of(m_ch2tool, [&](const pair<char, puz_tool>& kv){
	//			auto& g = kv.second;
	//			return boost::algorithm::any_of(g.m_inner, [&](const Position& p3){
	//				return manhattan_distance(p2, p3) <= g.m_remaining;
	//			});
	//		});
	//	});
	//}))
	//	return false;

	adjust_area(true);
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

void puz_state::gen_children(list<puz_state>& children) const
{
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

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_SPACE || ch == PUZ_WALL)
				out << PUZ_WALL << ' ';
			else{
				//auto it = m_game->m_pos2num.find(p);
				//if(it == m_game->m_pos2num.end())
				//	out << ". ";
				//else
				//	out << format("%-2d") % it->second;
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