#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 3/SlitherLink

    Summary
    Draw a loop a-la-minesweeper!

    Description
    1. Draw a single looping path with the aid of the numbered hints. The path
       cannot have branches or cross itself.
    2. Each number in a tile tells you on how many of its four sides are touched
       by the path.
    3. For example:
    4. A 0 tells you that the path doesn't touch that square at all.
    5. A 1 tells you that the path touches that square ONLY one-side.
    6. A 3 tells you that the path does a U-turn around that square.
    7. There can't be tiles marked with 4 because that would form a single
       closed loop in it.
    8. Empty tiles can have any number of sides touched by that path.
*/

namespace puzzles::SlitherLink2{

constexpr auto PUZ_UNKNOWN = -1;
constexpr auto PUZ_LINE_OFF = '0';
constexpr auto PUZ_LINE_ON = '1';
constexpr auto PUZ_LINE_UNKNOWN = ' ';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<int, vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 1)
{
    for (int r = 0; r < m_sidelen - 1; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            char ch = str[c];
            m_pos2num[{r, c}] = ch != ' ' ? ch - '0' : PUZ_UNKNOWN;
        }
    }

    auto& perms_unknown = m_num2perms[PUZ_UNKNOWN];
    for (int i = 0; i < 4; ++i) {
        auto& perms = m_num2perms[i];
        auto perm = string(4 - i, PUZ_LINE_OFF) + string(i, PUZ_LINE_ON);
        do {
            perms.push_back(perm);
            perms_unknown.push_back(perm);
        } while(boost::next_permutation(perm));
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char line(const Position& p, bool is_vert) const {
        return (*this)[(p.first * sidelen() + p.second) * 2 + (is_vert ? 1 : 0)];
    }
    char& line(const Position& p, bool is_vert) {
        return (*this)[(p.first * sidelen() + p.second) * 2 + (is_vert ? 1 : 0)];
    }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen * 2, PUZ_LINE_UNKNOWN), m_game(&g)
{
    for (auto& kv : g.m_pos2num) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(g.m_num2perms.at(kv.second).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto& perms = m_game->m_num2perms.at(m_game->m_pos2num.at(p));
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            for (int i = 0; i < 4; ++i) {
                char ch1 = line(p + offset2[i], i % 2 == 0);
                char ch2 = perm[i];
                if (ch1 != PUZ_LINE_UNKNOWN && ch1 != ch2)
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms.at(m_game->m_pos2num.at(p))[n];
    for (int i = 0; i < 4; ++i)
        line(p + offset2[i], i % 2 == 0) = perm[i];

    ++m_distance;
    m_matches.erase(p);

    return check_loop();
}

bool puz_state::check_loop() const
{
    //set<Position> rng;
    //for(int r = 0; r < sidelen(); ++r)
    //    for (int c = 0; c < sidelen(); ++c) {
    //        Position p(r, c);
    //        for (bool b : {false, true})
    //            if (line(p, b) != PUZ_LINE_UNKNOWN)
    //                rng.insert(p);
    //    }

    //bool has_loop = false;
    //while(!rng.empty()) {
    //    auto p = *rng.begin(), p2 = p;
    //    for (int n = -1;;) {
    //        rng.erase(p2);
    //        auto& str = dots(p2)[0];
    //        for (int i = 0; i < 4; ++i)
    //            if (str[i] == PUZ_LINE_ON && (i + 2) % 4 != n) {
    //                p2 += offset[n = i];
    //                break;
    //            }
    //        if (p2 == p)
    //            if (has_loop)
    //                return false;
    //            else {
    //                has_loop = true;
    //                break;
    //            }
    //        if (!rng.contains(p2))
    //            break;
    //    }
    //}
    return true;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    for (;;) {
        int m;
        while ((m = find_matches(false)) == 1);
        if (m != 1)
            return m == 2;
        if (!check_loop())
            return false;
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
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
    for (int r = 0;; ++r) {
        // draw horz-lines
        for (int c = 0; c < sidelen(); ++c)
            out << (line({r, c}, false) == PUZ_LINE_ON ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-lines
            out << (line(p, true) == PUZ_LINE_ON ? '|' : ' ');
            if (c == sidelen() - 1) break;
            int n = m_game->m_pos2num.at(p);
            if (n == PUZ_UNKNOWN)
                out << ' ';
            else
                out << n;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_SlitherLink2()
{
    using namespace puzzles::SlitherLink2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SlitherLink.xml", "Puzzles/SlitherLink2.txt", solution_format::GOAL_STATE_ONLY);
}
