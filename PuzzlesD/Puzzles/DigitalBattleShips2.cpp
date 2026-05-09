#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 13/Digital Battle Ships

    Summary
    Please divert your course 12+1+2 to avoid collision

    Description
    1. Play like Solo Battle Ships, with a difference.
    2. Each number on the outer board tells you the SUM of the ship or
       ship pieces you're seeing in that row or column.
    3. A ship or ship piece is worth the number it occupies on the board.
    4. Standard rules apply: a ship or piece of ship can't touch another,
       not even diagonally.
    5. In each puzzle there are
       1 Aircraft Carrier (4 squares)
       2 Destroyers (3 squares)
       3 Submarines (2 squares)
       4 Patrol boats (1 square)

    Variant
    5. Some puzzle can also have a:
       1 Supertanker (5 squares)
*/

namespace puzzles::DigitalBattleships2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TOP = '^';
constexpr auto PUZ_BOTTOM = 'v';
constexpr auto PUZ_LEFT = '<';
constexpr auto PUZ_RIGHT = '>';
constexpr auto PUZ_MIDDLE = '+';
constexpr auto PUZ_BOAT = 'o';
constexpr auto PUZ_UNKNOWN = -1;

const string ship_info[][2] = {
    {"o", "o"},
    {"<>", "^v"},
    {"<+>", "^+v"},
    {"<++>", "^++v"},
    {"<+++>", "^+++v"},
};

struct puz_move
{
    int m_ship;
    bool m_is_vert;
    map<Position, char> m_pos2char;
    set<Position> m_neighbors;
    map<int, int> m_area2num;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_has_supertanker;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, int> m_pos2num;
    map<int, int> m_area2num;
    vector<puz_move> m_moves;
    map<int, vector<int>> m_ship2move_ids, m_area2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() - 1)
