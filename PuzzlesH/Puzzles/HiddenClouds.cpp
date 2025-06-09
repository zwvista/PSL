#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 7/Hidden Clouds

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
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_hint
{
    int m_num = 0;
    vector<int> m_box_ids;
    bool operator<(const puz_hint& x) const {
        return tie(m_num, m_box_ids) < tie(x.m_num, x.m_box_ids);
    }
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
    map<Position, puz_hint> m_pos2hint;
    vector<puz_box> m_boxes;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            char ch = str[c];
            if (ch != PUZ_SPACE)
                m_pos2hint[{r, c}].m_num = ch - '0';
        }
    }
    // 2. Clouds have a square form (even of one single tile)
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (int h = 1; h <= m_sidelen - r; ++h)
                for (int w = 1; w <= m_sidelen - c; ++w) {
                    Position box_sz(h - 1, w - 1);
                    Position tl(r, c), br = tl + box_sz;
                    map<Position, int> pos2num;
                    for (auto& [p, hint]: m_pos2hint) {
                        int n = 0;
                        for (auto& os : offset) {
                            auto p2 = p + os;
                            if (p2.first >= tl.first && p2.first <= br.first &&
                                p2.second >= tl.second && p2.second <= br.second)
                                ++n;
                        }
                        // 4. Numbers indicate the total number of clouds tiles in the tile itself
                        // and in the four tiles around it (up down left right)
                        if (n > hint.m_num)
                            goto next_box;
                        if (n > 0)
                            pos2num[p] = n;
                    }
                    {
                        int n = m_boxes.size();
                        auto& o = m_boxes.emplace_back();
                        o.m_box = {tl, br};
                        o.m_pos2num = pos2num;
                        for (auto& [p, n2] : pos2num)
                            m_pos2hint.at(p).m_box_ids.push_back(n);
                    }
                next_box:;
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

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the hint
    // value: the hint
    map<Position, puz_hint> m_matches;
    vector<int> m_area2num;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    m_matches = g.m_pos2hint;
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto f = [&](const Position& p) {
        if (!is_valid(p)) return false;
        char ch = cells(p);
        return ch != PUZ_SPACE && ch != PUZ_EMPTY;
    };
    
    for (auto& [p, hint] : m_matches) {
        boost::remove_erase_if(hint.m_box_ids, [&](int id) {
            auto& [box, pos2num] = m_game->m_boxes[id];
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            // 2. Clouds can't touch each other horizontally or vertically.
            for (int r = box.first.first; r <= box.second.first; ++r) {
                Position p1(r, box.first.second - 1), p2(r, box.second.second + 1);
                if (f(p1) || f(p2))
                    return true;
            }
            for (int c = box.first.second; c <= box.second.second; ++c) {
                Position p1(box.first.first - 1, c), p2(box.second.first + 1, c);
                if (f(p1) || f(p2))
                    return true;
            }
        });

        if (!init)
            switch(hint.m_box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(hint.m_box_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [box, pos2num] = m_game->m_boxes[n];
    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c)
            cells({r, c}) = PUZ_CLOUD;
    // 2. Clouds can't touch each other horizontally or vertically.
    auto f = [&](const Position& p) {
        if (is_valid(p))
            cells(p) = PUZ_EMPTY;
    };
    for (int r = box.first.first; r <= box.second.first; ++r)
        f({r, box.first.second - 1}), f({r, box.second.second + 1});
    for (int c = box.first.second; c <= box.second.second; ++c)
        f({box.first.first - 1, c}), f({box.second.first + 1, c});
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
    auto& [p, hint] = *boost::min_element(m_matches, [](
        const pair<const Position, puz_hint>& kv1,
        const pair<const Position, puz_hint>& kv2) {
        return kv1.second.m_box_ids.size() < kv2.second.m_box_ids.size();
    });
    for (int n : hint.m_box_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            out << format("{:02}", cells(p)) << " ";
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
