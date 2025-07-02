#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Light Connect Puzzle!

    Summary
    Connect Lamps and Batteries

    Description
*/

namespace puzzles::LightConnect{

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_pipe_on(int pipe, int d) { return (pipe & (1 << d)) != 0; }

const vector<int> pipes_lamp_battery = {
    // ╵(up) ╶(right) ╷(down) ╴(left)
    1, 2, 4, 8
};
const vector<int> pipes_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_pipe_count;
    vector<int> m_pipes;
    set<Position> m_batteries, m_lamps;

    puz_game(const vector<string>& strs, const xml_node& level);
    int& pipes(const Position& p) { return m_pipes[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_pipe_count(m_sidelen * m_sidelen)
    , m_pipes(m_pipe_count)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            int n = isdigit(ch) ? ch - '0' : ch <= 'F' ? ch - 'A' + 10 : 1 << (ch - 'G');
            Position p(r, c);
            pipes(p) = n;
            if (boost::algorithm::any_of_equal(pipes_lamp_battery, n))
                (isdigit(ch) ? m_lamps : m_batteries).insert(p);
        }
    }

}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int pipes(const Position& p) const { return m_pipes[p.first * sidelen() + p.second]; }
    int& pipes(const Position& p) { return m_pipes[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_pipes) < tie(x.m_matches, x.m_pipes);
    }
    bool make_move_hint(const Position& p, int n);
    void make_move_hint2(const Position& p, int n);
    bool make_move_dot(const Position& p, int n);
    int find_matches(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_game->m_pipe_count * 4 - m_finished.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<int> m_pipes;
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_pipes(g.m_pipes), m_game(&g)
{

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    //for (auto& [p, perm_ids] : m_matches) {
    //    auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
    //    boost::remove_erase_if(perm_ids, [&](int id) {
    //        auto& perm = perms[id];
    //        for (int i = 0; i < 8; ++i) {
    //            auto p2 = p + offset[i];
    //            int pipe = perm[i];
    //            if (!is_valid(p2)) {
    //                if (pipe != pipe_off)
    //                    return true;
    //            } else if (boost::algorithm::none_of_equal(dots(p2), pipe))
    //                return true;
    //        }
    //        return false;
    //    });

    //    if (!init)
    //        switch(perm_ids.size()) {
    //        case 0:
    //            return 0;
    //        case 1:
    //            return make_move_hint2(p, perm_ids.front()), 1;
    //        }
    //}
    return 2;
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<pair<Position, int>> newly_finished;
        //for (int r = 0; r < sidelen(); ++r)
        //    for (int c = 0; c < sidelen(); ++c) {
        //        Position p(r, c);
        //        const auto& dt = dots(p);
        //        for (int i = 0; i < 4; ++i)
        //            if (!m_finished.contains({p, i}) && (
        //                boost::algorithm::all_of(dt, [=](int pipe) {
        //                return is_pipe_on(pipe, i);
        //            }) || boost::algorithm::all_of(dt, [=](int pipe) {
        //                return !is_pipe_on(pipe, i);
        //            })))
        //                newly_finished.emplace(p, i);
        //    }

        if (newly_finished.empty())
            return n;

        n = 1;
        //for (const auto& kv : newly_finished) {
        //    auto& [p, i] = kv;
        //    int pipe = dots(p)[0];
        //    auto p2 = p + offset[i * 2];
        //    if (is_valid(p2)) {
        //        auto& dt = dots(p2);
        //        // The line segments in adjacent cells must be connected
        //        boost::remove_erase_if(dt, [=](int pipe2) {
        //            return is_pipe_on(pipe2, (i + 2) % 4) != is_pipe_on(pipe, i);
        //        });
        //        if (!init && dt.empty())
        //            return 0;
        //    }
        //    m_finished.insert(kv);
        //}
        //m_distance += newly_finished.size();
    }
}

void puz_state::make_move_hint2(const Position& p, int n)
{
    //auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    //for (int i = 0; i < 8; ++i) {
    //    auto p2 = p + offset[i];
    //    if (is_valid(p2))
    //        dots(p2) = {perm[i]};
    //}
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::check_loop() const
{
    //set<Position> rng;
    //for (int r = 0; r < sidelen(); ++r)
    //    for (int c = 0; c < sidelen(); ++c) {
    //        Position p(r, c);
    //        auto& dt = dots(p);
    //        if (dt.size() == 1 && dt[0] != pipe_off)
    //            rng.insert(p);
    //    }

    //bool has_branch = false;
    //while (!rng.empty()) {
    //    auto p = *rng.begin(), p2 = p;
    //    for (int n = -1;;) {
    //        rng.erase(p2);
    //        auto& pipe = dots(p2)[0];
    //        for (int i = 0; i < 4; ++i)
    //            // proceed only if the line segment does not revisit the previous position
    //            if (is_pipe_on(pipe, i) && (i + 2) % 4 != n) {
    //                p2 += offset[(n = i) * 2];
    //                break;
    //            }
    //        if (p2 == p)
    //            // we have a loop here,
    //            // and we are supposed to have exhausted the line segments
    //            return !has_branch && rng.empty();
    //        if (!rng.contains(p2)) {
    //            has_branch = true;
    //            break;
    //        }
    //    }
    //}
    return true;
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    make_move_hint2(p, n);
    for (;;) {
        int m;
        while ((m = find_matches(false)) == 1);
        if (m == 0)
            return false;
        m = check_dots(false);
        if (m != 1)
            return m == 2;
        if (!check_loop())
            return false;
    }
}

bool puz_state::make_move_dot(const Position& p, int n)
{
    m_distance = 0;
    //auto& dt = dots(p);
    //dt = {dt[n]};
    //int m = check_dots(false);
    //return m == 1 ? check_loop() : m == 2;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : perm_ids)
            if (children.push_back(*this); !children.back().make_move_hint(p, n))
                children.pop_back();
    } else {
        //int i = boost::min_element(m_dots, [&](const puz_dot& dt1, const puz_dot& dt2) {
        //    auto f = [](const puz_dot& dt) {
        //        int sz = dt.size();
        //        return sz == 1 ? 100 : sz;
        //    };
        //    return f(dt1) < f(dt2);
        //}) - m_dots.begin();
        //auto& dt = m_dots[i];
        //Position p(i / sidelen(), i % sidelen());
        //for (int n = 0; n < dt.size(); ++n)
        //    if (children.push_back(*this); !children.back().make_move_dot(p, n))
        //        children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        //for (int c = 0; c < sidelen(); ++c) {
        //    Position p(r, c);
        //    auto it = m_game->m_pos2num.find(p);
        //    out << char(it == m_game->m_pos2num.end() ? ' ' : it->second + '0')
        //        << (is_pipe_on(dots(p)[0], 1) ? '-' : ' ');
        //}
        //println(out);
        //if (r == sidelen() - 1) break;
        //for (int c = 0; c < sidelen(); ++c)
        //    // draw vertical lines
        //    out << (is_pipe_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_LightConnect()
{
    using namespace puzzles::LightConnect;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LightConnect.xml", "Puzzles/LightConnect.txt", solution_format::GOAL_STATE_ONLY);
}
