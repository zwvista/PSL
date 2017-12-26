#pragma once

#include "AIhelper.h"

enum class solution_format
{
    ALL_STATES,
    GOAL_STATE_ONLY,
    MOVES_ONLY,
    MOVES_ONLY_SINGLE_LINE,
    CUSTOM_STATES,
    CUSTOM_SOLUTIONS,
};

template<class puz_game, class puz_state, class puz_solver>
void solve_puzzle(const string& fn_in, const string& fn_out,
                  solution_format fmt = solution_format::ALL_STATES,
                  function<void(ostream&, const list<puz_state>&)> states_dumper = {},
                  function<void(ostream&, const list<list<puz_state>>&)> solutions_dumper = {})
{
    list<puz_game> games;

    load_xml(games, fn_in);
    ofstream out(fn_out);

    for (const puz_game& game : games) {
        util::high_resolution_timer t;
        puz_state sstart(game);
        list<list<puz_state>> spaths;
        //out << "Start state:" << endl << sstart << endl;
        out << "Level " << game.m_id << endl;
        cout << "Level " << game.m_id << endl;
        bool found;
        size_t vert_num;
        boost::tie(found, vert_num) = puz_solver::find_solution(sstart, spaths);
        if (fmt == solution_format::CUSTOM_SOLUTIONS)
            solutions_dumper(out, spaths);
        else {
            int i = 1;
            for (auto& spath : spaths) {
                if (spaths.size() > 1)
                    out << format("Solution %d:\n") % i++;
                out << "Sequence of moves: ";
                if (fmt != solution_format::MOVES_ONLY_SINGLE_LINE)
                    out << endl;
                if (fmt == solution_format::CUSTOM_STATES)
                    states_dumper(out, spath);
                else if (fmt == solution_format::GOAL_STATE_ONLY)
                    out << spath.back();
                else {
                    for (puz_state& s : spath) {
                        if (fmt == solution_format::MOVES_ONLY || fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
                            s.dump_move(out);
                        else
                            out << s;
                    }
                    if (fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
                        out << endl;
                }
                out << "Number of moves: " << spath.size() - 1 << endl;
            }
        }
        out << "Number of vertices examined: " << vert_num << endl;
        out << t.elapsed() << " [s]" << endl;
        cout << t.elapsed() << " [s]" << endl;
        out << endl;
    }
}
