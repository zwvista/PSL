#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 4/Mixed Tatamis

    Summary
    Just that long

    Description
    1. Divide the board into areas (Tatami).
    2. Every Tatami must be exactly one cell wide and can be of length
       from 1 to 4 cells.
    3. A cell with a number indicates the length of the Tatami. Not all
       Tatamis have to be marked by a number.
    4. Two Tatamis of the same size must not be orthogonally adjacent.
    5. A grid dot must not be shared by the corners of four Tatamis
       (lines can't form 4-way crosses).
*/

namespace puzzles::MixedTatamis{

#define PUZ_SPACE        ' '

struct puz_box_info
{
    // the number of the box
    int m_num;
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
            if (ch != ' ')
                m_pos2boxinfo[{r, c}].m_num = ch - '0';
        }
    }

    for (auto& kv : m_pos2boxinfo) {
        // the position of the number
        const auto& pn = kv.first;
        auto& info = kv.second;
        int n = info.m_num;
        auto& boxes = info.m_boxes;

        for (int i = 1; i <= m_sidelen; ++i)
            for (int j = 0; j < 2; ++j) {
                int h = i, w = 1;
                if (h == n) continue;
                if (j == 1) ::swap(h, w);
                Position box_sz(h - 1, w - 1);
                auto p2 = pn - box_sz;
                //   - - - -
                //  |       |
                //         - - - -
                //  |     |4|     |
                //   - - - -
                //        |       |
                //         - - - -
                for (int r = p2.first; r <= pn.first; ++r)
                    for (int c = p2.second; c <= pn.second; ++c) {
                        Position tl(r, c), br = tl + box_sz;
                        if (tl.first >= 0 && tl.second >= 0 &&
                            br.first < m_sidelen && br.second < m_sidelen &&
                            // All the other symbols should not be inside this box
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
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(int r, int c) const { return m_cells[r * sidelen() + c]; }
    char& cells(int r, int c) { return m_cells[r * sidelen() + c]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool check_four_boxes();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

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
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& kv : g.m_pos2boxinfo) {
        auto& box_ids = m_matches[kv.first];
        box_ids.resize(kv.second.m_boxes.size());
        boost::iota(box_ids, 0);
    }

    find_matches(true);
}
    
bool puz_state::check_four_boxes()
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            if (m_horz_walls.count({r + 1, c}) == 1 &&
                m_horz_walls.count({r + 1, c + 1}) == 1 &&
                m_vert_walls.count({r, c + 1}) == 1 &&
                m_vert_walls.count({r + 1, c + 1}) == 1)
                return false;
    return true;
}

int puz_state::find_matches(bool init)
{
    set<Position> spaces;
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

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
    return check_four_boxes() && boost::count(m_cells, PUZ_SPACE) == spaces.size() ? 2 : 0;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& box = m_game->m_pos2boxinfo.at(p).m_boxes[n];

    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c)
            cells(r, c) = m_ch, ++m_distance;
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);

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
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_horz_walls.count({r, c}) == 1 ? " -" : "  ");
        out << endl;
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.count(p) == 1 ? '|' : ' ');
            if (c == sidelen()) break;
            auto it = m_game->m_pos2boxinfo.find(p);
            if (it == m_game->m_pos2boxinfo.end())
                out << '.';
            else
                out << it->second.m_num;
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_MixedTatamis()
{
    using namespace puzzles::MixedTatamis;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MixedTatamis.xml", "Puzzles/MixedTatamis.txt", solution_format::GOAL_STATE_ONLY);
}
