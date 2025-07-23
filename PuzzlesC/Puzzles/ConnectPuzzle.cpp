#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Number Connect

    Summary
    Connect the numbers 

    Description
*/

namespace puzzles::ConnectPuzzle{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_line
{
    char m_letter;
    Position m_start, m_end;
    int m_count = 0;
};

struct puz_move
{
    char m_letter;
    vector<Position> m_path;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, puz_line> m_letter2line;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game* game, const puz_line* line)
        : m_game(game), m_line(line) {make_move(line->m_start);}
    bool make_move(const Position& p);

    bool is_goal_state() const { return get_heuristic() == 0; }
    unsigned int get_heuristic() const { return manhattan_distance(back(), m_line->m_end); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return m_distance; }

    const puz_game* m_game;
    const puz_line* m_line;
    unsigned int m_distance = 0;
};

struct puz_state3 : Position
{
    puz_state3(const puz_state2* s, const puz_line* line) : m_state(s), m_line(line) {
        make_move(s->m_line == line ? s->back() : line->m_start);
    }
    const puz_game& game() const { return *m_state->m_game; }
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }

    bool is_goal_state() const { return get_heuristic() == 0; }
    unsigned int get_heuristic() const { return manhattan_distance(*this, m_line->m_end); }
    void gen_children(list<puz_state3>& children) const;
    unsigned int get_distance(const puz_state3& child) const { return 1; }

    const puz_state2* m_state;
    const puz_line* m_line;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os;
            game().is_valid(p2) && boost::algorithm::none_of_equal(*m_state, p2))
            if (char ch = game().cells(p2); ch == PUZ_SPACE || p2 == m_line->m_end) {
                children.push_back(*this);
                children.back().make_move(p2);
            }
}

bool puz_state2::make_move(const Position& p)
{
    int d1 = empty() ? 0 : get_heuristic();
    push_back(p);
    for (auto& [letter, line] : m_game->m_letter2line) {
        puz_state3 sstart(this, &line);
        list<list<puz_state3>> spaths;
        if (auto [found, _1] = puz_solver_astar<puz_state3>::find_solution(sstart, spaths); !found)
            return false;
    }
    m_distance = size() == 1 ? 1 : d1 - get_heuristic();
    return true;
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& p = back(); auto& os : offset)
        if (auto p2 = p + os; m_game->is_valid(p2))
            if (char ch = m_game->cells(p2);
                p2 == m_line->m_end ||
                ch == PUZ_SPACE && boost::algorithm::none_of_equal(*this, p2))
                if (children.push_back(*this); !children.back().make_move(p2))
                    children.pop_back();
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            if (ch != PUZ_SPACE) {
                auto& [letter, start, end, count] = m_letter2line[ch];
                letter = ch;
                (count++ == 0 ? start : end) = p;
            }
            m_cells.push_back(ch);
        }
    }
    for (auto& [letter, line] : m_letter2line) {
        puz_state2 sstart(this, &line);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_astar<puz_state2, true, false>::find_solution(sstart, spaths); found)
            for (auto& spath : spaths) {
                auto& s = spath.back();
                int n = m_moves.size();
                vector v(s.begin() + 1, s.end() - 1);
                m_moves.emplace_back(letter, v);
                for (auto& p2 : v)
                    m_pos2move_ids[p2].push_back(n);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
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
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_2, path] = m_game->m_moves[id];
            return !boost::algorithm::all_of(path, [&](const Position& p2) {
                return cells(p2) == PUZ_SPACE;
            });
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
    auto& [letter, path] = m_game->m_moves[n];
    for (auto& p : path)
        cells(p) = letter, ++m_distance, m_matches.erase(p);
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
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& move_id : move_ids)
        if (children.push_back(*this); !children.back().make_move(move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << format("{} ", cells({r, c}));
        println(out);
    }
    return out;
}

}

void solve_puz_ConnectPuzzle()
{
    using namespace puzzles::ConnectPuzzle;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ConnectPuzzle.xml", "Puzzles/ConnectPuzzle.txt", solution_format::GOAL_STATE_ONLY);
}
