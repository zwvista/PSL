#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Neighbours

    Summary
    Neighbours, yes, but not equally sociable

    Description
    1. The board represents a piece of land bought by a bunch of people. They
       decided to split the land in equal parts.
    2. However some people are more social and some are less, so each owner
       wants an exact number of neighbours around him.
    3. Each number on the board represents an owner house and the number of
       neighbours he desires.
    4. Divide the land so that each one has an equal number of squares and
       the requested number of neighbours.
    5. Later on, there will be Question Marks, which represents an owner for
       which you don't know the neighbours preference.
*/

namespace puzzles::Neighbours{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_QM = '?';
constexpr auto PUZ_UNKNOWN = 0;

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_garden
{
    char m_name;
    int m_num;
};

struct puz_move
{
    char m_name;
    Position m_p_hint;
    set<Position> m_area;
    set<Position> m_neighbours;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_garden_size;
    map<Position, puz_garden> m_pos2garden;
    map<char, Position> m_name2hint;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) {make_move(p);}

    bool is_goal_state() const { return size() == m_game->m_garden_size; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    if (is_goal_state())
        return;
    for (auto& p : *this)
        for (auto& os : offset) {
            auto p2 = p + os;
            char ch2 = m_game->cells(p2);
            if (ch2 == PUZ_SPACE && !contains(p2))
                children.emplace_back(*this).make_move(p2);
        }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_g = 'a';
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c - 1];
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                char name = ch_g++;
                m_cells.push_back(name);
                int num = ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0';
                m_pos2garden[p] = {name, num};
                m_name2hint[name] = p;
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    m_garden_size = (m_sidelen - 2) * (m_sidelen - 2) / m_pos2garden.size();

    for (auto& [p, garden] : m_pos2garden) {
        auto& [name, num] = garden;
        // Gardens can have any form.
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p});
        // save all goal states as permutations
        // A goal state is a garden formed from the number
        for (auto& s : smoves) {
            if (!s.is_goal_state() || boost::algorithm::any_of(m_moves, [&](const puz_move& move) {
                return move.m_area == s;
            })) continue;
            int n = m_moves.size();
            auto& [name2, p_hint, area, neighbours] = m_moves.emplace_back();
            name2 = name, p_hint = p, area = s;
            // Gardens are separated by a wall.
            for (auto& p2 : s)
                for (auto& os : offset)
                    if (auto p3 = p2 + os; !s.contains(p3))
                        if (char ch3 = cells(p3); ch3 != PUZ_BOUNDARY)
                            neighbours.insert(p3);
            for (auto& p2 : s)
                m_pos2move_ids[p2].push_back(n);
        }
    }
}

struct puz_state
{
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
    bool check_neighbours() const;

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
    map<Position, int> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_2, p_hint, area, _3] = m_game->m_moves[id];
            return !boost::algorithm::all_of(area, [&](const Position& p2) {
                char ch2 = cells(p2);
                return p_hint == p2 || ch2 == PUZ_SPACE;
            });
        });
        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return check_neighbours() ? 2 : 0;
}

bool puz_state::check_neighbours() const
{
    for (auto& [p, n] : m_finished) {
        int num = m_game->m_pos2garden.at(p).m_num;
        if (num == PUZ_UNKNOWN) continue;
        auto& [_1, _2, _3, neighbours] = m_game->m_moves[n];
        set<char> chars;
        int n_spaces = 0;
        for (auto& p2 : neighbours)
            if (char ch = cells(p2); ch == PUZ_SPACE)
                n_spaces++;
            else
                chars.insert(ch);
        int min_guaranteed = chars.size();
        int max_possible = min_guaranteed + n_spaces;

        // Prune if we can't possibly reach the target
        if (max_possible < num) return false;
        // Prune if we have already exceeded the target
        if (min_guaranteed > num) return false;
        // Final check
        if (is_goal_state() && min_guaranteed != num) return false;
    }
    return true;
}

void puz_state::make_move2(int n)
{
    auto& [name, p_hint, area, _3] = m_game->m_moves[n];
    for (auto& p2 : area) {
        cells(p2) = name;
        m_distance++, m_matches.erase(p2);
    }
    m_finished[p_hint] = n;
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
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
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
            char ch = cells(p);
            auto pHouse = m_game->m_name2hint.at(ch);
            if (pHouse != p)
                out << ' ';
            else {
                set<char> chars;
                auto& [_1, _2, _3, neighbours] = m_game->m_moves[m_finished.at(pHouse)];
                for (auto& p2 : neighbours)
                    chars.insert(cells(p2));
                out << chars.size();
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Neighbours()
{
    using namespace puzzles::Neighbours;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Neighbours.xml", "Puzzles/Neighbours.txt", solution_format::GOAL_STATE_ONLY);
}
