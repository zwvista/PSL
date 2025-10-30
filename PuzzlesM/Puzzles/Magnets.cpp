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

constexpr char rect_perms[][2] = {
    {PUZ_POSITIVE, PUZ_NEGATIVE},
    {PUZ_NEGATIVE, PUZ_POSITIVE},
    {PUZ_EMPTY, PUZ_EMPTY},
};

struct puz_hint
{
    int m_positive, m_negative;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_hint> m_hints;
    vector<vector<Position>> m_rects;
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

struct puz_counts
{
    // the number of remaining times that the key char can be used in the area
    // the number of times that the key char has been used in the area
    int m_remaining, m_used;
};

struct puz_area
{
    puz_area(int positive, int negative, int total)
        : m_ch2counts{
            {PUZ_POSITIVE, {positive, 0}},
            {PUZ_NEGATIVE, {negative, 0}},
            {PUZ_EMPTY, {
            positive == PUZ_UNKNOWN || negative == PUZ_UNKNOWN ? PUZ_UNKNOWN :
            total - positive - negative, 0}}
        } {}
    void fill_cell(char ch) {
        auto& [remaining, used] = m_ch2counts.at(ch);
        ++used;
        if (remaining != PUZ_UNKNOWN)
            --remaining;
    }
    bool cant_use(char ch) const { return m_ch2counts.at(ch).m_remaining == 0; }
    bool is_ready() const {
        auto f = [&](char ch) {
            int n = m_ch2counts.at(ch).m_remaining;
            return n == 0 || n == PUZ_UNKNOWN;
        };
        return f(PUZ_POSITIVE) && f(PUZ_NEGATIVE);
    }

    // key : all chars used to fill a position
    map<char, puz_counts> m_ch2counts;
};

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);
    bool is_ready() const {
        return boost::algorithm::all_of(m_areas, [](const puz_area& a) {
            return a.is_ready();
        });
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: the index of the rect
    // value.elem: the index of the perm
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
    vector<puz_area> m_areas;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
    for (int i = 0; i < sidelen() * 2; ++i) {
        auto& [pos, neg] = g.m_hints[i];
        m_areas.emplace_back(pos, neg, sidelen());
    }

    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            if (Position p(r, c); g.cells(p) == PUZ_EMPTY) {
                cells(p) = PUZ_EMPTY;
                m_areas[r].fill_cell(PUZ_EMPTY);
                m_areas[c + sidelen()].fill_cell(PUZ_EMPTY);
            }

    for (int i = 0; i < g.m_rects.size(); ++i)
        m_matches[i] = {0, 1, 2};
}

int puz_state::find_matches(bool init)
{
    for (auto& [i, perm_ids] : m_matches) {
        auto& rect = m_game->m_rects[i];
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = rect_perms[id];
            for (int k = 0; k < rect.size(); ++k) {
                auto& p = rect[k];
                char ch = perm[k];
                if (m_areas[p.first].cant_use(ch) ||
                    m_areas[p.second + sidelen()].cant_use(ch))
                    return true;
                if (ch == PUZ_EMPTY)
                    continue;
                for (int d = 0; d < 4; ++d) {
                    auto p2 = p + offset[d];
                    if (is_valid(p2) && cells(p2) == ch)
                        return true;
                }
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(i, perm_ids[0]), 1;
            }
    }
    return !is_goal_state() || is_ready() ? 2 : 0;
}

void puz_state::make_move2(int i, int j)
{
    auto& rect = m_game->m_rects[i];
    auto& perm = rect_perms[j];
    for (int k = 0; k < rect.size(); ++k) {
        auto& p = rect[k];
        char ch = perm[k];
        cells(p) = ch;
        m_areas[p.first].fill_cell(ch);
        m_areas[p.second + sidelen()].fill_cell(ch);
    }
    ++m_distance, m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [i, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int j : perm_ids)
        if (!children.emplace_back(*this).make_move(i, j))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen() + 2; ++r) {
        for (int c = 0; c < sidelen() + 2; ++c) {
            Position p(r, c);
            if (r < sidelen() && c < sidelen())
                out << cells(p);
            else if (r >= sidelen() && c >= sidelen())
                out << m_game->cells(p);
            else {
                bool is_row = c >= sidelen();
                out <<
                    m_areas[is_row ? r : c + sidelen()].m_ch2counts.at
                    ((is_row ? c : r) == sidelen() ?
                    PUZ_POSITIVE : PUZ_NEGATIVE).m_used;
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
