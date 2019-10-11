#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 1/Wildlife Park

    Summary
    One rises, the other falls

    Description
    1. At the last game of Wildlife Football, Lemurs accused Giraffes of cheating,
       Monkeys ran away with the coin after the toss and Lions ate the ball.
    2. So you're given the task to raise some fencing between the different species,
       while spirits cool down.
    3. Fences should encompass at least one animal of a certain species, and all
       animals of a certain species must be in the same enclosure.
    4. There can't be empty enclosures.
    5. Where three or four fences meet, a fence post is put in place. On the Park
       all posts are already marked with a dot.
*/

namespace puzzles::WildlifePark{

#define PUZ_SPACE            ' '
#define PUZ_POST             'O'

// n-e-s-w
// 0 means line is off in this direction
// 1,2,4,8 means line is on in this direction

inline bool is_lineseg_on(int lineseg, int d) { return (lineseg & (1 << d)) != 0; }

const int lineseg_off = 0;
const vector<int> linesegs_all = {
    // „¢  „Ÿ  „¡  „£  „   „¤
    12, 10, 6, 9, 5, 3,
};
const vector<int> linesegs_all3 = {
    // „¦  „§  „¨  „¥  „©
    14, 13, 11, 7, 15,
};

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    set<Position> m_posts;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2 + 1)
{
    for (int r = 0;; ++r) {
        auto& str_h = strs[r * 2];
        // posts
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2] == PUZ_POST)
                m_posts.insert({r, c});
        if (r == m_sidelen - 1) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
        }
    }
}

typedef vector<int> puz_dot;

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    const puz_dot& dots(const Position& p) const { return m_dots[p.first * sidelen() + p.second]; }
    puz_dot& dots(const Position& p) { return m_dots[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_dots, m_matches) < tie(x.m_dots, x.m_matches);
    }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<puz_dot> m_dots;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_dots(g.m_sidelen * g.m_sidelen)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            auto& dt = dots(p);
            bool is_dot3 = g.m_posts.count(p) != 0;
            if (!is_dot3 && r > 0 && c > 0 && r < sidelen() - 1 && c < sidelen() - 1)
                dt.push_back(lineseg_off);
            for (int lineseg : (is_dot3 ? linesegs_all3 : linesegs_all))
                if ([&] {
                    for (int i = 0; i < 4; ++i)
                        // The line segment cannot lead to a position outside the board
                        if (is_lineseg_on(lineseg, i) && !is_valid(p + offset[i]))
                            return false;
                        return true;
                }())
                    dt.push_back(lineseg);
        }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        int area_id = kv.first;
        auto& perm_ids = kv.second;

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0;; ++r) {
    //    // draw horz-walls
    //    for (int c = 0; c < sidelen(); ++c)
    //        out << (m_game->m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
    //    out << endl;
    //    if (r == sidelen()) break;
    //    for (int c = 0;; ++c) {
    //        Position p(r, c);
    //        // draw vert-walls
    //        out << (m_game->m_vert_walls.count(p) == 1 ? '|' : ' ');
    //        if (c == sidelen()) break;
    //        out << cells(p);
    //    }
    //    out << endl;
    //}
    return out;
}

}

void solve_puz_WildlifePark()
{
    using namespace puzzles::WildlifePark;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WildlifePark.xml", "Puzzles/WildlifePark.txt", solution_format::GOAL_STATE_ONLY);
}
