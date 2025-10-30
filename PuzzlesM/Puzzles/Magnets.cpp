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

using puz_chars = vector<char>;
const puz_chars all_chars = {PUZ_POSITIVE, PUZ_NEGATIVE, PUZ_EMPTY};

struct puz_hint
{
    int m_positive, m_negative;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_rect_count;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2rect;
    vector<puz_hint> m_hints;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * (m_sidelen + 2) + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 2)
    , m_rect_count(m_sidelen * m_sidelen / 2)
    , m_areas(m_rect_count + m_sidelen * 2)
    , m_hints(m_sidelen * 2)
{
    m_cells = boost::accumulate(strs, string());
    for (int r = 0, n = 0; r < m_sidelen + 2; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen + 2; ++c)
            switch(Position p(r, c); char ch = str[c]) {
            case PUZ_POSITIVE:
            case PUZ_NEGATIVE:
            case PUZ_EMPTY:
                break;
            case PUZ_HORZ:
            case PUZ_VERT:
                for (int d = 0; d < 2; ++d) {
                    auto p = ch == PUZ_HORZ ? Position(r, c + d) : Position(r + d, c);
                    m_pos2rect[p] = n;
                    for (int i : {n, m_rect_count + p.first, m_rect_count + m_sidelen + p.second})
                        m_areas[i].push_back(p);
                }
                ++n;
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
    puz_area(int index, int positive, int negative, int total)
        : m_index(index)
        , m_ch2counts{
            {PUZ_POSITIVE, {positive, 0}},
            {PUZ_NEGATIVE, {negative, 0}},
            {PUZ_EMPTY, {
            positive == PUZ_UNKNOWN || negative == PUZ_UNKNOWN ? PUZ_UNKNOWN :
            total - positive - negative, 0}}
        } {}
    bool fill_cell(const Position& p, char ch) {
        auto& [remaining, used] = m_ch2counts.at(ch);
        ++used;
        return remaining == PUZ_UNKNOWN || --remaining >= 0;
    }
    bool cant_use(char ch) const { return m_ch2counts.at(ch).m_remaining == 0; }
    bool is_ready() const {
        auto f = [&](char ch) {
            int n = m_ch2counts.at(ch).m_remaining;
            return n == 0 || n == PUZ_UNKNOWN;
        };
        return f(PUZ_POSITIVE) && f(PUZ_NEGATIVE);
    }

    // the index of the area
    int m_index;
    // key : all chars used to fill a position
    map<char, puz_counts> m_ch2counts;
};

struct puz_group : vector<puz_area>
{
    bool is_ready() const {
        return boost::algorithm::all_of(*this, [](const puz_area& a) {
            return a.is_ready();
        });
    }
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
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);
    bool make_move2(const Position& p, char ch);
    void check_area(const puz_area& a, char ch) {
        if (a.cant_use(ch))
            for (auto& p : m_game->m_areas[a.m_index])
                remove_pair(p, ch);
    }
    // remove the possibility of filling the position p with char ch
    void remove_pair(const Position& p, char ch) {
        if (auto i = m_pos2chars.find(p); i != m_pos2chars.end())
            boost::remove_erase(i->second, ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::count(m_cells, PUZ_SPACE);}
    unsigned int get_distance(const puz_state& child) const {return 2;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    puz_group m_groups;
    map<Position, puz_chars> m_pos2chars;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c)
            if (Position p(r, c); g.cells(p) == PUZ_EMPTY)
                cells(p) = PUZ_EMPTY;
            else
                m_pos2chars[p] = all_chars;

    for (int i = 0; i < sidelen() * 2; i++) {
        auto& [positive, negative] = g.m_hints[i];
        auto& a = m_groups.emplace_back(i + g.m_rect_count, positive, negative, sidelen());
        for (char ch : all_chars)
            check_area(a, ch);
    }
}

bool puz_state::make_move(const Position& p, char ch)
{
    if (!make_move2(p, ch))
        return false;

    // The rectangle should be handled within one move
    auto rng = m_game->m_areas.at(m_game->m_pos2rect.at(p));
    boost::remove_erase(rng, p);
    auto& p2 = rng[0];

    // We either fill the rectangle with a Magnet or mark it as empty
    char ch2 =
        ch == PUZ_EMPTY ? PUZ_EMPTY :
        ch == PUZ_POSITIVE ? PUZ_NEGATIVE :
        PUZ_POSITIVE;
    return make_move2(p2, ch2);
}

bool puz_state::make_move2(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2chars.erase(p);

    for (auto a : {&m_groups[p.first], &m_groups[p.second + sidelen()]}) {
        if (!a->fill_cell(p, ch))
            return false;
        check_area(*a, ch);
    }

    // respect the rule of poles
    if (ch != PUZ_EMPTY)
        for (auto& os : offset)
            if (auto p2 = p + os; is_valid(p2))
                remove_pair(p2, ch);

    return boost::algorithm::none_of(m_pos2chars, [](const pair<const Position, puz_chars>& kv) {
        return kv.second.empty();
    }) && (!is_goal_state() || m_groups.is_ready());
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, chars] = *boost::min_element(m_pos2chars, [](
        const pair<const Position, puz_chars>& kv1,
        const pair<const Position, puz_chars>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (char ch : chars)
        if (!children.emplace_back(*this).make_move(p, ch))
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
                    m_groups[is_row ? r : c + sidelen()].m_ch2counts.at
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
