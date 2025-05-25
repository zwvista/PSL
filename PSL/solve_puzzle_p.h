#pragma once

#include "PSLhelper.h"

enum class solution_format
{
    ALL_STATES,
    GOAL_STATE_ONLY,
    MOVES_ONLY,
    MOVES_ONLY_SINGLE_LINE,
    CUSTOM_STATES,
    CUSTOM_SOLUTIONS,
};

template<typename T>
concept puz_game_solve_puzzle = puz_game_load_xml<T> && requires(const T& t) {
    { t.m_id } -> same_as<const string&>;
};

template<typename T>
concept puz_state_format_to_string = requires(const T& t, back_insert_iterator<string> it) {
    { t.format_to_string(it) } -> same_as<void>;
};

template<typename T, typename G>
concept puz_state_solve_puzzle = puz_game_solve_puzzle<G> && puz_state_format_to_string<T> &&
    requires(const G& g, const T& t, ostream& out) {
    T{ g };
    { t.dump_move(out) } -> same_as<void>;
};

template<typename T, typename S>
concept puz_solver_solve_puzzle = requires(const T& t, const S& sstart, list<list<S>>& state_paths) {
    { t.find_solution(sstart, state_paths) } -> same_as<pair<bool, size_t>>;
};

template<
    puz_game_solve_puzzle puz_game,
    puz_state_solve_puzzle<puz_game> puz_state,
    puz_solver_solve_puzzle<puz_state> puz_solver
>
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
        // println(out, "Start state:\n{}", sstart);
        println(out, "Level {}", game.m_id);
        println("Level {}", game.m_id);
        const auto [found, vert_num] = puz_solver::find_solution(sstart, spaths);
        if (fmt == solution_format::CUSTOM_SOLUTIONS)
            solutions_dumper(out, spaths);
        else {
            int i = 1;
            for (auto& spath : spaths) {
                if (spaths.size() > 1)
                    println(out, "Solution {}:", i++);
                print(out, "Sequence of moves: ");
                if (fmt != solution_format::MOVES_ONLY_SINGLE_LINE)
                    println(out);
                if (fmt == solution_format::CUSTOM_STATES)
                    states_dumper(out, spath);
                else if (fmt == solution_format::GOAL_STATE_ONLY)
                    print(out, "{}", spath.back());
                else {
                    for (puz_state& s : spath) {
                        if (fmt == solution_format::MOVES_ONLY || fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
                            s.dump_move(out);
                        else
                            print(out, "{}", s);
                    }
                    if (fmt == solution_format::MOVES_ONLY_SINGLE_LINE)
                        println(out);
                }
                println(out, "Number of moves: {}", spath.size() - 1);
            }
        }
        println(out, "Number of vertices examined: {}", vert_num);
        println(out, "{} [s]", t.elapsed());
        println("{} [s]", t.elapsed());
        println(out);
    }
}

namespace std {
    template<puz_state_format_to_string puz_state>
    struct formatter<puz_state> : formatter<string> {
        auto format(const puz_state& p, format_context& ctx) const {
            string temp;
            p.format_to_string(back_inserter(temp));
            return formatter<string>::format(temp, ctx);
        }
    };
}
