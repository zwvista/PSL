#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 5/Lakes and Meadows

    Summary
    Lakes and Meadows

    Description
    1. Some of the cells have lakes in them.
    2. The aim is to divide the grid into square blocks such that each
       block contains exactly one lake.
*/

namespace puzzles::LakesAndMeadows{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_LAKE = 'L';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

// top-left and bottom-right
using puz_box = pair<Position, Position>;
    
struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, vector<puz_box>> m_pos2boxes;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (str[c] != ' ')
                m_pos2boxes[{r, c}];
    }

    for (auto& [pn, boxes] : m_pos2boxes)
        for (int n = 1; n < m_sidelen; ++n) {
            Position box_sz(n - 1, n - 1);
            auto p2 = pn - box_sz;
            //   - - - -
            //  |       |
            //         - - - -
            //  |     |L|     |
            //   - - - -
            //        |       |
            //         - - - -
            for (int r = p2.first; r <= pn.first; ++r)
                for (int c = p2.second; c <= pn.second; ++c) {
                    Position tl(r, c), br = tl + box_sz;
                    if (tl.first >= 0 && tl.second >= 0 &&
                        br.first < m_sidelen && br.second < m_sidelen &&
                        // All the other lakes should not be inside this box
                        boost::algorithm::none_of(m_pos2boxes, [&](
                            const pair<const Position, vector<puz_box>>& kv) {
                        auto& p = kv.first;
                        return p != pn &&
                            p.first >= tl.first && p.second >= tl.second &&
                            p.first <= br.first && p.second <= br.second;
                    }))
                        boxes.emplace_back(tl, br);
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& [p, boxes] : g.m_pos2boxes) {
        auto& box_ids = m_matches[p];
        box_ids.resize(boxes.size());
        boost::iota(box_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    set<Position> spaces;
    for (auto& [p, box_ids] : m_matches) {
        auto& boxes = m_game->m_pos2boxes.at(p);
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& box = boxes[id];
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    if (cells({r, c}) != PUZ_SPACE)
                        return true;
            return false;
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, box_ids.front()) ? 1 : 0;
            }

        // pruning
        for (int id : box_ids) {
            auto& box = boxes[id];
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    spaces.emplace(r, c);
        }
    }
    // All the boxes added up should cover all the remaining spaces
    return boost::count(m_cells, PUZ_SPACE) == spaces.size() ? 2 : 0;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& box = m_game->m_pos2boxes.at(p)[n];

    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c)
            cells({r, c}) = m_ch, ++m_distance;
    ++m_ch;
    m_matches.erase(p);
    return is_goal_state() || !m_matches.empty();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
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
    for (int n : box_ids)
        if (children.push_back(*this); !children.back().make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2boxes.find(p); it == m_game->m_pos2boxes.end())
                out << " .";
            else
                out << ' ' << PUZ_LAKE;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_LakesAndMeadows()
{
    using namespace puzzles::LakesAndMeadows;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LakesAndMeadows.xml", "Puzzles/LakesAndMeadows.txt", solution_format::GOAL_STATE_ONLY);
}
