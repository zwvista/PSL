#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 5/Insane Tatamis

    Summary
    Not that long

    Description
    1. Divide the board into rectangular areas, each containing a number.
    2. Every area must be exactly one tile wide.
    3. The length of the other side is NOT equal to the number of this
       region.
    4. A grid dot must not be shared by the corners of four areas.
*/

namespace puzzles::InsaneTatamis{

constexpr auto PUZ_SPACE = ' ';

// top-left and bottom-right
typedef pair<Position, Position> puz_box;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<puz_box> m_boxes;
    map<Position, vector<int>> m_pos2boxids;

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
            for (int i = 1; i <= m_sidelen; ++i)
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
                                if (rng.size() > 1 || it->second == i)
                                    goto next;
                            }
                        }
                    if (rng.size() == 1) {
                        int n = m_boxes.size();
                        m_boxes.emplace_back(tl, br);
                        for (int r2 = tl.first; r2 <= br.first; ++r2)
                            for (int c2 = tl.second; c2 <= br.second; ++c2)
                                m_pos2boxids[{r2, c2}].push_back(n);
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
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
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
: m_game(&g)
, m_cells(g.m_sidelen* g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2boxids)
{
    find_matches(true);
}
    
bool puz_state::check_four_boxes()
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            if (m_horz_walls.contains({r + 1, c}) &&
                m_horz_walls.contains({r + 1, c + 1}) &&
                m_vert_walls.contains({r, c + 1}) &&
                m_vert_walls.contains({r + 1, c + 1}))
                return false;
    return true;
}

int puz_state::find_matches(bool init)
{
    set<Position> spaces;
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& box_ids = kv.second;

        boost::remove_erase_if(box_ids, [&](int id) {
            auto& box = m_game->m_boxes[id];
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
                return make_move2(box_ids[0]), 1;
            }
    }
    return check_four_boxes() ? 2 : 0;
}

void puz_state::make_move2(int n)
{
    auto& box = m_game->m_boxes[n];
    auto &tl = box.first, &br = box.second;
    for (int r = tl.first; r <= br.first; ++r)
        for (int c = tl.second; c <= br.second; ++c) {
            Position p(r, c);
            cells(p) = m_ch, ++m_distance, m_matches.erase(p);
        }
    for (int r = tl.first; r <= br.first; ++r)
        m_vert_walls.emplace(r, tl.second),
        m_vert_walls.emplace(r, br.second + 1);
    for (int c = tl.second; c <= br.second; ++c)
        m_horz_walls.emplace(tl.first, c),
        m_horz_walls.emplace(br.first + 1, c);

    ++m_ch;
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
            out << (m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
                out << '.';
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_InsaneTatamis()
{
    using namespace puzzles::InsaneTatamis;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/InsaneTatamis.xml", "Puzzles/InsaneTatamis.txt", solution_format::GOAL_STATE_ONLY);
}
