#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 10/Tap-A-Row

	Summary
	Tap me a row, please

	Description
	1. Plays with the same rules as Tapa with these variations:
	2. The number also tells you the filled cell count for that row.
	3. In other words, the sum of the digits in that row equals the number
	   of that row.
*/

namespace puzzles{ namespace TapARow{

#define PUZ_SPACE		' '
#define PUZ_QM			'?'
#define PUZ_FILLED		'F'
#define PUZ_EMPTY		'.'
#define PUZ_HINT		'H'
#define PUZ_BOUNDARY	'B'
#define PUZ_UNKNOWN		-1

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

typedef vector<int> puz_hint;

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, puz_hint> m_pos2hint;
	map<puz_hint, vector<string>> m_hint2perms;
	string m_start;

	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id{attrs.get<string>("id")}
, m_sidelen(strs.size() + 2)
{
	m_start.append(m_sidelen, PUZ_BOUNDARY);
	for(int r = 1; r < m_sidelen - 1; ++r){
		auto& str = strs[r - 1];
		m_start.push_back(PUZ_BOUNDARY);
		for(int c = 1; c < m_sidelen - 1; ++c){
			auto s = str.substr(c * 4 - 4, 4);
			if(s == "    ")
				m_start.push_back(PUZ_SPACE);
			else{
				m_start.push_back(PUZ_HINT);
				auto& hint = m_pos2hint[{r, c}];
				for(int i = 0, n = 0; i < 4; ++i){
					char ch = s[i];
					if(ch != PUZ_SPACE)
						hint.push_back(ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0');
				}
				m_hint2perms[hint];
			}
		}
		m_start.push_back(PUZ_BOUNDARY);
	}
	m_start.append(m_sidelen, PUZ_BOUNDARY);

	// A tile is surrounded by at most 8 tiles, each of which has two states:
	// filled or empty. So all combinations of the states of the
	// surrounding tiles can be coded into an 8-bit number(0 -- 255).
	for(int i = 1; i < 256; ++i){
		vector<int> filled;
		for(int j = 0; j < 8; ++j)
			if(i & (1 << j))
				filled.push_back(j);

		string perm(8, PUZ_EMPTY);
		for(int j : filled)
			perm[j] = PUZ_FILLED;

		vector<int> hint;
		for(int j = 0; j < filled.size(); ++j)
			if(j == 0 || filled[j] - filled[j - 1] != 1)
				hint.push_back(1);
			else
				++hint.back();
		if(filled.size() > 1 && hint.size() > 1 && filled.back() - filled.front() == 7)
			hint.front() += hint.back(), hint.pop_back();
		boost::sort(hint);
		
		for(auto& kv : m_hint2perms){
			auto& hint2 = kv.first;
			if(hint2 == hint)
				kv.second.push_back(perm);
			else if(boost::algorithm::any_of_equal(hint2, PUZ_UNKNOWN) && hint2.size() == hint.size()){
				auto hint3 = hint2;
				boost::remove_erase(hint3, PUZ_UNKNOWN);
				if(boost::includes(hint, hint3))
					kv.second.push_back(perm);
			}
		}
	}

	// A cell with a 0 means all its surrounding cells are empty.
	auto it = m_hint2perms.find({0});
	if(it != m_hint2perms.end())
		it->second = {string(8, PUZ_EMPTY)};
}

typedef pair<vector<Position>, int> puz_path;

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move_hint(const Position& p, int n);
	bool make_move_hint2(const Position& p, int n);
	bool make_move_space(const Position& p, char ch);
	int find_matches(bool init);
	vector<puz_path> find_paths() const;
	bool is_valid_move() const;

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
	unsigned int get_distance(const puz_state& child) const { return m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	map<Position, vector<int>> m_matches;
	unsigned int m_distance = 0;
};

int puz_state::find_matches(bool init)
{
	for(auto& kv : m_matches){
		const auto& p = kv.first;
		auto& perm_ids = kv.second;

		string chars;
		for(auto& os : offset)
			chars.push_back(cells(p + os));

		auto& perms = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p));
		boost::remove_erase_if(perm_ids, [&](int id){
			return !boost::equal(chars, perms[id], [](char ch1, char ch2){
				return (ch1 == PUZ_BOUNDARY || ch1 == PUZ_HINT) && ch2 == PUZ_EMPTY ||
					ch1 == PUZ_SPACE || ch1 == ch2;
			});
		});

		if(!init)
			switch(perm_ids.size()){
			case 0:
				return 0;
			case 1:
				return make_move_hint2(p, perm_ids.front()) ? 1 : 0;
			}
	}
	return 2;
}

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
	for(const auto& kv : g.m_pos2hint){
		auto& perm_ids = m_matches[kv.first];
		perm_ids.resize(g.m_hint2perms.at(kv.second).size());
		boost::iota(perm_ids, 0);
	}

	find_matches(true);
}

struct puz_state2 : Position
{
	puz_state2(const set<Position>& a);

	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const set<Position>* m_area;
};

