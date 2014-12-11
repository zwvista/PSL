#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 5/NumberLink

	Summary
	Connect the same numbers without the crossing paths

	Description
	1. Connect the couples of equal numbers (i.e. 2 with 2, 3 with 3 etc)
	   with a continuous line.
	2. The line can only go horizontally or vertically and can't cross
	   itself or other lines.
	3. Lines must originate on a number and must end in the other equal
	   number.
	4. At the end of the puzzle, you must have covered ALL the squares with
	   lines and no line can cover a 2*2 area (like a 180 degree turn).
	5. In other words you can't turn right and immediately right again. The
	   same happens on the left, obviously. Be careful not to miss this rule.

	Variant
	6. In some levels there will be a note that tells you don't need to cover
	   all the squares.
	7. In some levels you will have more than a couple of the same number.
	   In these cases, you must connect ALL the same numbers together.
*/

namespace puzzles{ namespace NumberLink{

#define PUZ_SPACE			' '
#define PUZ_NUMBER			'N'
#define PUZ_BOUNDARY		'+'
#define PUZ_LINE_OFF		'0'
#define PUZ_LINE_ON			'1'

const string lines_off = "0000";

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, char> m_pos2num;
	map<char, vector<Position>> m_num2targets;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			char ch = str[c - 1];
			m_start.push_back(ch);
			if(ch != PUZ_SPACE){
				Position p(r, c);
				m_pos2num[p] = ch;
				m_num2targets[ch].push_back(p);
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(string(m_sidelen, PUZ_BOUNDARY));
}

struct puz_link
{
	Position m_head, m_tail;
	vector<pair<int, int>> m_moves;
	puz_link() {}
	puz_link(const Position& p){
		m_head = m_tail = p;
	}

	void find_moves(struct puz_state& s, bool both, const vector<puz_link>& links);
};

struct puz_state : vector<string>
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	string cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	string& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(char num, int n, bool opposite);
	bool check_board() const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_num2links, 0, [](int acc, const pair<char, vector<puz_link>>& kv){
			return acc + kv.second.size();
		});
	}
	unsigned int get_distance(const puz_state& child) const { return 1; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<char, vector<puz_link>> m_num2links;
};

void puz_link::find_moves(struct puz_state& s, bool both, const vector<puz_link>& links)
{
	m_moves.clear();
	for(int i = 0; i < (both ? 2 : 1); ++i){
		auto p = i == 0 ? m_head : m_tail;
		auto p3 = i == 0 ? m_tail : m_head;
		char ch = s.cells(p)[0];
		for(int j = 0; j < 4; ++j){
			int k = i == 0 ? j : ~j;
			auto p2 = p + offset[j];
			char ch2 = s.cells(p2)[0];
			if(ch2 == PUZ_SPACE)
				m_moves.emplace_back(k, -1);
			else if(ch2 == ch && p2 != p3){
				auto it = boost::find_if(links, [&](const puz_link& link){
					return p2 == link.m_head;
				});
				if(it != links.end()){
					m_moves.emplace_back(k, (it - links.begin()) * 2);
					continue;
				}
				it = boost::find_if(links, [&](const puz_link& link){
					return p2 == link.m_tail;
				});
				if(it != links.end())
					m_moves.emplace_back(k, (it - links.begin()) * 2 + 1);
			}
		}
	}
}

puz_state::puz_state(const puz_game& g)
: vector<string>(g.m_start.size()), m_game(&g)
{
	for(int i = 0; i < size(); ++i){
		auto& str = (*this)[i];
		str.push_back(g.m_start[i]);
		str.append(lines_off);
	}
	
	for(auto& kv : g.m_num2targets){
		auto& links = m_num2links[kv.first];
		for(auto& p : kv.second)
			links.push_back(p);
		for(auto& link : links)
			link.find_moves(*this, false, links);
	}
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& a, const Position& p_start)
		: m_area(a) { make_move(p_start); }

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>& m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p = *this + os;
		if(m_area.count(p) != 0){
			children.push_back(*this);
			children.back().make_move(p);
		}
	}
}

bool puz_state::check_board() const
{
	set<Position> area, area2;
	for(int r = 1; r < sidelen() - 1; ++r)
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			if(cells(p)[0] == PUZ_SPACE)
				area.insert(p);
		}

	//for(auto& kv : m_num2links){
	//	auto& link = kv.second;
	//	auto& tg = link.m_targets;
	//	auto area3 = area;
	//	area3.insert(tg.begin(), tg.end());

	//	list<puz_state2> smoves;
	//	puz_move_generator<puz_state2>::gen_moves({area3, link.m_head}, smoves);
	//	set<Position> area4(smoves.begin(), smoves.end());
	//	if(boost::algorithm::any_of(tg, [&](const Position& p){
	//		return area4.count(p) == 0;
	//	}))
	//		return false;

	//	area4.erase(link.m_head);
	//	for(auto& p : tg)
	//		area4.erase(p);
	//	area2.insert(area4.begin(), area4.end());
	//}

	return area.size() == area2.size();
}

bool puz_state::make_move(char num, int n, bool opposite)
{
	//auto& link = m_num2links.at(num);

	//link.m_new_target = false;
	//if(opposite){
	//	::swap(link.m_head, link.m_tail);
	//}
	//auto p = link.m_head + offset[n];
	//auto &str = cells(link.m_head), &str2 = cells(p);
	//char ch = str2[0];
	//auto& tg = link.m_targets;

	//str2[0] = str[0];
	//str2[(2 + n) % 4 + 1] = str[n + 1] = PUZ_LINE_ON;
	//link.m_head = p;

	//if(ch != PUZ_SPACE){
	//	tg.erase(p);
	//	if(tg.empty())
	//		m_num2links.erase(num);
	//	else{
	//		link.m_new_target = true;
	//		link.find_moves(*this);
	//	}
	//}
	//else
	//	link.find_moves(*this);

	return check_board();
}

void puz_state::gen_children(list<puz_state>& children) const
{
	//auto& kv = *boost::min_element(m_num2links, [&](
	//	const pair<char, puz_link>& kv1,
	//	const pair<char, puz_link>& kv2){
	//	return kv1.second.m_moves.size() < kv2.second.m_moves.size();
	//});
	//for(auto& kv2 : kv.second.m_moves){
	//	children.push_back(*this);
	//	if(!children.back().make_move(kv.first, kv2.first, kv2.second))
	//		children.pop_back();
	//}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1;; ++r){
		// draw horz-lines
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			auto& str = cells(p);
			auto it = m_game->m_pos2num.find(p);
			out << (it != m_game->m_pos2num.end() ? it->second : ' ')
				<< (str[2] == PUZ_LINE_ON ? '-' : ' ');
		}
		out << endl;
		if(r == sidelen() - 2) break;
		for(int c = 1; c < sidelen() - 1; ++c)
			// draw vert-lines
			out << (cells({r, c})[3] == PUZ_LINE_ON ? "| " : "  ");
		out << endl;
	}
	return out;
}

}}

void solve_puz_NumberLink()
{
	using namespace puzzles::NumberLink;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\NumberLink.xml", "Puzzles\\NumberLink.txt", solution_format::GOAL_STATE_ONLY);
}