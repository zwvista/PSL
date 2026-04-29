#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 2/Magnets

    Summary
    Place Magnets on the board, respecting the orientation of poles

    Description
    1. Each Magnet has a positive(+) and a negative(-) pole.
    2. Every rectangle can either contain a Magnet or be empty.
    3. The number on the board tells you how many positive and negative poles
       you can see from there in a straight line.
    4. When placing a Magnet, you have to respect the rule that the same pole
       (+ and + / - and -) can't be adjacent horizontally or vertically.
    5. In some levels, a few numbers on the border can be hidden.
*/

namespace puzzles::Magnets{

constexpr auto PUZ_HORZ = 'H';
constexpr auto PUZ_VERT = 'V';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_POSITIVE = '+';
constexpr auto PUZ_NEGATIVE = '-';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_UNKNOWN = 9999;

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_hint
{
    int m_positive, m_negative;
};

using puz_cell = vector<char>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    // index: row or column of the hints
    vector<puz_hint> m_hints;
    // all rectangles
    vector<vector<Position>> m_rects;
    // key: a position in the rectangle
    // value: id of the rectangle
    map<Position, int> m_pos2rect;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * (m_sidelen + 2) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 2)
    , m_hints(m_sidelen * 2)
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen + 2; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen + 2; ++c)
            switch(Position p(r, c); char ch = str[c]) {
            case PUZ_POSITIVE:
            case PUZ_NEGATIVE:
            case PUZ_EMPTY:
                break;
            case PUZ_HORZ:
            case PUZ_VERT:
                {
                    int n = m_rects.size();
                    auto& rect = m_rects.emplace_back();
                    for (int d = 0; d < 2; ++d) {
                        auto p = ch == PUZ_HORZ ? Position(r, c + d) : Position(r + d, c);
                        rect.push_back(p);
                        m_pos2rect[p] = n;
                    }
                }
                break;
            default:
                if ((c >= m_sidelen) != (r >= m_sidelen)) {
                    bool is_row = c >= m_sidelen;
                    auto& hint = m_hints[is_row ? r : c + m_sidelen];
                    ((is_row ? c : r) == m_sidelen ? hint.m_positive : hint.m_negative) =
                        ch == PUZ_SPACE ? PUZ_UNKNOWN : ch - '0';
                }
                break;
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    const puz_cell& cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    puz_cell& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int i, char ch);
    int check_magnets();
    bool check_hints();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return sidelen() * sidelen() - m_finished.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_cell> m_cells;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen)
, m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (g.cells(p) == PUZ_EMPTY)
                cells(p) = {PUZ_EMPTY}, m_finished.insert(p);
            else
                cells(p) = {PUZ_POSITIVE, PUZ_NEGATIVE, PUZ_EMPTY};
        }
    check_hints();
    check_magnets();
}

int puz_state::check_magnets()
{
    int n = 2;
    for (;;) {
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c) {
                Position p(r, c);
                if (m_finished.contains(p)) continue;
                auto& cl = cells(p);
                if (cl.size() != 1) continue;
                newly_finished.insert(p);
            }
        
        if (newly_finished.empty())
            return n;

        n = 1;
        for (auto& p : newly_finished) {
            char ch = cells(p)[0];
            auto& rect = m_game->m_rects[m_game->m_pos2rect.at(p)];
            for (auto& p2 : rect)
                if (p2 != p)
                    cells(p2) = {ch == PUZ_POSITIVE ? PUZ_NEGATIVE : ch == PUZ_NEGATIVE ? PUZ_POSITIVE : PUZ_EMPTY};
            if (ch != PUZ_EMPTY)
                for (auto& os : offset)
                    if (auto p2 = p + os; is_valid(p2)) {
                        auto& cl = cells(p2);
                        boost::remove_erase(cl, ch);
                        if (cl.empty())
                            return 0;
                    }
        }
        m_finished.insert_range(newly_finished);
        m_distance += newly_finished.size();
    }
}

bool puz_state::check_hints()
{
    for (int i = 0; i < sidelen() * 2; ++i) {
        auto& hint = m_game->m_hints[i];
        bool is_row = i < sidelen();
        for (int j = 0; j < 2; ++j) {
            bool is_positive = j == 0;
            int num = is_positive ? hint.m_positive : hint.m_negative;
            if (num == PUZ_UNKNOWN) continue;
            char ch = is_positive ? PUZ_POSITIVE : PUZ_NEGATIVE;
            int max_possible = 0, min_guaranteed = 0;
            for (int k = 0; k < sidelen(); ++k) {
                auto p = is_row ? Position(i, k) : Position(k, i - sidelen());
                auto& cl = cells(p);
                if (boost::algorithm::any_of_equal(cl, ch)) max_possible++;
                if (cl.size() == 1 && cl[0] == ch) min_guaranteed++;
            }

            // Prune if we can't possibly reach the target
            if (max_possible < num) return false;
            // Prune if we have already exceeded the target
            if (min_guaranteed > num) return false;
            // Final check
            if (is_goal_state() && min_guaranteed != num) return false;
            if (min_guaranteed == num)
                for (int k = 0; k < sidelen(); ++k) {
                    auto p = is_row ? Position(i, k) : Position(k, i - sidelen());
                    if (auto& cl = cells(p); cl.size() > 1)
                        boost::remove_erase(cl, ch);
                }
        }
    }
    return true;
}

bool puz_state::make_move(int i, char ch)
{
    m_distance = 1;
    m_cells[i] = {ch};
    for (;;) {
        int m = check_magnets();
        if (m == 2) return true;
        if (m == 0) return false;
        if (!check_hints()) return false;
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int i = boost::min_element(m_cells, [](const puz_cell& cl1, const puz_cell& cl2) {
        auto f = [](const puz_cell& cl) {
            int sz = cl.size();
            return sz == 1 ? 100 : sz;
        };
        return f(cl1) < f(cl2);
    }) - m_cells.begin();
    auto& cl = m_cells[i];
    for (char ch : cl)
        if (!children.emplace_back(*this).make_move(i, ch))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen() + 2; ++r) {
        for (int c = 0; c < sidelen() + 2; ++c) {
            Position p(r, c);
            if (r < sidelen() && c < sidelen())
                out << cells(p)[0];
            else if (r >= sidelen() && c >= sidelen())
                out << m_game->cells(p);
            else {
                bool is_row = c >= sidelen();
                bool is_positive = (is_row ? c : r) - sidelen() == 0;
                int n = 0;
                for (int i = 0; i < sidelen(); ++i) {
                    auto p2 = is_row ? Position(r, i) : Position(i, c);
                    char ch = cells(p2)[0];
                    if (is_positive && ch == PUZ_POSITIVE || !is_positive && ch == PUZ_NEGATIVE)
                        ++n;
                }
                out << n;
            }
            out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Magnets()
{
    using namespace puzzles::Magnets;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/Magnets.xml", "Puzzles/Magnets.txt", solution_format::GOAL_STATE_ONLY);
}