puz_state2::puz_state2(const set<Position>& a)
: m_area(&a)
{
	make_move(*a.begin());
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(int i = 0; i < 4; ++i){
		auto p2 = *this + offset[i * 2];
		if(m_area->count(p2) != 0){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

vector<puz_path> puz_state::find_paths() const
{
	set<Position> a;
	for(int i = 0; i < length(); ++i){
		char ch = (*this)[i];
		if(ch == PUZ_SPACE || ch == PUZ_FILLED)
			a.insert({i / sidelen(), i % sidelen()});
	}

	vector<puz_path> paths;
	while(!a.empty()){
		list<puz_state2> smoves;
		puz_move_generator<puz_state2>::gen_moves(a, smoves);
		paths.emplace_back();
		auto& path = paths.back();
		for(auto& p : smoves){
			a.erase(p);
			path.first.push_back(p);
			if(cells(p) == PUZ_FILLED)
				path.second++;
		}
	}

	boost::sort(paths, [&](const puz_path& path1, const puz_path& path2){
		return path1.second > path2.second;
	});

	return paths;
}

bool puz_state::make_move_hint2(const Position& p, int n)
{
	auto& perm = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p))[n];
	for(int i = 0; i < 8; ++i){
		auto p2 = p + offset[i];
		char& ch = cells(p2);
		if(ch == PUZ_SPACE)
			ch = perm[i], ++m_distance;
	}

	m_matches.erase(p);
	if(m_matches.empty()){
		auto paths = find_paths();
		while(paths.size() > 1){
			auto& path = paths.back();
			if(path.second != 0)
				return false;

			for(auto& p2 : path.first)
				cells(p2) = PUZ_EMPTY, ++m_distance;
			paths.pop_back();
		}
	}

	return is_valid_move();
}

bool puz_state::make_move_hint(const Position& p, int n)
{
	m_distance = 0;
	if(!make_move_hint2(p, n))
		return false;
	int m;
	while((m = find_matches(false)) == 1);
	return m == 2;
}

bool puz_state::make_move_space(const Position& p, char ch)
{
	cells(p) = ch;
	m_distance = 1;

	return find_paths().size() == 1 && is_valid_move();
}

bool puz_state::is_valid_move() const
{
	auto four_cells = [&](const Position& p1, const Position& p2, const Position& p3, const Position& p4, char ch){
		return cells(p1) == ch && cells(p2) == ch && cells(p3) == ch && cells(p4) == ch;
	};

	auto is_valid_square = [&](char ch){
		for(int r = 1; r < sidelen() - 2; ++r)
			for(int c = 1; c < sidelen() - 2; ++c)
				if(four_cells({r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}, ch))
					return false;
		return true;
	};

	auto is_valid_row = [&]{
		bool b = is_goal_state();
		for(int r = 1; r < sidelen() - 1; ++r){
			bool has_hint = false;
			int n_hint = 0, n_filled = 0;
			for(int c = 1; c < sidelen() - 1; ++c){
				Position p(r, c);
				char ch = cells(p);
				if(ch == PUZ_HINT){
					has_hint = true;
					n_hint += boost::accumulate(m_game->m_pos2hint.at(p), 0);
				}
				else if(ch == PUZ_FILLED)
					n_filled++;
			}
			if(has_hint && (!b && n_filled > n_hint || b && n_filled != n_hint))
				return false;
		}
		return true;
	};

	return is_valid_square(PUZ_FILLED) && is_valid_row();
}

void puz_state::gen_children(list<puz_state>& children) const
{
	if(!m_matches.empty()){
		auto& kv = *boost::min_element(m_matches, [](
			const pair<const Position, vector<int>>& kv1,
			const pair<const Position, vector<int>>& kv2){
			return kv1.second.size() < kv2.second.size();
		});

		for(int n : kv.second){
			children.push_back(*this);
			if(!children.back().make_move_hint(kv.first, n))
				children.pop_back();
		}
	}
	else{
		int n = find(PUZ_SPACE);
		if(n == -1)
			return;
		Position p(n / sidelen(), n % sidelen());
		for(char ch : {PUZ_FILLED, PUZ_EMPTY}){
			children.push_back(*this);
			if(!children.back().make_move_space(p, ch))
				children.pop_back();
		}
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r) {
		for(int c = 1; c < sidelen() - 1; ++c){
			Position p(r, c);
			char ch = cells(p);
			if(ch == PUZ_HINT){
				auto& hint = m_game->m_pos2hint.at(p);
				for(int n : hint)
					out << char(n == PUZ_UNKNOWN ? PUZ_QM : n + '0');
				out << string(4 - hint.size(), ' ');
			}
			else
				out << ch << "   ";
		}
		out << endl;
	}
	return out;
}

}}

void solve_puz_TapARow()
{
	using namespace puzzles::TapARow;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\TapARow.xml", "Puzzles\\TapARow.txt", solution_format::GOAL_STATE_ONLY);
}