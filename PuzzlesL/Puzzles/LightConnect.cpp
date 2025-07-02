#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Light Connect Puzzle!

    Summary
    Connect Lamps and Batteries

    Description
    1. Hint:Moves and secures several elements in the correct position.
    1. Warp: The Light appears moving to the opposite side.
    2. Fixing: Secures elements in the correct position.
*/

namespace puzzles::LightConnect{

constexpr auto PUZ_PIPE_OFF = '0';
constexpr auto PUZ_PIPE_1 = '1';
constexpr auto PUZ_PIPE_I = 'I';
constexpr auto PUZ_PIPE_L = 'L';
constexpr auto PUZ_PIPE_3 = '3';
constexpr auto PUZ_BATTERY_Y = 'Y';
constexpr auto PUZ_BATTERY_R = 'R';
constexpr auto PUZ_BATTERY_B = 'B';
constexpr auto PUZ_LAMP_Y = 'y';
constexpr auto PUZ_LAMP_R = 'r';
constexpr auto PUZ_LAMP_B = 'b';
constexpr auto PUZ_WARP_VERT = 'V';
constexpr auto PUZ_WARP_HORZ = 'H';
constexpr auto PUZ_FIXING = 'F';

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

constexpr int lineseg_off = 0;
inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const vector<vector<int>> linesegs_all = {
    {lineseg_off},
    // ╵(up) ╶(right) ╷(down) ╴(left)
    {1, 2, 4, 8},
    // │  ─
    {5, 10},
    // └  ┌  ┐  ┘
    {3, 6, 12, 9},
    // ┴  ├  ┬  ┤
    {11, 7, 14, 13},
};

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

using puz_dot = vector<int>;

struct puz_game
{
    string m_id;
    Position m_size;
    int m_dot_count;
    string m_cells;
    map<char, set<Position>> m_color2rng;
    vector<puz_dot> m_dots;
    map<int, set<Position>> m_area2jumpers;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size(), strs[0].length() / 2))
    , m_dot_count(rows() * cols())
{
    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            char ch = str[c * 2];
            m_cells.push_back(ch);
            switch (ch) {
            case PUZ_BATTERY_Y:
            case PUZ_BATTERY_R:
            case PUZ_BATTERY_B:
            case PUZ_LAMP_Y:
            case PUZ_LAMP_R:
            case PUZ_LAMP_B:
                m_color2rng[toupper(ch)].insert(p);
                break;
            case PUZ_WARP_HORZ:
                m_area2jumpers[r].insert(p);
                break;
            case PUZ_WARP_VERT:
                m_area2jumpers[c + rows()].insert(p);
                break;
            }

            char ch2 = str[c * 2 + 1];
            auto& dt = m_dots.emplace_back();
            if (ch == PUZ_WARP_HORZ || ch == PUZ_WARP_VERT || ch == PUZ_FIXING)
                dt = {isdigit(ch2) ? ch2 - '0' : ch2 - 'A' + 10};
            else {
                auto& linesegs_all2 = linesegs_all[
                    ch2 == PUZ_PIPE_1 ? 1 :
                    ch2 == PUZ_PIPE_I ? 2 :
                    ch2 == PUZ_PIPE_L ? 3 :
                    ch2 == PUZ_PIPE_3 ? 4 :
                    0];
                for (int lineseg : linesegs_all2)
                    if ([&] {
                        for (int i = 0; i < 4; ++i) {
                            if (!is_lineseg_on(lineseg, i))
                                continue;
                            auto p2 = p + offset[i];
                            // A line segment cannot go beyond the boundaries of the board
                            if (!is_valid(p2))
                                return false;
                        }
                        return true;
                    }())
                        dt.push_back(lineseg);
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * cols() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const { return m_dots < x.m_dots; }
    bool make_move_dot(const Position& p, int n);
    int check_dots(bool init);
    bool check_connected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count * 4 - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<puz_dot> m_dots;
    set<pair<Position, int>> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_dots(g.m_dots), m_game(&g)
{
    check_dots(true);
}

int puz_state::check_dots(bool init)
{
    int n = 2;
    for (;;) {
        set<pair<Position, int>> newly_finished;
        for (int r = 0; r < rows(); ++r)
            for (int c = 0; c < cols(); ++c) {
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
            int lineseg = dots(p)[0];
            auto p2 = p + offset[i];
            if (is_valid(p2)) {
                auto& dt = dots(p2);
                // The line segments in adjacent cells must be connected
                boost::remove_erase_if(dt, [=](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(kv);
        }
        m_distance += newly_finished.size();
    }
}

struct puz_state2 : Position
{
    puz_state2(const puz_state* s, const Position& p) : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    switch (auto g = *m_state->m_game; char ch = g.cells(*this)) {
    case PUZ_WARP_HORZ:
    case PUZ_WARP_VERT:
        for (int n = ch == PUZ_WARP_HORZ ? first : second + g.rows();
            auto& p2 : g.m_area2jumpers.at(n))
            if (p2 != *this) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        break;
    default:
        for (int i = 0; i < 4; ++i)
            if (boost::algorithm::any_of(m_state->dots(*this), [&](int lineseg) {
                return is_lineseg_on(lineseg, i);
            })) {
                auto p2 = *this + offset[i];
                children.push_back(*this);
                children.back().make_move(p2);
            }
        break;
    }
}

bool puz_state::check_connected() const
{
    for (auto& [ch, rng] : m_game->m_color2rng) {
        int i = m_game->m_cells.find(ch);
        auto smoves = puz_move_generator<puz_state2>::gen_moves(
            {this, {i / cols(), i % cols()}});
        if (boost::count_if(smoves, [&](const Position& p) {
            return toupper(m_game->cells(p)) == ch;
        }) != rng.size())
            return false;
    }
    return true;
}

bool puz_state::make_move_dot(const Position& p, int n)
{
    m_distance = 0;
    auto& dt = dots(p);
    dt = {dt[n]};
    int m = check_dots(false);
    return m == 1 ? check_connected() : m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int i = boost::min_element(m_dots, [&](const puz_dot& dt1, const puz_dot& dt2) {
        auto f = [](const puz_dot& dt) {
            int sz = dt.size();
            return sz == 1 ? 100 : sz;
        };
        return f(dt1) < f(dt2);
    }) - m_dots.begin();
    auto& dt = m_dots[i];
    Position p(i / cols(), i % cols());
    for (int n = 0; n < dt.size(); ++n)
        if (children.push_back(*this); !children.back().make_move_dot(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            out << m_game->cells(p) << (is_lineseg_on(dt[0], 1) ? '-' : ' ');
        }
        println(out);
        if (r == rows() - 1) break;
        for (int c = 0; c < cols(); ++c)
            // draw vertical lines
            out << (is_lineseg_on(dots({r, c})[0], 2) ? "| " : "  ");
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
