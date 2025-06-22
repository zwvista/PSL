#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 5/TetrominoPegs

    Summary
    Stuck in Tetris

    Description
    1. Divide the board into Tetrominoes, area of exactly four tiles, of a shape
       like the pieces of Tetris, that is: L, I, T, S or O.
    2. Wood cells are fixed pegs and aren't part of Tetrominoes.
    3. Tetrominoes may be rotated or mirrored.
    4. Two Tetrominoes sharing an edge must be different.
*/

namespace puzzles::TetrominoPegs{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_PEG = '.';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

// 3. Tetrominoes may be rotated or mirrored.
const vector<vector<vector<Position>>> tetrominoes = {
    { // L
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
        {{0, 1}, {1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, 1}, {0, 2}, {1, 0}},
        {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 2}, {1, 0}, {1, 1}, {1, 2}},
    },
    { // I
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
        {{0, 0}, {0, 1}, {0, 2}, {0, 3}},
    },
    { // T
        {{0, 0}, {0, 1}, {0, 2}, {1, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {2, 0}},
    },
    { // S
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
        {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
    },
    { // O
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
    },
};

using puz_piece = pair<int, vector<Position>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<puz_piece> m_pieces;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < tetrominoes.size(); ++i)
                for (auto& t : tetrominoes[i]) {
                    vector<Position> rng;
                    for (auto& p2 : t) {
                        auto p3 = p + p2;
                        if (is_valid(p3) && cells(p3) == PUZ_SPACE)
                            rng.push_back(p3);
                        else
                            goto next;
                    }
                    m_pieces.emplace_back(i, rng);
                next:;
                }
        }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
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

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    for (int i = 0; i < g.m_pieces.size(); ++i)
        for (auto& p : g.m_pieces[i].second)
            m_matches[p].push_back(i);
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, piece_ids] : m_matches) {
        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& piece = m_game->m_pieces[id];
            char ch = piece.first + '0';
            for (auto& p : piece.second) {
                if (cells(p) != PUZ_SPACE)
                    return true;
                // 4. Two Tetrominoes sharing an edge must be different.
                for (auto& os : offset) {
                    auto p2 = p + os;
                    if (is_valid(p2) && ch == cells(p2))
                        return true;
                }
            }
            return false;
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
    auto& piece = m_game->m_pieces[i];
    for (auto p : piece.second) {
        cells(p) = piece.first + '0';
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
    auto& [p, piece_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : piece_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TetrominoPegs()
{
    using namespace puzzles::TetrominoPegs;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TetrominoPegs.xml", "Puzzles/TetrominoPegs.txt", solution_format::GOAL_STATE_ONLY);
}
