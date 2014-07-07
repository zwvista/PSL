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

	ios game: Logic Games/Puzzle Set 1/Hedgehog Forest

	What's up?
	Lost in the Woods

	Description
	1. After eating all the fruit, the Hedgehog wanders in the Forest.
	2. In the middle, he finds Acorns, which he loves particularly.
	3. In order to get Acorns, he can move in and out of the central section
	   (like Orchards).
	4. At the margins of the Forest there are a few boulders, with no fruit
	   on them.
	5. He can dig under the boulders, but not stop on them (disregard them).
	6. The highest number in the Forest is 73. After 73, you don't need to
	   reach n.1, like the Back Garden and Orchard.

	Variants
	7. He can move in and out the middle section, horizontally or vertically
	   (not diagonally).
	8. Numbers go from 1 to 73 and there's no need to loop back to n.1.
	   Boulders can be ignored.

	ios game: Logic Games/Puzzle Set 1/Hedgehog City

	What's up?
	Mr. Hedgehog goes to Washington

	Description
	1. In the end, Mr. Hedgehog gets lost in the City after hearing about
	   a 'Big apple'.
	2. Having found no such big fruit, he adapts to the City, taking the
	   Tube and eating Junk Food (and Pizza)!
	3. The Tube can take Mr. H. across town to the opposite District, in
	   the same exact spot where he was in the starting District.
	4. Junk Food has the same effect as Cider on the Hedgehog. Same as in
	   Orchard, he moves Clockwise and Counterclockwise.
	5. He also needs to close the trip.
*/

namespace puzzles{ namespace Hedgehog{

#define PUZ_SPACE		0
#define PUZ_BOULDER		-1
#define PUZ_BOULDER_STR	" OO"

const Position offset[] = {
	{-1, 0},		// n
	{-1, 1},		// ne
	{0, 1},		// e
	{1, 1},		// se
	{1, 0},		// s
	{1, -1},		// sw
	{0, -1},		// w
	{-1, -1},	// nw
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
			auto s = str.substr(c * 3, 3);
			int n = s == PUZ_BOULDER_STR ? PUZ_BOULDER : atoi(s.c_str());
			m_start.push_back(n);
			if(n > 0)
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
	auto f = [&](const pair<int, Position>& kv_cur, const pair<int, Position>& kv_next){
		if(kv_next.first - kv_cur.first != 1){
			m_segments.emplace_back();
			auto& o = m_segments.back();
			o.m_cur = {kv_cur.second, kv_cur.first};
			o.m_dest = {kv_next.second, kv_next.first};
			segment_next(o);
		}
	};
	for(auto prev = m_game->m_num2pos.begin(), first = std::next(prev),
		last = m_game->m_num2pos.end(); first != last; ++prev, ++first)
		f(*prev, *first);

	// close the trip
	auto &kv1 = *m_game->m_num2pos.rbegin(), &kv2 = *m_game->m_num2pos.begin();
	int n = m_game->m_game_type == puz_game_type::FOREST ? -1 :
		sidelen() * sidelen() + 1;
	f(kv1, {n, kv2.second});
}

vector<int> puz_state::get_dirs(const Position& p) const
{
	vector<int> dirs;
	if(m_game->m_game_type == puz_game_type::FOREST){
		// 2  | 24  | 4
		// -  -     -
		// 02 | 0246| 46
		// -  -     -
		// 0  | 06  | 6
		switch(int n = p.first / 3 * 3 + p.second / 3){
		case 0: dirs = {2}; break;
		case 1: dirs = {2, 4}; break;
		case 2: dirs = {4}; break;
		case 3: dirs = {0, 2}; break;
		case 4: dirs = {0, 2, 4, 6}; break;
		case 5: dirs = {4, 6}; break;
		case 6: dirs = {0}; break;
		case 7: dirs = {0, 6}; break;
		case 8: dirs = {6}; break;
		}
	}
	else{
		// 234 | 456
		// - - - - -
		// 012 | 670
		int half = sidelen() / 2;
		int n = p.first / half * 2 + p.second / half;
		switch(n){
		case 0: n = 2; break;
		case 1: n = 4; break;
		case 2: n = 0; break;
		case 3: n = 6; break;
		}
		dirs.push_back(n);
		if(m_game->m_game_type == puz_game_type::CITY)
			dirs.push_back(n + 1);
		if(m_game->m_game_type != puz_game_type::BACK_GARDEN)
			dirs.push_back((n + 2) % 8);
	}
	return dirs;
};

void puz_state::segment_next(puz_segment& o) const
{
	auto f = [&](int d){
		int half = sidelen() / 2;
		auto os = offset[d];
		if(d % 2 == 1)
			os = {os.first * half, os.second * half};
		return os;
	};

	int n1 = o.m_cur.second + 1, n2 = o.m_dest.second;
	auto &p1 = o.m_cur.first, &p2 = o.m_dest.first;
	o.m_next.clear();
	auto ds1 = get_dirs(p1);
	for(int d1 : ds1){
		auto os = f(d1);
		for(auto p3 = p1 + os; is_valid(p3); p3 += os){
			auto ds3 = get_dirs(p3);
			if(ds3 == ds1) continue;
			for(int d3 : ds3){
				if(cells(p3) == PUZ_SPACE && (n1 + 1 != n2 || [&]{
					auto os2 = f(d3);
					for(auto p4 = p3 + os2; is_valid(p4); p4 += os2){
						auto ds4 = get_dirs(p4);
						if(ds4 == ds3) continue;
						if(p4 == p2)
							return true;
					}
					return false;
				}()) && !boost::algorithm::any_of_equal(o.m_next, p3))
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
	if(o.m_dest.second - o.m_cur.second == 1 ||
		o.m_dest.second == -1 && o.m_cur.second == 73)
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
		for(int c = 0; c < sidelen(); ++c){
			int n = cells({r, c});
			if(n == PUZ_BOULDER)
				out << PUZ_BOULDER_STR;
			else
				out << format("%3d") % n;
		}
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
