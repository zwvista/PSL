#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 1/FlowerOMino

    Summary
    Don't tread On flowers, Often.

    Description
    1. You are a gardener. you've been employed by a sour weird lady.
    2. This lady, after years of having her garden grow wild with flowers
       and without enclosures, decided that those are way too many flowers
       and too few enclosures.
    3. So now being an avid fan of Tetris, she asked you to divide the garden
       in many Tetris shaped mini-gardens.
    4. And while doing that they HAVE to tread, destroy and plow as many
       flowers as you can, provide you leave just the one in each Tetris
       mini-garden.
    5. Divide the board in tetrominos (4-tile pieces). Each tetromino should
       have only one flower inside it.
*/

namespace puzzles::FlowerOMino{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_HEDGE = '=';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

const vector<vector<Position>> tetrominoes = {
    // L
    {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
    {{0, 1}, {1, 1}, {2, 0}, {2, 1}},
    {{0, 0}, {0, 1}, {0, 2}, {1, 0}},
    {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
    {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
    {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
    {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
    {{0, 2}, {1, 0}, {1, 1}, {1, 2}},
    // I
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
    {{0, 0}, {0, 1}, {0, 2}, {0, 3}},
    // T
    {{0, 0}, {0, 1}, {0, 2}, {1, 1}},
    {{0, 1}, {1, 0}, {1, 1}, {2, 1}},
    {{0, 1}, {1, 0}, {1, 1}, {1, 2}},
    {{0, 0}, {1, 0}, {1, 1}, {2, 0}},
    // S
    {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
    {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
    {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
    // O
    {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // key: the position of flower
    // value: 
    //   one position: The flower is at the center of the tile.
    //   two positions: The flower is on a edge shared by that two tiles.
    set<vector<Position>> m_flowers;
    vector<vector<Position>> m_pieces;
    map<Position, vector<int>> m_pos2piece_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_start(m_sidelen * m_sidelen, PUZ_SPACE)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (char ch = str[c]; ch == PUZ_HEDGE)
                cells(p) = ch;
            else if (ch != PUZ_SPACE) {
                int n = ch - '0';
                if (n & 1) m_flowers.insert({p});
                if (n & 2) m_flowers.insert({p, p + offset[1]});
                if (n & 4) m_flowers.insert({p, p + offset[2]});
            }
        }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < tetrominoes.size(); ++i) {
                auto& t = tetrominoes[i];
                vector<Position> rng; 
                if ([&] {
                    for (auto& p2 : t) {
                        auto p3 = p + p2;
                        if (is_valid(p3) && cells(p3) == PUZ_SPACE)
                            rng.push_back(p3);
                        else
                            return false;
                    }
                    return true;
                }() && boost::count_if(m_flowers, [&](const vector<Position>& v) {
                    return boost::algorithm::all_of(v, [&](const Position& p) {
                        return boost::algorithm::any_of_equal(rng, p);
                    });
                }) == 1) {
                    int n = m_pieces.size();
                    m_pieces.push_back(rng);
                    for (auto& p2 : rng)
                        m_pos2piece_ids[p2].push_back(n);
                }
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
    char m_ch = 'a';
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_matches(g.m_pos2piece_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, piece_ids] : m_matches) {
        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& rng = m_game->m_pieces[id];
            return boost::algorithm::any_of(rng, [&](const Position& p) {
                return cells(p) != PUZ_SPACE;
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
    auto& rng = m_game->m_pieces[i];
    for (auto& p2 : rng)
        cells(p2) = m_ch;
    for (auto it = m_matches.begin(); it != m_matches.end();)
        if (boost::algorithm::any_of_equal(it->second, i))
            ++m_distance, it = m_matches.erase(it);
        else
            ++it;
    ++m_ch;
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
        for (int c = 0; ; ++c) {
            Position p(r, c);
            out << ' ';
            if (c == sidelen()) break;
            bool has_horz_wall = f(p, p + offset[0]);
            out << (has_horz_wall ? '-' : ' ');
            out << (m_game->m_flowers.contains({p + offset[0], p}) ? '*' :
                has_horz_wall ? '-' : ' ');
            out << (has_horz_wall ? '-' : ' ');
        }
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            out << "   ";
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_flowers.contains({p + offset[3], p}) ? '*' :
                f(p, p + offset[3]) ? '|' : ' ');
            if (c == sidelen()) break;
            out << ' ' << cells(p) << (m_game->m_flowers.contains({p}) ? '*' : ' ');
        }
        println(out);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            out << "   ";
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FlowerOMino()
{
    using namespace puzzles::FlowerOMino;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FlowerOMino.xml", "Puzzles/FlowerOMino.txt", solution_format::GOAL_STATE_ONLY);
}
