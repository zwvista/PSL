#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 4/Assembly Instructions

    Summary
    New screw legs 'A' to seat 'C' using bolts 'J'...

    Description
    1. Divide the board so that every letter corresponds to a 'part' which
       has the same shape and orientation everywhere it is found.
    2. So for example if letter 'A' is a 2x3 rectangle, every 'A' on the board
       will correspond to a 2x3 rectangle and 'A' will appear in the same position
       in the rectangle itself.
    3. If letter 'B' has an L shape with the letter on the top left, every 'B'
       will have an L shape with the letter on the top left, etc.
*/

namespace puzzles::AssemblyInstructions{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

using puz_rng2D = vector<set<Position>>;

struct puz_move
{
    char letter;
    vector<Position> m_rng_hints;
    vector<char> m_names;
    puz_rng2D m_rng2D;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2letter;
    map<char, vector<Position>> m_letter2rng;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char name = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_pos2letter[p] = ch;
                m_letter2rng[ch].push_back(p);
                m_cells.push_back(name++);
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_pos2letters) < tie(x.m_cells, x.m_pos2letters);
    }
    bool make_move(const puz_move& move);
    void gen_maps();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_pos2letters.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, set<char>> m_pos2letters;
    map<char, int> m_letter2size;
    set<char> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
{
    gen_maps();
}

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_state* s, const vector<Position>& rng)
        : vector<Position>(rng.size()), m_state(s) { make_move(rng); }
    void make_move(const vector<Position>& rng) {
        for (int i = 0; i < rng.size(); ++i)
            (*this)[i] = rng[i];
    }

    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    for (auto& os : offset) {
        vector<Position> rng;
        for (int i = 0; i < size(); ++i)
            rng.push_back(at(i) + os);
        if (boost::algorithm::all_of(rng, [&](const Position& p) {
            return m_state->cells(p) == PUZ_SPACE;
        }))
            children.emplace_back(*this).make_move(rng);
    }
}

void puz_state::gen_maps()
{
    for (auto& [_1, rng] : m_pos2letters)
        rng.clear();
    m_letter2size.clear();
    for (auto& [letter, rng] : m_game->m_letter2rng) {
        if (m_finished.contains(letter)) continue;
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, rng});
        m_letter2size[letter] = smoves.size();
        for (auto& v : smoves)
            for (auto& p : v)
                if (cells(p) == PUZ_SPACE)
                    m_pos2letters[p].insert(letter);
    }
}

bool puz_state::make_move(const puz_move& move)
{
    m_distance = 0;
    auto& [letter, _1, names, rng2D] = move;
    m_finished.insert(letter);
    for (int i = 0; i < names.size(); ++i)
        for (char name = names[i]; auto& p : rng2D[i])
            if (char& ch = cells(p); ch == PUZ_SPACE)
                ch = name, m_distance++, m_pos2letters.erase(p);
    gen_maps();
    return boost::algorithm::none_of(m_pos2letters, [&](const pair<const Position, set<char>>& kv) {
        return kv.second.empty();
    });
}

struct puz_state3 : puz_rng2D
{
    puz_state3(const puz_state* s, const vector<Position>& rng, const vector<Position>& rng2)
        : puz_rng2D(rng.size()), m_state(s), m_rng2(rng2) { make_move(rng); }
    void make_move(const vector<Position>& rng) {
        for (int i = 0; i < rng.size(); ++i)
            (*this)[i].insert(rng[i]);
    }

    bool is_goal_state() const {
        return boost::algorithm::all_of(m_rng2, [&](const Position& p) {
            return boost::algorithm::any_of(*this, [&](const set<Position>& rng) {
                return rng.contains(p);
            });
        });
    }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
    vector<Position> m_rng2;
};

void puz_state3::gen_children(list<puz_state3>& children) const {
    int sz = front().size();
    for (int j = 0; j < sz; ++j)
        for (auto& os : offset) {
            vector<Position> rng;
            for (int i = 0; i < size(); ++i)
                rng.push_back(*next(at(i).begin(), j) + os);
            if (boost::algorithm::all_of(rng, [&](const Position& p) {
                return m_state->cells(p) == PUZ_SPACE &&
                    boost::algorithm::none_of(*this, [&](const set<Position>& rng2) {
                        return rng2.contains(p);
                    });
            }))
                children.emplace_back(*this).make_move(rng);
        }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, letters] = *boost::min_element(m_pos2letters, [&](
        const pair<const Position, set<char>>& kv1,
        const pair<const Position, set<char>>& kv2) {
        auto f = [&](const set<char>& letters) {
            int sz_all = boost::accumulate(letters, 0, [&](int acc, char letter) {
                return acc + m_letter2size.at(letter);
            });
            return pair(letters.size(), sz_all);
        };
        return f(kv1.second) < f(kv2.second);
    });
    vector<puz_move> moves;
    for (auto& letter : letters) {
        auto& rng = m_game->m_letter2rng.at(letter);
        vector<char> names;
        for (auto& p : rng)
            names.push_back(cells(p));
        vector<Position> rng2;
        if (letters.size() > 1)
            rng2 = {p};
        else
            for (auto& [p2, letters2] : m_pos2letters)
                if (letters2.size() == 1 && *letters2.begin() == letter)
                    rng2.push_back(p2);
        auto smoves = puz_move_generator<puz_state3>::gen_moves({this, rng, rng2});
        for (auto& s : smoves)
            if (s.is_goal_state())
                moves.emplace_back(letter, rng, names, s);
    }
    for (auto& move : moves)
        if (!children.emplace_back(*this).make_move(move))
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
            if (auto it = m_game->m_pos2letter.find(p); it == m_game->m_pos2letter.end())
                out << ".";
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_AssemblyInstructions()
{
    using namespace puzzles::AssemblyInstructions;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/AssemblyInstructions.xml", "Puzzles/AssemblyInstructions.txt", solution_format::GOAL_STATE_ONLY);
}
