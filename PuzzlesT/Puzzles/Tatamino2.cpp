#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Tatamino

    Summary
    Which is a little Tatami

    Description
    1. Plays like Fillomino, in which you have to guess areas on the board
       marked by their number.
    2. In Tatamino, however, you only have areas 1, 2 and 3 tiles big.
    3. Please remember two areas of the same number size can't be touching.
*/

namespace puzzles::Tatamino2{

constexpr auto PUZ_SPACE = ' ';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<pair<Position, char>> m_pos2num;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            if (ch != PUZ_SPACE)
                m_pos2num.emplace_back(p, ch);
        }
    }
}

struct puz_area
{
    set<Position> m_inner, m_outer;
    int m_cell_count;
};

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
    bool make_move(const Position& p, char ch);
    int get_id() const {
        for (int i = 0;; ++i)
            if (!m_id2area.contains(i))
                return i;
    }
    // remove the possibility of filling the position p with char ch
    void remove_pair(const Position& p, char ch) {
        auto i = m_pos2nums.find(p);
        if (i != m_pos2nums.end())
            boost::remove_erase(i->second, ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<Position>> m_id2area;
    map<Position, int> m_pos2id;
    map<Position, string> m_pos2nums;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            m_pos2nums[{r, c}] = "123";

    for (auto& [p, ch] : m_game->m_pos2num)
        make_move(p, ch);
}

bool puz_state::make_move(const Position& p, char ch)
{
    int n = ch - '0';
    cells(p) = ch;
    m_pos2nums.erase(p);

    set<int> ids;
    for (auto& os : offset) {
        auto p2 = p + os;
        if (is_valid(p2) && cells(p2) == ch)
            ids.insert(m_pos2id.at(p2));
    }

    vector<Position> rng{p};
    for (int id : ids) {
        auto& v = m_id2area.at(id);
        rng.insert(rng.end(), v.begin(), v.end());
        m_id2area.erase(id);
    }

    int sz = rng.size();
    if (sz > n)
        return false;
    if (sz == n)
        for (auto& p2 : rng) {
            for (auto& os : offset) {
                auto p3 = p2 + os;
                if (is_valid(p3))
                    remove_pair(p3, ch);
            }
            m_pos2id.erase(p2);
            m_pos2nums.erase(p2);
        } else {
        int id = get_id();
        for (auto& p2 : rng)
            m_pos2id[p2] = id;
        m_id2area[id] = rng;
    }

    return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, string>& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, nums] = *boost::min_element(m_pos2nums, [](
        const pair<const Position, string>& kv1,
        const pair<const Position, string>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (char ch : nums)
        if (children.push_back(*this); !children.back().make_move(p, ch))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    //for(int r = 0;; ++r) {
    //    // draw horizontal lines
    //    for (int c = 0; c < sidelen(); ++c)
    //        out << (m_horz_walls.at({r, c}) == PUZ_WALL_ON ? " -" : "  ");
    //    println(out);
    //    if (r == sidelen()) break;
    //    for (int c = 0;; ++c) {
    //        Position p(r, c);
    //        // draw vertical lines
    //        out << (m_vert_walls.at(p) == PUZ_WALL_ON ? '|' : ' ');
    //        if (c == sidelen()) break;
    //        out << cells(p);
    //    }
    //    println(out);
    //}
    return out;
}

}

void solve_puz_Tatamino2()
{
    using namespace puzzles::Tatamino2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Tatamino.xml", "Puzzles/Tatamino2.txt", solution_format::GOAL_STATE_ONLY);
}
