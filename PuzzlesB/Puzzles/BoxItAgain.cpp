#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 15/Box It Again

    Summary
    Harder Boxes

    Description
    1. Just like Box It Up, you have to divide the Board in Boxes (Rectangles).
    2. Each Box must contain one number and the number represents the area of
       that Box.
    3. However this time, some tiles can be left unboxed, the board isn't 
       entirely covered by boxes.
*/

namespace puzzles::BoxItAgain{

constexpr auto PUZ_SPACE = ' ';

struct puz_box_info
{
    // the area of the box
    int m_area;
    // top-left and bottom-right
    vector<pair<Position, Position>> m_boxes;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_box_info> m_pos2boxinfo;

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
                m_pos2boxinfo[{r, c}].m_area = ch - '0';
        }
    }

    for (auto& [pn, info] : m_pos2boxinfo) {
        int box_area = info.m_area;
        auto& boxes = info.m_boxes;

        for (int h = 1; h <= m_sidelen; ++h) {
            int w = box_area / h;
            if (h * w != box_area || w > m_sidelen) continue;
            Position box_sz(h - 1, w - 1);
            auto p2 = pn - box_sz;
            //   - - - - 
            //  |       |
            //         - - - - 
            //  |     |8|     |
            //   - - - -      
            //        |       |
            //         - - - - 
            for (int r = p2.first; r <= pn.first; ++r)
                for (int c = p2.second; c <= pn.second; ++c) {
                    Position tl(r, c), br = tl + box_sz;
                    if (tl.first >= 0 && tl.second >= 0 &&
                        br.first < m_sidelen && br.second < m_sidelen &&
                        // All the other numbers should not be inside this box
                        boost::algorithm::none_of(m_pos2boxinfo, [&](
                        const pair<const Position, puz_box_info>& kv) {
                        auto& p = kv.first;
                        return p != pn &&
                            p.first >= tl.first && p.second >= tl.second &&
                            p.first <= br.first && p.second <= br.second;
                    }))
                        boxes.emplace_back(tl, br);
                }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(int r, int c) const { return m_cells[r * sidelen() + c]; }
    char& cells(int r, int c) { return m_cells[r * sidelen() + c]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
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
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    set<Position> m_horz_walls, m_vert_walls;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    for (auto& [p, info] : g.m_pos2boxinfo) {
        auto& box_ids = m_matches[p];
        box_ids.resize(info.m_boxes.size());
        boost::iota(box_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, box_ids] : m_matches) {
        auto& boxes = m_game->m_pos2boxinfo.at(p).m_boxes;
        boost::remove_erase_if(box_ids, [&](int id) {
            auto& box = boxes[id];
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c)
                    if (cells(r, c) != PUZ_SPACE)
                        return true;
            return false;
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, box_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& box = m_game->m_pos2boxinfo.at(p).m_boxes[n];

    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c)
            cells(r, c) = m_ch;
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);

    ++m_ch, ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
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
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horizontal walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2boxinfo.find(p); it == m_game->m_pos2boxinfo.end())
                out << ".";
            else
                out << it->second.m_area;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BoxItAgain()
{
    using namespace puzzles::BoxItAgain;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BoxItAgain.xml", "Puzzles/BoxItAgain.txt", solution_format::GOAL_STATE_ONLY);
}