, m_has_supertanker(level.attribute("SuperTanker").as_int() == 1)
{
    if (m_has_supertanker)
        m_ship2num[5] = 1;

    for (int r = 0; r <= m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c <= m_sidelen; ++c) {
            Position p(r, c);
            if (r == m_sidelen && c == m_sidelen)
                break;
            auto s = str.substr(c * 2, 2);
            if (s == "  ")
                continue;
            int n = stoi(string(s));
            if (r == m_sidelen || c == m_sidelen)
                m_area2num[c == m_sidelen ? r : c + m_sidelen] = n;
            else
                m_pos2num[p] = n;
        }
    }

    for (const auto& [i, n] : m_ship2num)
        for (int j = 0; j < 2; ++j) {
            bool vert = j == 1;
            if (i == 1 && vert)
                continue;
            auto& chars = ship_info[i - 1][j];
            int len = chars.length();
            for (int r = 0; r < m_sidelen - (!vert ? 0 : len - 1); ++r)
                for (int c = 0; c < m_sidelen - (vert ? 0 : len - 1); ++c) {
                    int n = m_moves.size();
                    auto& [ship, is_vert, pos2char, neighbors, area2num] = m_moves.emplace_back();
                    ship = i, is_vert = vert;
                    for (int dr = -1; dr <= 1; ++dr)
                        for (int dc = -1; dc <= len; ++dc)
                            if (dr == 0 && dc >= 0 && dc < len) {
                                int r2 = !vert ? r : r + dc, c2 = vert ? c : c + dc;
                                Position p2(r2, c2);
                                int num = m_pos2num.at(p2);
                                area2num[r2] += num, area2num[c2 + m_sidelen] += num;
                                pos2char[p2] = chars[dc];
                            } else if (Position p2(!vert ? r + dr : r + dc, vert ? c + dr : c + dc); is_valid(p2))
                                neighbors.insert(p2);
                    m_ship2move_ids[i].push_back(n);
                    for (auto& [a, num] : m_area2num)
                        if (auto it = area2num.find(a);
                            it != area2num.end() && it->second <= num)
                            m_area2move_ids[a].push_back(n);
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
        return tie(m_cells, m_area_matches) < tie(x.m_cells, x.m_area_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    void check_area();

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        }) + m_area_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_area2num;
    map<int, int> m_ship2num;
    map<int, vector<int>> m_ship_matches, m_area_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_area2num(g.m_area2num)
, m_ship2num(g.m_ship2num)
, m_ship_matches(g.m_ship2move_ids)
, m_area_matches(g.m_area2move_ids)
{
    check_area();
    find_matches(true);
}

void puz_state::check_area()
{
    for (auto it = m_area2num.begin(); it != m_area2num.end();)
        if (auto [a, num] = *it; num != 0)
            it++;
        else {
            bool is_row = a < sidelen();
            for (int i = 0; i < sidelen(); ++i)
                if (char& ch = cells({is_row ? a : i, is_row ? i : a - sidelen()}); ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
            it = m_area2num.erase(it);
            m_area_matches.erase(a);
            ++m_distance;
        }
}

int puz_state::find_matches(bool init)
{
    auto f = [&](vector<int>& move_ids) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [ship, _1, pos2char, neighbors, area2num] = m_game->m_moves[id];
            auto it = m_ship2num.find(ship);
            return !(it != m_ship2num.end() && m_ship_matches.at(ship).size() >= it->second
            && boost::algorithm::all_of(pos2char, [&](const pair<const Position, char>& kv) {
                return cells(kv.first) == PUZ_SPACE;
            }) && boost::algorithm::all_of(neighbors, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            }) && boost::algorithm::all_of(area2num, [&](const pair<const int, int>& kv) {
                auto it = m_area2num.find(kv.first);
                return it == m_area2num.end() || it->second >= kv.second;
            }));
        });
    };
    for (auto& [_1, move_ids] : m_ship_matches) {
        f(move_ids);
        if (!init)
            switch(move_ids.size()) {
                case 0:
                    return 0;
                case 1:
                    return make_move2(move_ids[0]), 1;
            }
    }
    for (auto& [_1, move_ids] : m_area_matches) {
        f(move_ids);
        if (!init)
            switch(move_ids.size()) {
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
    auto& [ship, _1, pos2char, neighbors, area2num] = m_game->m_moves[n];
    for (auto& [p2, ch] : pos2char)
        cells(p2) = ch;
    for (auto& p2 : neighbors)
        cells(p2) = PUZ_EMPTY;
    for (auto& [a, num] : area2num)
        if (auto it = m_area2num.find(a); it != m_area2num.end())
            it->second -= num;
    if (--m_ship2num.at(ship) == 0)
        m_ship2num.erase(ship), m_ship_matches.erase(ship);
    ++m_distance;
    check_area();
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
    auto& [_1, move_ids1] = *boost::min_element(m_ship_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    auto& [_2, move_ids2] = *boost::min_element(m_area_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    auto& move_ids = move_ids1.size() <= move_ids2.size() ? move_ids1 : move_ids2;
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int i) {
        int sum = 0;
        bool is_row = i < sidelen();
        for (int j = 0; j < sidelen(); ++j) {
            Position p(is_row ? i : j, is_row ? j : i - sidelen());
            if (char ch = cells(p); ch != PUZ_EMPTY && ch != PUZ_SPACE)
                sum += m_game->m_pos2num.at(p);
        }
        return sum;
    };
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen() || r == sidelen())
                out << format("{:2}",
                f(c == sidelen() ? r : c + sidelen()))
                << ' ';
            else {
                Position p(r, c);
                char ch = cells(p);
                if (ch == PUZ_EMPTY || ch == PUZ_SPACE)
                    out << PUZ_EMPTY << ' ';
                else
                    out << ch << m_game->m_pos2num.at(p);
                out << ' ';
            }
        println(out);
    }
    return out;
}

}

void solve_puz_DigitalBattleships2()
{
    using namespace puzzles::DigitalBattleships2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DigitalBattleships.xml", "Puzzles/DigitalBattleships.txt", solution_format::GOAL_STATE_ONLY);
}
