#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 6/Yalooniq

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

#define PUZ_LINE_OFF        '0'
#define PUZ_LINE_ON            '1'
#define PUZ_SQUARE_OFF        '0'
#define PUZ_SQUARE_ON        '1'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
const string square_dirs = "^>v<";

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

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
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            auto s = str.substr(2 * c, 2);
            if (s != "  ") {
                auto& info = m_pos2info[{r, c}];
                info.m_num = s[0] - '0';
                info.m_dir = s[1];
            }
        }
    }

    for (auto& kv : m_pos2info) {
        auto& p = kv.first;
        auto& info = kv.second;

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

typedef vector<int> puz_dot;

struct puz_state : vector<puz_dot>
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_dot& dots(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move_square(const Position& p, int n);
    void make_move_square2(const Position& p, int n);
    bool make_move_line(const Position& p, int n);
    int find_matches(bool init);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count, {lineseg_off}), m_game(&g)
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
                        // The line segment cannot lead to a position
                        // outside the board or cover any arrow cell
                        if (!is_valid(p2) || g.m_pos2info.contains(p2))
                            return false;
                    }
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    for (auto& kv : g.m_pos2info) {
        auto& p = kv.first;
        auto& info = kv.second;
        m_finished.insert(p);
        auto& perm_ids = m_matches[p];
        perm_ids.resize(info.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
    check_dots(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

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
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                const auto& dt = dots(p);
                if (dt.size() == 1 && !m_finished.contains(p))
                    newly_finished.insert(p);
            }

        if (newly_finished.empty())
            return n;

        n = 1;
        for (const auto& p : newly_finished) {
            int lineseg = dots(p)[0];
            bool is_square = lineseg == lineseg_off && !m_game->m_pos2info.contains(p);
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                if (!is_valid(p2))
                    continue;
                auto& dt = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (is_square && !m_game->m_pos2info.contains(p2))
                    boost::remove_erase(dt, lineseg_off);
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(p);
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
    }
}

bool puz_state::make_move_line(const Position& p, int n)
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
                // go ahead if the line segment does not lead a way back
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }
            if (p2 == p)
                // we have a loop here,
                // so we should have exhausted the line segments 
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
        auto& kv = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : kv.second) {
            children.push_back(*this);
            if (!children.back().make_move_square(kv.first, n))
                children.pop_back();
        }
    } else {
        int n = boost::min_element(*this, [](const puz_dot& dt1, const puz_dot& dt2) {
            auto f = [](int sz) {return sz == 1 ? 1000 : sz;};
            return f(dt1.size()) < f(dt2.size());
        }) - begin();
        Position p(n / sidelen(), n % sidelen());
        auto& dt = dots(p);
        for (int i = 0; i < dt.size(); ++i) {
            children.push_back(*this);
            if (!children.back().make_move_line(p, i))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            auto it = m_game->m_pos2info.find(p);
            if (it != m_game->m_pos2info.end()) {
                auto& info = it->second;
                out << info.m_num << info.m_dir;
            } else
                out << (dt[0] == lineseg_off ? "S " :
                    is_lineseg_on(dt[0], 1) ? " -" : "  ");
        }
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0; c < sidelen(); ++c)
            // draw vert-lines
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
