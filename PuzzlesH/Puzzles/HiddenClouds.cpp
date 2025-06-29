#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Hidden Clouds

    Summary
    Hide and Seek in the sky

    Description
    1. Try to find the clouds.
    2. Clouds have a square form (even of one single tile) and can't touch
       each other horizontally or vertically.
    3. Clouds of the same size cannot see each other horizontally or vertically,
       that is, there must be other Clouds between them
       (horizontally or vertically).
    4. Numbers indicate the total number of clouds tiles in the tile itself
       and in the four tiles around it (up down left right)
*/

namespace puzzles::HiddenClouds{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_CLOUD = 'C';

constexpr Position offset[] = {
    {0, 0},         // o
    {-1, 0},       // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},       // w
};

struct puz_box
{
    // top-left and bottom-right
    pair<Position, Position> m_box;
    map<Position, int> m_pos2num;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    map<Position, vector<int>> m_pos2boxids;
    vector<puz_box> m_boxes;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            Position p(r, c);
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch - '0', m_pos2boxids[p];
        }
    }
    // 2. Clouds have a square form (even of one single tile)
    for (int r1 = 0; r1 < m_sidelen; ++r1)
        for (int c1 = 0; c1 < m_sidelen; ++c1)
            for (int sz = 1; sz <= min(m_sidelen - r1, m_sidelen - c1); ++sz) {
                Position tl(r1, c1), br = tl + Position(sz - 1, sz - 1);
                auto& [r2, c2] = br;
                if (map<Position, int> pos2num; [&] {
                    for (auto& [p, num] : m_pos2num) {
                        int n = boost::accumulate(offset, 0, [&](int acc, const Position& os) {
                            auto [r, c] = p + os;
                            return acc + (r >= r1 && r <= r2 && c >= c1 && c <= c2 ? 1 : 0);
                        });
                        // 4. Numbers indicate the total number of clouds tiles in the tile itself
                        // and in the four tiles around it (up down left right)
                        if (n > num)
                            return false;
                        if (n > 0)
                            pos2num[p] = n;
                    }
                    return true;
                }()) {
                    int n = m_boxes.size();
                    m_boxes.emplace_back(pair{tl, br}, pos2num);
                    for (auto& [p, _1] : pos2num)
                        m_pos2boxids.at(p).push_back(n);
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
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { 
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches); 
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    char& sizes(const Position& p) { return m_sizes[p.first * sidelen() + p.second]; }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells, m_sizes;
    // key: the position of the hint
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    // key: the position of the hint
    // value: the value of the hint
    map<Position, int> m_pos2num;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_sizes(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    for (auto& [p, num] : g.m_pos2num)
        if ((m_pos2num[p] = num) != 0)
            m_matches[p] = g.m_pos2boxids.at(p);
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto f = [&](const Position& p) {
        if (!is_valid(p)) return false;
        char ch = cells(p);
        return ch != PUZ_SPACE && ch != PUZ_EMPTY;
    };

    for (auto& [p, box_ids] : m_matches) {
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& [box, pos2num] = m_game->m_boxes[id];
            auto& [tl, br] = box;
            auto& [r1, c1] = tl;
            auto& [r2, c2] = br;
            int sz = r2 - r1 + 1;
            for (int r = r1; r <= r2; ++r)
                for (int c = c1; c <= c2; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            // 2. Clouds can't touch each other horizontally or vertically.
            for (int r = r1; r <= r2; ++r) {
                Position p1(r, c1 - 1), p2(r, c2 + 1);
                if (f(p1) || f(p2))
                    return true;
            }
            for (int c = c1; c <= c2; ++c) {
                Position p1(r1 - 1, c), p2(r2 + 1, c);
                if (f(p1) || f(p2))
                    return true;
            }
            // 3. Clouds of the same size cannot see each other horizontally or vertically
            auto ff = [&](const Position& p) {
                char ch = sizes(p);
                return ch == PUZ_SPACE ? 1 : ch == sz + '0' ? 0 : 2;
            };
            set<int> s;
            for (int r = r1; r <= r2; ++r)
                s.insert([&] {
                    for (int c = c1 - 2; c >= 0; --c)
                        if (int n = ff({r, c}); n != 1)
                            return n;
                    return 1;
                }());
            if (!s.contains(2) && s.contains(0))
                return true;
            s.clear();
            for (int r = r1; r <= r2; ++r)
                s.insert([&] {
                    for (int c = c2 + 2; c < sidelen(); ++c)
                        if (int n = ff({r, c}); n != 1)
                            return n;
                    return 1;
                }());
            if (!s.contains(2) && s.contains(0))
                return true;
            s.clear();
            for (int c = c1; c <= c2; ++c)
                s.insert([&] {
                    for (int r = r1 - 2; r >= 0; --r)
                        if (int n = ff({r, c}); n != 1)
                            return n;
                    return 1;
                }());
            if (!s.contains(2) && s.contains(0))
                return true;
            s.clear();
            for (int c = c1; c <= c2; ++c)
                s.insert([&] {
                    for (int r = r2 + 2; r < sidelen(); ++r)
                        if (int n = ff({r, c}); n != 1)
                            return n;
                    return 1;
                }());
            if (!s.contains(2) && s.contains(0))
                return true;
            s.clear();
            // 4. Numbers indicate the total number of clouds tiles in the tile itself
            // and in the four tiles around it (up down left right)
            return boost::algorithm::any_of(pos2num, [&](const pair<const Position, int>& kv) {
                return kv.second > m_pos2num.at(kv.first);
            });
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(box_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [box, pos2num] = m_game->m_boxes[n];
    auto& [tl, br] = box;
    auto& [r1, c1] = tl;
    auto& [r2, c2] = br;
    int sz = r2 - r1 + 1;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c) {
            Position p(r, c);
            cells(p) = PUZ_CLOUD, sizes(p) = sz + '0';
        }
    // 2. Clouds can't touch each other horizontally or vertically.
    auto f = [&](const Position& p) {
        if (is_valid(p))
            cells(p) = PUZ_EMPTY;
    };
    for (int r = r1; r <= r2; ++r)
        f({r, c1 - 1}), f({r, c2 + 1});
    for (int c = c1; c <= c2; ++c)
        f({r1 - 1, c}), f({r2 + 1, c});

    for (auto& [p, box_ids] : m_matches)
        boost::remove_erase(box_ids, n);

    // 4. Numbers indicate the total number of clouds tiles in the tile itself
    // and in the four tiles around it (up down left right)
    for (auto& [p, i] : pos2num)
        if (m_matches.contains(p) && (m_pos2num.at(p) -= i) == 0)
            m_matches.erase(p), ++m_distance;
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, box_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : box_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << " --";
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << '|';
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << ' ';
            else
                out << it->second;
            out << cells({r, c});
        }
        println(out);
    }
    return out;
}

}

void solve_puz_HiddenClouds()
{
    using namespace puzzles::HiddenClouds;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/HiddenClouds.xml", "Puzzles/HiddenClouds.txt", solution_format::GOAL_STATE_ONLY);
}
