#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 4/Calcudoku

	Summary
	Mathematical Sudoku

	Description
	1. Write numbers ranging from 1 to board size respecting the calculation
	   hint.
	2. The tiny numbers and math signs in the corner of an area give you the
	   hint about what's happening inside that area.
	3. For example a '3+' means that the sum of the numbers inside that area
	   equals 3. In that case you would have to write the numbers 1 and 2
	   there.
	4. Another example: '12*' means that the multiplication of the numbers
	   in that area gives 12, so it could be 3 and 4 or even 3, 4 and 1,
	   depending on the area size.
	5. Even where the order of the operands matter (in subtraction and division)
	   they can appear in any order inside the area (ie.e. 2/ could be done
	   with 4 and 2 or 2 and 4.
	6. All the numbers appear just one time in each row and column, but they
	   could be repeated in non-straight areas, like the L-shaped one.
*/

namespace puzzles{ namespace Calcudoku{

#define PUZ_ADD		'+'
#define PUZ_SUB		'-'
#define PUZ_MUL		'*'
#define PUZ_DIV		'/'

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_area_info
{
	vector<Position> m_range;
	char m_operator;
	int m_result;
	vector<string> m_disps;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2area;
	vector<puz_area_info> m_area_info;
	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size())
{
	for(int r = 0; r < m_sidelen; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen; ++c){
			Position p(r, c);
			auto s = str.substr(c * 4, 4);
			int n = s[0] - 'a';
			m_pos2area[p] = n;

			if(n >= m_area_info.size())
				m_area_info.resize(n + 1);
			auto& info = m_area_info[n];
			info.m_range.push_back(p);
			if(s.substr(1) == "    ") continue;
			info.m_operator = s[2];
			info.m_result = atoi(s.substr(0, 2).c_str());
		}
	}

	map<pair<int, int>, vector<string>> pair2disps;
	for(auto& info : m_area_info){
		int ps_cnt = info.m_range.size();
		int flower_cnt = info.m_flower_count;
		auto& disps = pair2disps[make_pair(ps_cnt, flower_cnt)];
		if(disps.empty())
			for(int i = 0; i < ps_cnt; ++i){
				if(flower_cnt != PUZ_FLOWER_COUNT_UNKOWN && flower_cnt != i) continue;
				auto disp = string(ps_cnt - i, PUZ_EMPTY) + string(i, PUZ_FLOWER);
				do
					disps.push_back(disp);
				while(boost::next_permutation(disp));
			}
		for(const auto& disp : disps){
			vector<Position> ps_flower;
			for(int i = 0; i < disp.size(); ++i)
				if(disp[i] == PUZ_FLOWER)
					ps_flower.push_back(info.m_range[i]);

			if([&](){
				// no touching
				for(const auto& ps1 : ps_flower)
					for(const auto& ps2 : ps_flower)
						if(boost::algorithm::any_of_equal(offset, ps1 - ps2))
							return false;
				return true;
			}())
				info.m_disps.push_back(disp);
		}
	}

	boost::sort(m_area_info, [this](const puz_area_info& info1, const puz_area_info& info2){
		return info1.m_disps.size() < info2.m_disps.size();
	});
}

struct puz_state : string
{
	puz_state() {}
	puz_state(const puz_game& g);
	int sidelen() const {return m_game->m_sidelen;}
	char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
	char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
	bool make_move(int i);
	void count_unbalanced();

	//solve_puzzle interface
	bool is_goal_state() const {return get_heuristic() == 0;}
	void gen_children(list<puz_state>& children) const;
	unsigned int get_heuristic() const {
		return m_game->m_area_info.size() - m_fb_index + m_unbalanced; 
	}
	unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
	void dump_move(ostream& out) const {}
	ostream& dump(ostream& out) const;
	friend ostream& operator<<(ostream& out, const puz_state& state) {
		return state.dump(out);
	}

	const puz_game* m_game = nullptr;
	int m_fb_index = 0;
	unsigned int m_distance = 0;
	unsigned int m_unbalanced = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
	for(int i = 0; i < sidelen(); ++i)
		cells(Position(i, 0)) = cells(Position(i, sidelen() - 1)) =
		cells(Position(0, i)) = cells(Position(sidelen() - 1, i)) = PUZ_BOUNDARY;

	count_unbalanced();
}

void puz_state::count_unbalanced()
{
	auto f = [this](Position p, const Position& os){
		char ch_last = PUZ_BOUNDARY;
		int fb_last = -1;
		for(int i = 1, n = 0; i < sidelen(); ++i){
			char ch = cells(p);
			if(is_empty(ch)){
				int fb = m_game->m_pos2fb.at(p);
				if(fb != fb_last)
					++n, fb_last = fb;
			}
			else{
				fb_last = -1;
				if(n > 2)
					m_unbalanced += n - 2;
				n = 0;
			}
			ch_last = ch;
			p += os;
		}
	};

	m_unbalanced = 0;
	for(int i = 1; i < sidelen() - 1; ++i){
		f(Position(i, 1), offset[1]);
		f(Position(1, i), offset[2]);
	}
}

struct puz_state2 : Position
{
	puz_state2(const puz_state& s);

	int sidelen() const { return m_state->sidelen(); }
	void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
	void gen_children(list<puz_state2>& children) const;

	const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
	int i = boost::find_if(s, [](char ch){
		return is_empty(ch);
	}) - s.begin();

	make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
	for(auto& os : offset){
		auto p2 = *this + os;
		if(is_empty(m_state->cells(p2))){
			children.push_back(*this);
			children.back().make_move(p2);
		}
	}
}

bool puz_state::make_move(int i)
{
	m_distance = 0;
	auto& info = m_game->m_area_info[m_fb_index];
	auto& area = info.m_range;
	auto& disp = info.m_disps[i];

	auto ub = m_unbalanced;
	for(int k = 0; k < disp.size(); ++k){
		auto& p = area[k];
		if((cells(p) = disp[k]) != PUZ_FLOWER) continue;

		// no touching
		for(auto& os : offset)
			if(cells(p + os) == PUZ_FLOWER)
				return false;
	}

	// interconnected spaces
	list<puz_state2> smoves;
	puz_move_generator<puz_state2>::gen_moves(*this, smoves);
	if(smoves.size() != boost::count_if(*this, [](char ch){
		return is_empty(ch);
	}))
		return false;

	count_unbalanced();
	m_distance += ub - m_unbalanced + 1;
	return ++m_fb_index != m_game->m_area_info.size() || m_unbalanced == 0;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	int sz = m_game->m_area_info[m_fb_index].m_disps.size();
	for(int i = 0; i < sz; ++i){
		children.push_back(*this);
		if(!children.back().make_move(i))
			children.pop_back();
	}
}

ostream& puz_state::dump(ostream& out) const
{
	for(int r = 1; r < sidelen() - 1; ++r){
		for(int c = 1; c < sidelen() - 1; ++c)
			out << cells(Position(r, c)) << ' ';
		out << endl;
	}
	return out;
}

}}

void solve_puz_Calcudoku()
{
	using namespace puzzles::Calcudoku;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Calcudoku.xml", "Puzzles\\Calcudoku.txt", solution_format::GOAL_STATE_ONLY);
}