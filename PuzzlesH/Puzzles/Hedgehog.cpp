#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 1/Hedgehog Back Garden

	Summary
	Vegetable Assault!

	Description
	1. A pretty neat hedgehog lives in a Back Garden, which is divided in
	   four smaller Vegetable fields.
	2. Hedgehogs are semi-omnivorous, but this one has a peculiar balanced
	   diet.
	3. He eats something different each day, moving CLOCKWISE between
	   vegetables.
	4. So, each day, he digs under the garden in a straight line and pops
	   up in next field.
	5. He never pops up in the same tile twice (nothing left to eat!)
	6. On the last day the Hedgehog needs to be able to correctly reach n.1,
	   closing the trip.

	ios game: Logic Games/Puzzle Set 1/Hedgehog Orchard

	What's up?
	Cider Party!

	Description
	1. Having eating a fair amount of vegetables, the Hedgehog switches to
	   fruit and moves to the Orchard.
	2. Here a few old apples have become Cider with time!
	3. Cider makes the Hedgehog particularly merry, in fact in the Orchard
	   he can move in both directions.
	4. Orchard plays like Back Garden, but each move can be Clockwise or
	   Counterclockwise.
	5. On the last day the Hedgehog needs to be able to correctly reach n.1,
	   closing the trip.
*/

namespace puzzles{ namespace Hedgehog{

#define PUZ_SPACE		0

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

enum class puz_game_type
{
	BACK_GARDEN,
	ORCHARD,
	FOREST,
	CITY,
};

struct puz_game	
{
	string m_id;
	int m_sidelen;
	puz_game_type m_game_type;
	map<int, Position> m_num2pos;
	vector<int> m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
	: m_id(attrs.get<string>("id"))
	, m_sidelen(strs.size())
{
	auto game_type = attrs.get<string>("GameType", "Back Garden");
	m_game_type =
		game_type == "Orchard" ? puz_game_type::ORCHARD :
		game_type == "Forest" ? puz_game_type::FOREST :
		game_type == "City" ? puz_game_type::CITY :
		puz_game_type::BACK_GARDEN;

	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			int n = atoi(str.substr(c * 3, 3).c_str());
			m_start.push_back(n);
			if(n != 0)
				m_num2pos[n] = p;
		}
	}
}

struct puz_segment
{
	pair<Position, int> m_cur, m_dest;
	vector<Position> m_next;
};

struct puz_state : vector<int>
{
	puz_state(const puz_game& g);
	int sidelen() const { return m_game->m_sidelen; }
	bool is_valid(const Position& p) const {
		return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
	}
	int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	void segment_next(puz_segment& o) const;
	bool make_move(int i, int j);
	vector<int> get_dirs(const Position& p) const;

	// solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return boost::accumulate(m_segments, 0, [&](int acc, const puz_segment& o){
			return acc + o.m_dest.second - o.m_cur.second - 1;
		});
	}
	unsigned int get_distance(const puz_state& child) const {return 1;}
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	vector<puz_segment> m_segments;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_start), m_game(&g)
{
	for(auto prev = m_game->m_num2pos.begin(), first = std::next(prev),
		last = m_game->m_num2pos.end(); first != last; ++prev, ++first)
		if(first->first - prev->first > 1){
			m_segments.emplace_back();
			auto& o = m_segments.back();
			o.m_cur = {prev->second, prev->first};
			o.m_dest = {first->second, first->first};
			segment_next(o);
		}
}

vector<int> puz_state::get_dirs(const Position& p) const
{
	// 1 2 | 2 3
	// - - - - -
	// 0 1 | 3 0
	int half = sidelen() / 2;
	int n = p.first / half * 2 + p.second / half;
	n = n < 2 ? n + 1 : n == 2 ? 0 : 3;
	vector<int> dirs = {n};
	if(m_game->m_game_type == puz_game_type::ORCHARD)
		dirs.push_back((n + 1) % 4);
	return dirs;
};

void puz_state::segment_next(puz_segment& o) const
{
	int n1 = o.m_cur.second + 1, n2 = o.m_dest.second;
	auto &p1 = o.m_cur.first, &p2 = o.m_dest.first;
	o.m_next.clear();
	auto ds1 = get_dirs(p1);
	for(int d1 : ds1){
		auto os = offset[d1];
		for(auto p3 = p1 + os; is_valid(p3); p3 += os){
			auto ds3 = get_dirs(p3);
			if(ds3 == ds1) continue;
			for(int d3 : ds3){
				if(cells(p3) == PUZ_SPACE && (n1 + 1 != n2 || [&]{
					auto os2 = offset[d3];
					for(auto p4 = p3 + os2; is_valid(p4); p4 += os2){
						auto ds4 = get_dirs(p4);
						if(ds4 == ds3) continue;
						if(p4 == p2)
							return true;
					}
					return false;
				}()))
					o.m_next.push_back(p3);
			}
		}
	}
}

bool puz_state::make_move(int i, int j)
{
	auto& o = m_segments[i];
	auto p = o.m_next[j];
	cells(o.m_cur.first = p) = ++o.m_cur.second;
	if(o.m_dest.second - o.m_cur.second == 1)
		m_segments.erase(m_segments.begin() + i);
	else
		segment_next(o);
	for(auto& o2 : m_segments)
		boost::remove_erase(o2.m_next, p);

	return boost::algorithm::none_of(m_segments, [](const puz_segment& o2){
		return o2.m_next.empty();
	});
}

void puz_state::gen_children(list<puz_state>& children) const
{
	auto it = boost::min_element(m_segments, [](
		const puz_segment& o1, const puz_segment& o2){
		return o1.m_next.size() < o2.m_next.size();
	});
	int i = it - m_segments.begin();

	for(int j = 0; j < it->m_next.size(); ++j){
		children.push_back(*this);
		if(!children.back().make_move(i, j))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 0; r < sidelen(); ++r){
		for(int c = 0; c < sidelen(); ++c)
			out << format("%3d") % cells({r, c});
		out << endl;
	}
	return out;
}

}}

void solve_puz_Hedgehog()
{
	using namespace puzzles::Hedgehog;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Hedgehog.xml", "Puzzles\\Hedgehog.txt", solution_format::GOAL_STATE_ONLY);
}
