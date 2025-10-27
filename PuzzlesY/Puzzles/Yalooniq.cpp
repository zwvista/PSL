#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 6/Yalooniq

    Summary
    Loops, Arrows and Squares

    Description
    1. The goal is to draw a single Loop on the board, similarly to LineSweeper.
    2. The Loop must go through ALL the available tiles on the board.
    3. The available tiles on which the Loop must go are the ones without
       Arrows and also not containing Squares.
    4. It is up to you to find the Squares, which are pointed at by the Arrows!
    5. The numbers beside the Arrows tell you how many Squares are present
       in that direction, from that point.
    6. The Squares can't touch horizontally or vertically.
    7. Lastly, please keep in mind that if there aren't Arrows pointing to 
       a tile, that tile can contain a Square too!
*/

namespace puzzles::Yalooniq{

constexpr auto PUZ_LINE_OFF = '0';
constexpr auto PUZ_LINE_ON = '1';
constexpr auto PUZ_SQUARE_OFF = '0';
constexpr auto PUZ_SQUARE_ON = '1';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

constexpr int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
constexpr string_view square_dirs = "^>v<";

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_square_info
{
    int m_num;
    char m_dir;
    vector<Position> m_rng;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, puz_square_info> m_pos2info;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_dot_count(m_sidelen * m_sidelen)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            auto s = str.substr(2 * c, 2);
            if (s != "  ") {
                auto& info = m_pos2info[{r, c}];
                info.m_num = s[0] - '0';
                info.m_dir = s[1];
            }
        }
    }

    for (auto& [p, info] : m_pos2info) {
        auto& os = offset[square_dirs.find(info.m_dir)];
        for (auto p2 = p + os; is_valid(p2); p2 += os)
            if (!m_pos2info.contains(p2))
                info.m_rng.push_back(p2);

        auto perm = string(info.m_rng.size() - info.m_num, PUZ_SQUARE_OFF) + string(info.m_num, PUZ_SQUARE_ON);
        do
            info.m_perms.push_back(perm);
        while (boost::next_permutation(perm));
    }
}

using puz_dot = vector<int>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_matches, m_dots) < tie(x.m_matches, x.m_dots);
    }
    bool make_move_square(const Position& p, int n);
    void make_move_square2(const Position& p, int n);
    bool make_move_dot(const Position& p, int n);
    int find_matches(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_game->m_dot_count * 4 - m_finished.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_dot> m_dots;
    map<Position, vector<int>> m_matches;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dot_count, {lineseg_off}), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (g.m_pos2info.contains(p))
                continue;

            auto& dt = dots(p);
            for (int lineseg : linesegs_all)
                if ([&]{
                    for (int i = 0; i < 4; ++i) {
                        if (!is_lineseg_on(lineseg, i))
                            continue;
                        auto p2 = p + offset[i];
                        // A line segment cannot go beyond the boundaries of the board
                        // or cover any arrow cell
                        if (!is_valid(p2) || g.m_pos2info.contains(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& [p, info] : g.m_pos2info) {
        for (int i = 0; i < 4; ++i)
            m_finished.emplace(p, i);
        auto& perm_ids = m_matches[p];
        perm_ids.resize(info.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& info = m_game->m_pos2info.at(p);
        auto& rng = info.m_rng;
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = info.m_perms[id];
            for (int i = 0; i < rng.size(); ++i) {
                char ch = perm[i];
                const auto& dt = dots(rng[i]);
                if (ch == PUZ_SQUARE_OFF && dt.size() == 1 && dt[0] == lineseg_off ||
                    ch == PUZ_SQUARE_ON && dt[0] != lineseg_off)
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_square2(p, perm_ids.front()), 1;
            }
    }
    return 2;
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<pair<Position, int>> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                for (int i = 0; i < 4; ++i)
                    if (!m_finished.contains({p, i}) && (
                        boost::algorithm::all_of(dt, [=](int lineseg) {
                        return is_lineseg_on(lineseg, i);
                    }) || boost::algorithm::all_of(dt, [=](int lineseg) {
                        return !is_lineseg_on(lineseg, i);
                    })))
                        newly_finished.emplace(p, i);
            }

        if (newly_finished.empty())
            return n;

        n = 1;
        for (const auto& kv : newly_finished) {
            auto& [p, i] = kv;
            auto& dt = dots(p);
            if (dt.empty())
                return 0;
            int lineseg = dt[0];
            bool is_square = lineseg == lineseg_off && !m_game->m_pos2info.contains(p);
            auto p2 = p + offset[i];
            if (is_valid(p2)) {
                auto& dt2 = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt2, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                    });
                if (is_square && !m_game->m_pos2info.contains(p2))
                    boost::remove_erase(dt, lineseg_off);
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(kv);
        }
        m_distance += newly_finished.size();
    }
}

void puz_state::make_move_square2(const Position& p, int n)
{
    auto& info = m_game->m_pos2info.at(p);
    auto& rng = info.m_rng;
    auto& perm = info.m_perms[n];
    for (int i = 0; i < rng.size(); ++i) {
        const auto& p2 = rng[i];
        auto& dt = dots(p2);
        if (perm[i] == PUZ_SQUARE_OFF) {
            if (dt[0] == lineseg_off)
                dt.erase(dt.begin());
        } else
            dt = {lineseg_off};
    }
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move_square(const Position& p, int n)
{
    m_distance = 0;
    make_move_square2(p, n);
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
    auto& dt = dots(p);
    dt = {dt[n]};
    return check_dots(false) != 0 && check_loop();
}

bool puz_state::check_loop() const
{
    set<Position> rng;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (dt.size() == 1 && dt[0] != lineseg_off)
                rng.insert(p);
        }

    while (!rng.empty()) {
        auto p = *rng.begin(), p2 = p;
        for (int n = -1;;) {
            rng.erase(p2);
            int lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                // proceed only if the line segment does not revisit the previous position
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // and we are supposed to have exhausted the line segments
                return rng.empty();
            if (!rng.contains(p2))
                break;
        }
    }
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
            if (!children.emplace_back(*this).make_move_square(p, n))
                children.pop_back();
    } else {
        int n = boost::min_element(m_dots, [](const puz_dot& dt1, const puz_dot& dt2) {
            auto f = [](int sz) {return sz == 1 ? 1000 : sz;};
            return f(dt1.size()) < f(dt2.size());
        }) - m_dots.begin();
        Position p(n / sidelen(), n % sidelen());
        auto& dt = dots(p);
        for (int i = 0; i < dt.size(); ++i)
            if (!children.emplace_back(*this).make_move_dot(p, i))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            if (auto it = m_game->m_pos2info.find(p); it != m_game->m_pos2info.end()) {
                auto& info = it->second;
                out << info.m_num << info.m_dir;
            } else
                out << (dt[0] == lineseg_off ? "S " :
                    is_lineseg_on(dt[0], 1) ? " -" : "  ");
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_Yalooniq()
{
    using namespace puzzles::Yalooniq;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Yalooniq.xml", "Puzzles/Yalooniq.txt", solution_format::GOAL_STATE_ONLY);
}
