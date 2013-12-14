#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
	ios game: Logic Games/Puzzle Set 7/Gardener

	Summary
	Hitori Flower Planting

	Description
	1. The Board represents a Garden, divided in many rectangular Flowerbeds.
	2. The owner of the Garden wants you to plant Flowers according to these
	   rules.
	3. A number tells you how many Flowers you must plant in that Flowerbed.
	   A Flowerbed without number can have any quantity of Flowers.
	4. Flowers can't be horizontally or vertically touching.
	5. All the remaining Garden space where there are no Flowers must be
	   interconnected (horizontally or vertically), as he wants to be able
	   to reach every part of the Garden without treading over Flowers.
	6. Lastly, there must be enough balance in the Garden, so a straight
	   line (horizontally or vertically) of non-planted tiles can't span
	   for more than two Flowerbeds.
	7. In other words, a straight path of empty space can't pass through
	   three or more Flowerbeds.
*/

namespace puzzles{ namespace Gardener{

#define PUZ_SPACE		' '
#define PUZ_EMPTY		'.'
#define PUZ_FLOWER		'F'
#define PUZ_BOUNDARY	'B'

#define PUZ_FLOWER_COUNT_UNKOWN		-1

bool is_empty(char ch) { return ch == PUZ_SPACE || ch == PUZ_EMPTY; }

const Position offset[] = {
	{-1, 0},		// n
	{0, 1},		// e
	{1, 0},		// s
	{0, -1},		// w
};

struct puz_fb_info
{
	vector<Position> m_range;
	int m_flower_count = PUZ_FLOWER_COUNT_UNKOWN;
	vector<string> m_disps;
};

struct puz_game
{
	string m_id;
	int m_sidelen;
	map<Position, int> m_pos2fb;
	vector<puz_fb_info> m_fb_info;
	puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() / 2 + 2)
{
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r];
		for(int c = 0; c < m_sidelen - 2; ++c){
			Position p(r + 1, c + 1);
			int n = str[c] - 'a';
			m_pos2fb[p] = n;
			if(n >= m_fb_info.size())
				m_fb_info.resize(n + 1);
			m_fb_info[n].m_range.push_back(p);
		}
	}
	for(int r = 0; r < m_sidelen - 2; ++r){
		auto& str = strs[r + m_sidelen - 2];
		for(int c = 0; c < m_sidelen - 2; ++c){
			char ch = str[c];
			if(ch != ' ')
				m_fb_info[m_pos2fb.at(Position(r + 1, c + 1))].m_flower_count = ch - '0';
		}
	}

	map<pair<int, int>, vector<string>> pair2disps;
	for(auto& info : m_fb_info){
		int pos_cnt = info.m_range.size();
		int flower_cnt = info.m_flower_count;
		auto& disps = pair2disps[make_pair(pos_cnt, flower_cnt)];
		if(disps.empty())
			for(int i = 0; i < pos_cnt; ++i){
				if(flower_cnt != PUZ_FLOWER_COUNT_UNKOWN && flower_cnt != i) continue;
				auto disp = string(pos_cnt - i, PUZ_EMPTY) + string(i, PUZ_FLOWER);
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

	boost::sort(m_fb_info, [this](const puz_fb_info& info1, const puz_fb_info& info2){
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
		return m_game->m_fb_info.size() - m_fb_index + m_unbalanced; 
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
	auto& info = m_game->m_fb_info[m_fb_index];
	auto& range = info.m_range;
	auto& disp = info.m_disps[i];

	auto ub = m_unbalanced;
	for(int k = 0; k < disp.size(); ++k){
		auto& p = range[k];
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
	return ++m_fb_index != m_game->m_fb_info.size() || m_unbalanced == 0;
}

void puz_state::gen_children(list<puz_state>& children) const
{
	int sz = m_game->m_fb_info[m_fb_index].m_disps.size();
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

void solve_puz_Gardener()
{
	using namespace puzzles::Gardener;
	solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
		"Puzzles\\Gardener.xml", "Puzzles\\Gardener.txt", solution_format::GOAL_STATE_ONLY);
}