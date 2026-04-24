#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Finger Pointing

    Summary
    Blame is in the air

    Description
    1. Fill the board with fingers. Two fingers pointing in the same direction
       can't be orthogonally adjacent.
    2. the number tells you how many fingers and finger 'trails' point to that tile.
*/

namespace puzzles::FingerPointing{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BLOCK = 'O';

constexpr array<Position, 4> offset = Position::Directions4;
constexpr string_view dirs = "^>v<";

using puz_move = map<Position, char>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : map<Position, int>
{
    puz_state2(const puz_game* game, const Position& p, int n)
        : m_game(game), m_num(n) { make_move(p, -1); }

    bool is_goal_state() const { return size() == m_num + 1; }
    void make_move(Position p, int i) { emplace(p, i); }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (is_goal_state())
        return;
    for (auto& [p, last_dir] : *this)
        for (int i = 0; i < 4; ++i) {
            // 1. Two fingers pointing in the same direction can't be orthogonally adjacent.
            if (last_dir == (i + 2) % 4 || last_dir == i) continue;
            auto p2 = p + offset[i];
            if (contains(p2)) continue;
            char ch = m_game->cells(p2);
            if (ch != PUZ_SPACE && ch != dirs[(i + 2) % 4]) continue;
            // 1. Two fingers pointing in the same direction can't be orthogonally adjacent.
            if (boost::algorithm::none_of(offset, [&](const Position& os) {
                auto it = find(p2 + os);
                return it != end() && it->second == i;
            }))
                children.emplace_back(*this).make_move(p2, i);
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BLOCK);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch != PUZ_BLOCK && ch != PUZ_SPACE && dirs.find(ch) == -1)
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            m_cells.push_back(ch);
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

    for (auto& [p, sum] : m_pos2num) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, sum});
        for (auto& s : smoves)
            if (s.is_goal_state()) {
                puz_move move;
                move[p] = PUZ_SPACE;
                for (auto& [p2, i] : s)
                    if (p2 != p)
                        move[p2] = dirs[(i + 2) % 4];
                int n = m_moves.size();
                m_moves.push_back(move);
                m_pos2move_id[p].push_back(n);
                for (auto& [p2, ch] : move)
                    m_pos2move_id[p2].push_back(n);
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
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the position of the board
    // value: the index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
, m_matches(g.m_pos2move_id)
{
    find_matches(false);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& move = m_game->m_moves[id];
            return !boost::algorithm::all_of(move, [&](const pair<const Position, char>& kv) {
                auto& [p2, ch2] = kv;
                return ch2 == PUZ_SPACE || cells(p2) == PUZ_SPACE &&
                    boost::algorithm::none_of(offset, [&](const Position& os) {
                        return cells(p2 + os) == ch2;
                    });
            });
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i)
{
    auto& move = m_game->m_moves[i];
    for (auto& [p, ch] : move) {
        if (ch != PUZ_SPACE)
            cells(p) = ch;
        ++m_distance;
        m_matches.erase(p);
    }
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
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            if (auto it = m_game->m_pos2num.find({r, c}); it != m_game->m_pos2num.end())
                out << format("{:2} ", it->second);
            else if (char ch = cells({r, c}); ch != PUZ_SPACE)
                out << format(" {} ", ch);
            else
                out << " . ";
        println(out);
    }
    return out;
}

}

void solve_puz_FingerPointing()
{
    using namespace puzzles::FingerPointing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FingerPointing.xml", "Puzzles/FingerPointing.txt", solution_format::GOAL_STATE_ONLY);
}
