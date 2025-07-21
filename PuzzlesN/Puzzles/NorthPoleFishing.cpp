#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 14/North Pole Fishing

    Summary
    Fishing Neighbours

    Description
    1. The board represents a piece of antarctic, which some fisherman want
       to split in equal parts.
    2. They decide each one should have a piece of land of exactly 4 squares,
       including one fishing hole.
    3. Divide the land accordingly.
    4. Ice blocks are just obstacles and don't count as piece of land.
*/

namespace puzzles::NorthPoleFishing{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_BLOCK = 'B';
constexpr auto PUZ_HOLE = 'H';
constexpr auto PUZ_PIECE_SIZE = 4;

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_piece
{
    Position m_p_hint;
    // character that represents the piece. 'a', 'b' and so on
    char m_id;
    // all permutations (forms) of the piece
    set<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2hole;
    string m_cells;
    vector<puz_piece> m_pieces;
    map<Position, vector<int>> m_pos2piece_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, int num, const Position& p)
        : m_game(game), m_num(num) {make_move(p);}

    bool is_goal_state() const { return size() == m_num; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& p : *this)
        for (auto& os : offset) {
            // Pieces extend horizontally or vertically
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            // An adjacent tile can be occupied by the piece
            // if it is a space tile and has not been occupied by the piece
            if (ch2 == PUZ_SPACE && !contains(p2)) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 2)
{
    char name = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_HOLE) {
                m_cells.push_back(PUZ_SPACE);
                m_pos2hole[{r, c}] = name++;
            } else
                m_cells.push_back(ch);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& [p, name] : m_pos2hole) {
        puz_state2 sstart(this, PUZ_PIECE_SIZE, p);
        list<list<puz_state2>> spaths;
        // Pieces can have any form.
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            // save all goal states as permutations
            // A goal state is a piece formed from the hole and has 4 tiles
            for (auto& spath : spaths) {
                int n = m_pieces.size();
                set<Position> rng = spath.back();
                for (auto& p2 : rng)
                    m_pos2piece_ids[p2].push_back(n);
                m_pieces.emplace_back(p, name, rng);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i);
    void make_move2(int i);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the board
    // value.elem: index of the pieces
    map<Position, vector<int>> m_matches;
    set<Position> m_used_hints;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g)
    , m_matches(g.m_pos2piece_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, piece_ids] : m_matches) {
        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& [p, _2, rng] = m_game->m_pieces[id];
            return m_used_hints.contains(p) ||
                !boost::algorithm::all_of(rng, [&](const Position& p2) {
                    return cells(p2) == PUZ_SPACE || p2 == p;
                });
        });

        if (!init)
            switch(piece_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(piece_ids.front()), 1;
            }
    }

    return 2;
}

void puz_state::make_move2(int i)
{
    auto& [p, name, rng] = m_game->m_pieces[i];
    for (auto& p2 : rng)
        if (cells(p2) = name; m_matches.erase(p2))
            ++m_distance;
    m_used_hints.insert(p);
}

bool puz_state::make_move(int i)
{
    m_distance = 0;
    make_move2(i);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [_1, piece_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& n : piece_ids)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return cells(p1) != cells(p2);
    };
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            //char ch = cells(p);
            if (auto it = m_game->m_pos2hole.find(p); it != m_game->m_pos2hole.end())
                out << PUZ_HOLE;
            else
                out << m_game->cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_NorthPoleFishing()
{
    using namespace puzzles::NorthPoleFishing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NorthPoleFishing.xml", "Puzzles/NorthPoleFishing.txt", solution_format::GOAL_STATE_ONLY);
}
