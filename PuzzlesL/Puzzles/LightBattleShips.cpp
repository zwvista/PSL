#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Light Battle Ships

    Summary
    Please divert your course 15 degrees to avoid collision

    Description
    1. A mix of Battle Ships and Lighthouses, you have to guess the usual
       piece of ships with the help of Lighthouses.
    2. Each number is a Lighthouse, telling you how many pieces of ship
       there are in that row and column, summed together.
    3. Ships cannot touch each other OR touch Lighthouses. Not even diagonally.
    4. In each puzzle there are
       1 Aircraft Carrier (4 squares)
       2 Destroyers (3 squares)
       3 Submarines (2 squares)
       4 Patrol boats (1 square)

    Variant
    5. Some puzzle can also have a:
       1 Supertanker (5 squares)
*/

namespace puzzles::LightBattleships{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_LIGHT = 'L';
constexpr auto PUZ_TOP = '^';
constexpr auto PUZ_BOTTOM = 'v';
constexpr auto PUZ_LEFT = '<';
constexpr auto PUZ_RIGHT = '>';
constexpr auto PUZ_MIDDLE = '+';
constexpr auto PUZ_BOAT = 'o';

constexpr array<Position, 8> offset = Position::Directions8;

struct puz_ship_info {
    // symbols that represent the ship
    string m_pieces[2];
    // the area occupied by the ship
    string m_area[3];
};

const puz_ship_info ship_info[] = {
    {{"o", "o"}, {"...", ". .", "..."}},
    {{"<>", "^v"}, {"....", ".  .", "...."}},
    {{"<+>", "^+v"}, {".....", ".   .", "....."}},
    {{"<++>", "^++v"}, {"......", ".    .", "......"}},
    {{"<+++>", "^+++v"}, {".......", ".     .", "......."}},
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    bool m_has_supertanker;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, char> m_pos2piece;
    map<Position, int> m_pos2light;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_has_supertanker(level.attribute("SuperTanker").as_int() == 1)
{
    if (m_has_supertanker)
        m_ship2num[5] = 1;

    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            switch(char ch = str[c]) {
            case PUZ_SPACE:
            case PUZ_EMPTY:
                m_cells.push_back(ch);
                break;
            case PUZ_BOAT:
            case PUZ_TOP:
            case PUZ_BOTTOM:
            case PUZ_LEFT:
            case PUZ_RIGHT:
            case PUZ_MIDDLE:
                m_cells.push_back(PUZ_SPACE);
                m_pos2piece[p] = ch;
                break;
            default:
                m_cells.push_back(PUZ_LIGHT);
                m_pos2light[p] = ch - '0';
                break;
            };
        }
    }
}

// key: the position of the ship piece
// value.elem.0: the kind of the ship
// value.elem.1: the position of the ship
// value.elem.2: false if the ship is horizontal, true if vertical
using puz_pos_match = map<Position, vector<tuple<int, Position, bool>>>;
// key: the kind of the ship
// value.elem.first: the position of the ship
// value.elem.second: false if the ship is horizontal, true if vertical
using puz_ship_match = map<int, vector<pair<Position, bool>>>;

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
    bool make_move(const Position& p_piece, const Position& p, int n, bool vert);
    void check_area();
    void find_matches();

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_ship2num;
    map<Position, char> m_pos2piece;
    map<Position, int> m_pos2light;
    puz_pos_match m_pos_matches;
    puz_ship_match m_ship_matches;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
, m_ship2num(g.m_ship2num), m_pos2piece(g.m_pos2piece)
, m_pos2light(g.m_pos2light)
{
    // 3. Ships cannot touch Lighthouses. Not even diagonally.
    for (auto& [p2, light] : g.m_pos2light)
        for (auto& os : offset) {
            auto p = p2 + os;
            if (!is_valid(p)) continue;
            char& ch = cells(p);
            if (ch == PUZ_SPACE)
                ch = PUZ_EMPTY;
        }

    check_area();
    find_matches();
}

