#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 3/Guesstris

    Summary
    Encoded Tetris

    Description
    1. Divide the board in Tetrominoes (Tetris-like shapes of four cells).
    2. Each Tetromino contains two different symbols.
    3. Tetrominoes of the same shape have the same couple of symbols inside
       them, although not necessarily in the same positions.
    4. Tetrominoes with the same symbols can be rotated or mirrored.
*/

namespace puzzles::Guesstris{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SQUARE = 'S';
constexpr auto PUZ_TRIANGLE = 'T';
constexpr auto PUZ_CIRCLE = 'C';
constexpr auto PUZ_DIAMOND = 'D';

constexpr array<Position, 4> offset = Position::Directions4;

// 5. Tetrominoes with the same symbols can be rotated or mirrored.
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

struct puz_piece
{
    int m_index;
    vector<Position> m_rng;
    set<char> m_symbols;
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    vector<puz_piece> m_pieces;
    map<Position, vector<int>> m_pos2piece_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            for (int i = 0; i < tetrominoes.size(); ++i)
                for (auto& t : tetrominoes[i]) {
                    vector<Position> rng;
                    vector<char> symbols;
                    for (auto& p2 : t) {
                        auto p3 = p + p2;
                        if (!is_valid(p3))
                            goto next;
                        rng.push_back(p3);
                        if (char ch = cells(p3); ch != PUZ_SPACE)
                            symbols.push_back(ch);
                    }
                    // 2. Each Tetromino contains two different symbols.
                    // 3. Tetrominoes of the same shape have the same couple of symbols inside
                    // them, although not necessarily in the same positions.
                    if (symbols.size() == 2) {
                        set<char> symbols2(symbols.begin(), symbols.end());
                        if (symbols2.size() == 2) {
                            int n = m_pieces.size();
                            boost::sort(symbols);
                            m_pieces.emplace_back(i, rng, symbols2);
                            for (auto& p2 : rng)
                                m_pos2piece_ids[p2].push_back(n);
                        }
                    }
                next:;
                }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
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
    map<Position, vector<int>> m_matches;
    // value.first: index of the tetrominoes
    // value.second: the symbols
    map<int, set<char>> m_index2symbols;
    unsigned int m_distance = 0;
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.rows() * g.cols(), PUZ_SPACE), m_game(&g)
, m_matches(g.m_pos2piece_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, piece_ids] : m_matches) {
        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& [index, rng, symbols] = m_game->m_pieces[id];
            if(boost::algorithm::any_of(rng, [&](const Position& p) {
                return cells(p) != PUZ_SPACE;
            }))
                return true;
            // 3. Tetrominoes of the same shape have the same couple of symbols inside them
            auto it = m_index2symbols.find(index);
            return it != m_index2symbols.end() && it->second != symbols;
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
    auto& [index, rng, symbols] = m_game->m_pieces[i];
    for (auto& p : rng) {
        cells(p) = m_ch;
        ++m_distance;
        m_matches.erase(p);
    }
    ++m_ch;
    m_index2symbols[index] = symbols;
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
    for (int n : piece_ids)
        if (children.push_back(*this); !children.back().make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == rows()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == cols()) break;
            //out << cells(p);
            out << m_game->cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Guesstris()
{
    using namespace puzzles::Guesstris;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Guesstris.xml", "Puzzles/Guesstris.txt", solution_format::GOAL_STATE_ONLY);
}
