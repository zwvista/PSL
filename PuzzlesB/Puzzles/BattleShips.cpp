#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 1/Battle Ships

    Summary
    Play solo Battleships, with the help of the numbers on the border.

    Description
    1. Standard rules of Battleships apply, but you are guessing the other
       player ships disposition, by using the numbers on the borders.
    2. Each number tells you how many ship or ship pieces you're seeing in
       that row or column.
    3. Standard rules apply: a ship or piece of ship can't touch another,
       not even diagonally.
    4. In each puzzle there are
       1 Aircraft Carrier (4 squares)
       2 Destroyers (3 squares)
       3 Submarines (2 squares)
       4 Patrol boats (1 square)

    Variant
    5. Some puzzle can also have a:
       1 Supertanker (5 squares)
*/

namespace puzzles::BattleShips{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_TOP = '^';
constexpr auto PUZ_BOTTOM = 'v';
constexpr auto PUZ_LEFT = '<';
constexpr auto PUZ_RIGHT = '>';
constexpr auto PUZ_MIDDLE = '+';
constexpr auto PUZ_BOAT = 'o';
constexpr auto PUZ_NOT_POS = Position(-1, -1);

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
    map<int, int> m_area2count;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<int, int> m_area2count;
    bool m_has_supertanker;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, char> m_pos2char;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
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
            if (r == m_sidelen && c == m_sidelen)
                break;
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
                m_pos2char[{r, c}] = ch;
                break;
            default:
                m_area2count[c == m_sidelen ? r : c + m_sidelen] = ch - '0';
                break;
            }
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
                    auto& [ship, is_vert, pos2char, neighbors, area2count] = m_moves.emplace_back();
                    ship = i, is_vert = vert;
                    for (int dr = -1; dr <= 1; ++dr)
                        for (int dc = -1; dc <= len; ++dc)
                            if (dr == 0 && dc >= 0 && dc < len) {
                                int r2 = !vert ? r : r + dc, c2 = vert ? c : c + dc;
                                area2count[r2]++, area2count[c2 + m_sidelen]++;
                                pos2char[{r2, c2}] = chars[dc];
                            } else if (Position p2(!vert ? r + dr : r + dc, vert ? c + dr : c + dc); is_valid(p2))
                                neighbors.insert(p2);
                    m_ship2move_ids[i].push_back(n);
                    for (auto& [p2, ch] : m_pos2char)
                        if (auto it = pos2char.find(p2);
                            it != pos2char.end() && it->second == ch)
                            m_pos2move_ids[p2].push_back(n);
                    for (auto& [a, cnt] : m_area2count)
                        if (auto it = area2count.find(a);
                            it != area2count.end() && it->second <= cnt)
                            m_area2move_ids[a].push_back(n);
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
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    void check_area();

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        }) + m_pos_matches.size() + m_area_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_area2count;
    map<int, int> m_ship2num;
    map<Position, vector<int>> m_pos_matches;
    map<int, vector<int>> m_ship_matches, m_area_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
, m_area2count(g.m_area2count)
, m_ship2num(g.m_ship2num)
, m_pos_matches(g.m_pos2move_ids)
, m_ship_matches(g.m_ship2move_ids)
, m_area_matches(g.m_area2move_ids)
{
    check_area();
    find_matches(true);
}

void puz_state::check_area()
{
    for (auto it = m_area2count.begin(); it != m_area2count.end();)
        if (auto [a, cnt] = *it; cnt != 0)
            it++;
        else {
            bool is_row = a < sidelen();
            for (int i = 0; i < sidelen(); ++i)
                if (char& ch = cells({is_row ? a : i, is_row ? i : a - sidelen()}); ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
            it = m_area2count.erase(it);
            m_area_matches.erase(a);
            ++m_distance;
        }
}

int puz_state::find_matches(bool init)
{
    auto f = [&](vector<int>& move_ids) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [ship, _1, pos2char, neighbors, area2count] = m_game->m_moves[id];
            auto it = m_ship2num.find(ship);
            return !(it != m_ship2num.end() && m_ship_matches.at(ship).size() >= it->second
            && boost::algorithm::all_of(pos2char, [&](const pair<const Position, char>& kv) {
                return cells(kv.first) == PUZ_SPACE;
            }) && boost::algorithm::all_of(neighbors, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            }) && boost::algorithm::all_of(area2count, [&](const pair<const int, int>& kv) {
                auto it = m_area2count.find(kv.first);
                return it != m_area2count.end() && it->second >= kv.second;
            }));
        });
    };
    if (!m_pos_matches.empty())
        for (auto& [p, move_ids] : m_pos_matches) {
            f(move_ids);
            if (!init)
                switch(move_ids.size()) {
                case 0:
                    return 0;
                case 1:
                    return make_move2(p, move_ids[0]), 1;
                }
        }
    else {
        for (auto& [_1, move_ids] : m_ship_matches) {
            f(move_ids);
            if (!init)
                switch(move_ids.size()) {
                    case 0:
                        return 0;
                    case 1:
                        return make_move2(PUZ_NOT_POS, move_ids[0]), 1;
                }
        }
        for (auto& [_1, move_ids] : m_area_matches) {
            f(move_ids);
            if (!init)
                switch(move_ids.size()) {
                    case 0:
                        return 0;
                    case 1:
                        return make_move2(PUZ_NOT_POS, move_ids[0]), 1;
                }
        }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [ship, _1, pos2char, neighbors, area2count] = m_game->m_moves[n];
    for (auto& [p2, ch] : pos2char)
        cells(p2) = ch;
    for (auto& p2 : neighbors)
        cells(p2) = PUZ_EMPTY;
    for (auto& [a, cnt] : area2count)
        m_area2count.at(a) -= cnt;
    if (--m_ship2num.at(ship) == 0)
        m_ship2num.erase(ship), m_ship_matches.erase(ship);
    ++m_distance;
    if (p != PUZ_NOT_POS)
        m_pos_matches.erase(p), ++m_distance;
    check_area();
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_pos_matches.empty()) {
        auto& [p, move_ids] = *boost::min_element(m_pos_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : move_ids)
            if (!children.emplace_back(*this).make_move(p, n))
                children.pop_back();
    } else {
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
            if (!children.emplace_back(*this).make_move(PUZ_NOT_POS, n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c) {
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen())
                out << m_game->m_area2count.at(r) << ' ';
            else if (r == sidelen())
                out << m_game->m_area2count.at(c + sidelen()) << ' ';
            else
                out << cells({r, c}) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BattleShips()
{
    using namespace puzzles::BattleShips;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BattleShips.xml", "Puzzles/BattleShips.txt", solution_format::GOAL_STATE_ONLY);
}