void puz_state::check_area()
{
    auto f = [](char& ch) {
        if (ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
    };

    for (auto& [p, light] : m_pos2light)
        if (light == 0) {
            for (int r = 0; r < sidelen(); ++r)
                f(cells({r, p.second}));
            for (int c = 0; c < sidelen(); ++c)
                f(cells({p.first, c}));
        }
}

void puz_state::find_matches()
{
    m_pos_matches.clear();
    m_ship_matches.clear();
    for (const auto& [i, n] : m_ship2num) {
        for (int j = 0; j < 2; ++j) {
            bool vert = j == 1;
            if (i == 1 && vert)
                continue;

            auto& s = ship_info[i - 1].m_pieces[j];
            int len = s.length();

            auto f = [&](Position p) {
                auto os = vert ? Position(1, 0) : Position(0, 1);
                for (char ch : s) {
                    if (!is_valid(p) || cells(p) != PUZ_SPACE)
                        return false;
                    p += os;
                }
                return true;
            };

            if (!m_pos2piece.empty())
                for (const auto& [p, ch] : m_pos2piece) {
                    auto& [r, c] = p;
                    if (!vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv) {
                        return kv.first.first == r && kv.second < len;
                    }) || vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv) {
                        return kv.first.second == c && kv.second < len;
                    }))
                        continue;

                    // 0
                    // < + + >
                    //       len-1
                    for (int k = 0; k < len; ++k) {
                        if (s[k] != ch)
                            continue;
                        auto p2 = p - (vert ? Position(k, 0) : Position(0, k));
                        if (f(p2))
                            m_pos_matches[p].emplace_back(i, p2 + Position(-1, -1), vert);
                    }
                } else
                for (int r = 0; r < sidelen() - (!vert ? 0 : len - 1); ++r) {
                    if (!vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv) {
                        return kv.first.first == r && kv.second < len;
                    }))
                        continue;
                    for (int c = 0; c < sidelen() - (vert ? 0 : len - 1); ++c) {
                        if (vert && boost::algorithm::any_of(m_pos2light, [=](const pair<const Position, int>& kv) {
                            return kv.first.second == c && kv.second < len;
                        }))
                            continue;
                        Position p(r, c);
                        if (f(p))
                            m_ship_matches[i].emplace_back(p + Position(-1, -1), vert);
                    }
                }
        }
    }
}

bool puz_state::make_move(const Position& p_piece, const Position& p, int n, bool vert)
{
    auto& info = ship_info[n - 1];
    int len = info.m_pieces[0].length();
    for (int r2 = 0; r2 < 3; ++r2)
        for (int c2 = 0; c2 < len + 2; ++c2) {
            auto p2 = p + Position(!vert ? r2 : c2, !vert ? c2 : r2);
            if (!is_valid(p2)) continue;
            char& ch = cells(p2);
            char ch2 = info.m_area[r2][c2];
            if (ch2 == ' ') {
                ch = info.m_pieces[!vert ? 0 : 1][c2 - 1];
                for (auto& [p3, light] : m_pos2light)
                    if (p3.first == p2.first || p3.second == p2.second)
                        --light;
            } else if (ch == PUZ_SPACE)
                ch = PUZ_EMPTY;
        }


    if (--m_ship2num[n] == 0)
        m_ship2num.erase(n);
    m_pos2piece.erase(p_piece);
    check_area();
    find_matches();

    if (!m_pos2piece.empty())
        return boost::algorithm::all_of(m_pos2piece, [&](const pair<const Position, char>& kv) {
            return m_pos_matches.contains(kv.first);
        });
    else if (!is_goal_state())
        return boost::algorithm::all_of(m_ship2num, [&](const pair<const int, int>& kv) {
            return m_ship_matches[kv.first].size() >= kv.second;
        });
    else
        return boost::algorithm::all_of(m_pos2light, [](const pair<const Position, int>& kv) {
            return kv.second == 0;
        });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_pos2piece.empty()) {
        auto& [p_piece, infos] = *boost::min_element(m_pos_matches, [](
            const puz_pos_match::value_type& kv1,
            const puz_pos_match::value_type& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (auto& [i, p, vert] : infos)
            if (!children.emplace_back(*this).make_move(p_piece, p, i, vert))
                children.pop_back();
    } else {
        auto& [n, infos] = *boost::min_element(m_ship_matches, [](
            const puz_ship_match::value_type& kv1,
            const puz_ship_match::value_type& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (auto& [p, vert] : infos)
            if (!children.emplace_back(*this).make_move(p, p, n, vert))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_LIGHT)
                out << format("{:<2}", m_game->m_pos2light.at(p));
            else
                out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_LightBattleships()
{
    using namespace puzzles::LightBattleships;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/LightBattleships.xml", "Puzzles/LightBattleships.txt", solution_format::GOAL_STATE_ONLY);
}
