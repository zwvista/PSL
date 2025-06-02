#pragma once

#include "stdafx.h"

template<typename T>
concept puz_state_solver_idastar = copyable<T> && requires(const T & t, list<T>&children, const T & child, const T & t2)
{
    { t.is_goal_state() } -> same_as<bool>;
    { t.gen_children(children) } -> same_as<void>;
    { t.get_heuristic() } -> same_as<unsigned int>;
    { t.get_distance(child) } -> same_as<unsigned int>;
    { t < t2 } -> same_as<bool>;
};

template<puz_state_solver_idastar puz_state>
class puz_solver_idastar
{
    static bool dfs(unsigned int start_cost, const puz_state& cur,
        unsigned int cost_limit, unsigned int& next_cost_limit,
        list<puz_state>& spath, size_t& examined)
    {
        ++examined;
        unsigned int minimum_cost = start_cost + cur.get_heuristic();
        if (minimum_cost > cost_limit)
            return next_cost_limit = std::min(next_cost_limit, minimum_cost), false;
        if (cur.is_goal_state())
            return next_cost_limit = cost_limit, true;

        list<puz_state> children;
        cur.gen_children(children);
        for (const puz_state& child : children) {
            // full cycle checking
            if (boost::algorithm::any_of_equal(spath, child))
                continue;
            unsigned int new_start_cost = start_cost + cur.get_distance(child);
            spath.push_back(child);
            bool found = dfs(new_start_cost, child, cost_limit, next_cost_limit, spath, examined);
            if (found)
                return true;
            spath.pop_back();
        }

        return false;
    }

    static bool dfs2(unsigned int start_cost, puz_state cur,
        unsigned int cost_limit, unsigned int& next_cost_limit,
        list<puz_state>& spath, size_t& examined)
    {
        ++examined;
        unsigned int minimum_cost = start_cost + cur.get_heuristic();
        if (minimum_cost > cost_limit)
            return next_cost_limit = std::min(next_cost_limit, minimum_cost), false;
        if (cur.is_goal_state())
            return next_cost_limit = cost_limit, true;

        typedef boost::tuple<unsigned int, puz_state, list<puz_state> > StateInfo;
        vector<StateInfo> stack;
        list<puz_state> children;
        cur.gen_children(children);
        stack.emplace_back(start_cost, cur, children);
        while (!stack.empty()) {
            boost::tie(start_cost, cur, children) = stack.back();
            stack.pop_back();
            for (auto i = children.begin(); i != children.end();) {
                const puz_state& child = *i;
                // full cycle checking
                if (boost::range::find(spath, child) != spath.end()) {
                    ++i;
                    continue;
                }
                stack.emplace_back(start_cost, cur, list<puz_state>(++i, children.end()));
                spath.push_back(child);
                start_cost += cur.get_distance(child);
                cur = child;
                ++examined;
                minimum_cost = start_cost + cur.get_heuristic();
                if (minimum_cost > cost_limit) {
                    next_cost_limit = std::min(next_cost_limit, minimum_cost);
                    i = children.end();
                } else if (cur.is_goal_state())
                    return next_cost_limit = cost_limit, true;
                else {
                    children.clear();
                    cur.gen_children(children);
                    i = children.begin();
                }
            }
            if (spath.size() > 1)
                spath.pop_back();
        }
        return false;
    }

public:
    static pair<bool, size_t> find_solution(const puz_state& sstart, list<list<puz_state>>& spaths)
    {
        unsigned int inf = numeric_limits<unsigned int>::max();
        unsigned int cost_limit = sstart.get_heuristic(), next_cost_limit = inf;
        size_t examined = 0;
        list<puz_state> spath;
        spath.assign(1, sstart);
        for (;;) {
            println("{}", cost_limit);
            bool found = dfs(0, sstart, cost_limit, next_cost_limit, spath, examined);
            if (found) {
                spaths.push_back(spath);
                return {true, examined};
            }
            if (next_cost_limit == inf)
                return {false, examined};
            cost_limit = next_cost_limit, next_cost_limit = inf;
        }
    }
};
