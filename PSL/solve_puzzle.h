#pragma once

#include "AIhelper.h"

enum class solution_format
{
	ALL_STATES,
	GOAL_STATE_ONLY,
	MOVES_ONLY,
	MOVES_ONLY_SINGLE_LINE,
	CUSTOM,
};

template<class puz_game, class puz_state, class puz_solver>
void solve_puzzle(const string& fn_in, const string& fn_out,
				  solution_format fmt = solution_format::ALL_STATES,
				  function<void(ostream&, const list<puz_state>&)> dumper = {})
{
	list<puz_game> games;

	load_xml(games, fn_in);
	ofstream out(fn_out);

	for(const puz_game& game : games){
		util::high_resolution_timer t;
		puz_state sstart(game);
		list<puz_state> spath;
		//out << "Start state:" << endl << sstart << endl;
		out << "Level " << game.m_id << endl;
		cout << "Level " << game.m_id << endl;
		bool found;
		size_t vert_num;
		boost::tie(found, vert_num) = puz_solver::find_solution(sstart, spath);
		if(found){
			out << "Sequence of moves: ";
			if(fmt != solution_format::MOVES_ONLY_SINGLE_LINE)
				out << endl;
			if(fmt == solution_format::CUSTOM)
				dumper(out, spath);
			else if(fmt == solution_format::GOAL_STATE_ONLY)
				out << spath.back();
			else{
				for(puz_state& s : spath){
					if(fmt == solution_format::MOVES_ONLY || fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
						s.dump_move(out);
					else
						out << s;
				}
				if(fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
					out << endl;
			}
			out << "Number of moves: " << spath.size() - 1 << endl;
		}
		out << "Number of vertices examined: " << vert_num << endl;
		out << t.elapsed() << " [s]" << endl;
		cout << t.elapsed() << " [s]" << endl;
		out << endl;
	}
}

template<class puz_game, class puz_state, class puz_solver_full>
void solve_puzzle_full(const string& fn_in, const string& fn_out,
				  solution_format fmt = solution_format::ALL_STATES,
				  function<void(ostream&, const vector<puz_state>&)> dumper = {})
{
	list<puz_game> games;

	load_xml(games, fn_in);
	ofstream out(fn_out);

	for(const puz_game& game : games){
		util::high_resolution_timer t;
		puz_state sstart(game);
		vector<vector<puz_state>> spaths;
		//out << "Start state:" << endl << sstart << endl;
		out << "Level " << game.m_id << endl;
		cout << "Level " << game.m_id << endl;
		bool found;
		size_t vert_num;
		boost::tie(found, vert_num) = puz_solver_full::find_solution(sstart, spaths);
		for(int i = 0; i < spaths.size(); ++i){
			auto& spath = spaths[i];
			out << format("Solution %d\n") % i;
			out << "Sequence of moves: ";
			if(fmt != solution_format::MOVES_ONLY_SINGLE_LINE)
				out << endl;
			if(fmt == solution_format::CUSTOM)
				dumper(out, spath);
			else if(fmt == solution_format::GOAL_STATE_ONLY)
				out << spath.back();
			else{
				for(puz_state& s : spath){
					if(fmt == solution_format::MOVES_ONLY || fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
						s.dump_move(out);
					else
						out << s;
				}
				if(fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
					out << endl;
			}
			out << "Number of moves: " << spath.size() - 1 << endl;
		}
		out << "Number of vertices examined: " << vert_num << endl;
		out << t.elapsed() << " [s]" << endl;
		cout << t.elapsed() << " [s]" << endl;
		out << endl;
	}
}