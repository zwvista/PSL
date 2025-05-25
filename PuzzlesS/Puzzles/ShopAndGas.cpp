#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 10/Shop & Gas

    Summary
    A Hard day at shopping!

    Description
    1. In Shop & Gas you take the typical day at shopping. By the way:
       you just bought a new hyper-ecological car.
    2. This car goes on a ultra-green combustible, which is the saviour
       of the environment. It costs close to zero, it does not pollute
       and is found in abundance everywhere.
    3. The only small problem is that the car consumes about 10 liter
       per Km. Yes, that's a problem.
    4. So while shopping you have to constantly refuel your car. Thus,
       Shop & Gas rules are as follows:
    5. You start from your house. Right away, you're low on fuel so you
       must pass a fuel station.
    6. All these prototype fuel stations are shaped like corners. Don't
       ask why. You just have to turn on those tiles.
    7. Each time you pass a gas station, you then have to go shopping.
    8. Shopping malls are a lot more consumer friendly and have straight
       roads. So you have to go straight on those tiles.
    9. After a shopping mall you are almost empty again. The next thing
       you must pass is a gas station. Then shopping, gas, etc.
    10.After you passed all the shopping malls and gas station, you have
       to go back to your house, forming a closed path.
    11.The last thing you have to pass before going back home, is a gas
       station.
*/

namespace puzzles::ShopAndGas{

#define PUZ_LINE_OFF        '0'
#define PUZ_LINE_ON            '1'
#define PUZ_HOME            'H'
#define PUZ_SHOP            'S'
#define PUZ_GAS                'G'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // ┐  ─  ┌  ┘  │  └
    12, 10, 6, 9, 5, 3,
};
// All these prototype fuel stations are shaped like corners.
const vector<int> linesegs_all_gas = {
    // └  ┌  ┐  ┘
    3, 6, 12, 9,
};
// Shopping malls have straight roads.
const vector<int> linesegs_all_shop = {
    // │  ─
    5, 10,
};

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_dot_count;
    map<Position, char> m_pos2object;

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
            char ch = str[c];
            if (ch != ' ')
                m_pos2object[{r, c}] = ch;
        }
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
    bool make_move(const Position& p, int n);
    int check_dots(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_game->m_dot_count - m_finished.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<puz_dot>(g.m_dot_count), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            auto it = g.m_pos2object.find(p);
            if (it == g.m_pos2object.end())
                dt.push_back(lineseg_off);

            auto& linesegs_all2 = 
                it == g.m_pos2object.end() || it->second == PUZ_HOME ? linesegs_all :
                it->second == PUZ_GAS ? linesegs_all_gas :
                linesegs_all_shop;
            for (int lineseg : linesegs_all2)
                if ([&]{
                    for (int i = 0; i < 4; ++i)
                        // The line segment cannot lead to a position outside the board
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                    return true;
                }())
                    dt.push_back(lineseg);
        }

    check_dots(true);
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
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                if (!is_valid(p2))
                    continue;
                auto& dt = dots(p2);
                boost::remove_erase_if(dt, [&, i](int lineseg2) {
                    return is_lineseg_on(lineseg2, (i + 2) % 4) != is_lineseg_on(lineseg, i);
                });
                if (!init && dt.empty())
                    return 0;
            }
            m_finished.insert(p);
        }
        m_distance += newly_finished.size();
    }
}

bool puz_state::make_move(const Position& p, int n)
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
        char last_obj = ' ';
        for (int n = -1;;) {
            rng.erase(p2);
            int lineseg = dots(p2)[0];
            for (int i = 0; i < 4; ++i)
                if (is_lineseg_on(lineseg, i) && (i + 2) % 4 != n) {
                    p2 += offset[n = i];
                    break;
                }

            auto it = m_game->m_pos2object.find(p2);
            if (it != m_game->m_pos2object.end()) {
                char obj = it->second;
                if (last_obj != ' ' && (last_obj == PUZ_GAS) == (obj == PUZ_GAS))
                    return false;
                last_obj = obj;
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
    int n = boost::min_element(*this, [](const puz_dot& dt1, const puz_dot& dt2) {
        auto f = [](int sz) {return sz == 1 ? 1000 : sz;};
        return f(dt1.size()) < f(dt2.size());
    }) - begin();
    Position p(n / sidelen(), n % sidelen());
    auto& dt = dots(p);
    for (int i = 0; i < dt.size(); ++i) {
        children.push_back(*this);
        if (!children.back().make_move(p, i))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            auto it = m_game->m_pos2object.find(p);
            out << (it != m_game->m_pos2object.end() ? it->second : ' ')
                << (is_lineseg_on(dt[0], 1) ? '-' : ' ');
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

void solve_puz_ShopAndGas()
{
    using namespace puzzles::ShopAndGas;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ShopAndGas.xml", "Puzzles/ShopAndGas.txt", solution_format::GOAL_STATE_ONLY);
}
