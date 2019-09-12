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

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_box_info
{
    // the length of the tatami
    char m_ch;
    // top-left and bottom-right
    pair<Position, Position> m_box;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_box_info> m_boxinfos;
    map<Position, vector<int>> m_pos2infoids;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c)
            for (int i = 1; i <= 4; ++i)
                for (int j = 0; j < 2; ++j) {
                    int h = i, w = 1;
                    if (j == 1) ::swap(h, w);
                    Position box_sz(h - 1, w - 1);
                    Position tl(r, c), br = tl + box_sz;
                    if (!(br.first < m_sidelen && br.second < m_sidelen)) continue;
                    vector<Position> rng;
                    for (int r2 = tl.first; r2 <= br.first; ++r2)
                        for (int c2 = tl.second; c2 <= br.second; ++c2) {
                            Position p(r2, c2);
                            if (auto it = m_pos2num.find(p); it != m_pos2num.end()) {
                                rng.push_back(p);
                                // 3. A cell with a number indicates the length of the Tatami.
                                // Not all Tatamis have to be marked by a number.
                                if (rng.size() > 1 || it->second != i)
                                    goto next;
                            }
                        }
                    if (rng.size() <= 1) {
                        int n = m_boxinfos.size();
                        puz_box_info info;
                        info.m_ch = i + '0';
                        info.m_box = {tl, br};
                        m_boxinfos.push_back(info);
                        for (int r2 = tl.first; r2 <= br.first; ++r2)
                            for (int c2 = tl.second; c2 <= br.second; ++c2)
                                m_pos2infoids[{r2, c2}].push_back(n);
                    }
                next:;
                }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    bool check_four_boxes();

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
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the box
    map<Position, vector<int>> m_matches;
    set<Position> m_horz_walls, m_vert_walls;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2infoids)
{
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
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            auto& info = m_game->m_boxinfos[id];
            auto& box = info.m_box;
            char ch = info.m_ch;
            for (int r = box.first.first; r <= box.second.first; ++r)
                for (int c = box.first.second; c <= box.second.second; ++c) {
                    Position p(r, c);
                    if (cells(p) != PUZ_SPACE)
                        return true;
                    // 4. Two Tatamis of the same size must not be orthogonally adjacent.
                    for (auto& os : offset)
                        if (auto p2 = p + os; is_valid(p2) && cells(p2) == ch)
                            return true;
                }
            return false;
        });

        if (!init)
            switch(box_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(box_ids[0]), 1;
            }
    }
    return check_four_boxes() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
    auto& info = m_game->m_boxinfos[n];
    auto& box = info.m_box;
    char ch = info.m_ch;

    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c) {
            Position p(r, c);
            cells(p) = ch, ++m_distance, m_matches.erase(p);
        }
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);
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
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(n))
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
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << '.';
            else
                out << it->second;
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
