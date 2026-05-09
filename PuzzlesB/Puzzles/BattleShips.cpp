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

const string ship_info[][2] = {
    {"o", "o"},
    {"<>", "^v"},
    {"<+>", "^+v"},
    {"<++>", "^++v"},
    {"<+++>", "^+++v"},
};

struct puz_move
{
    int m_kind;
    bool m_is_vert;
    map<Position, char> m_piece2char;
    set<Position> m_neighbors;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    bool m_has_supertanker;
    map<int, int> m_kind2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, char> m_pos2piece;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    map<int, vector<int>> m_kind2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() - 1)
, m_has_supertanker(level.attribute("SuperTanker").as_int() == 1)
, m_piece_counts_rows(m_sidelen)
, m_piece_counts_cols(m_sidelen)
{
    if (m_has_supertanker)
        m_kind2num[5] = 1;

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
                m_pos2piece[{r, c}] = ch;
                break;
            default:
                (c == m_sidelen ? m_piece_counts_rows[r] : m_piece_counts_cols[c]) = ch - '0';
                break;
            }
        }
    }

    for (const auto& [i, n] : m_kind2num)
        for (int j = 0; j < 2; ++j) {
            bool vert = j == 1;
            if (i == 1 && vert)
                continue;
            auto& chars = ship_info[i - 1][j];
            int len = chars.length();
            for (int r = 0; r < m_sidelen - (!vert ? 0 : len - 1); ++r)
                for (int c = 0; c < m_sidelen - (vert ? 0 : len - 1); ++c) {
                    int n = m_moves.size();
                    auto& [kind, is_vert, piece2char, neighbors] = m_moves.emplace_back();
                    kind = i, is_vert = vert;
                    for (int i = 0; i < len; ++i)
                        piece2char[{!vert ? r : r + i, vert ? c : c + i}] = chars[i];
                    for (int dr = -1; dr <= 1; ++dr)
                        for (int dc = -1; dc <= len; ++dc)
                            if (!(dr == 0 && dc >= 0 && dc < len))
                                if (Position p2(!vert ? r + dr : r + dc, vert ? c + dr : c + dc); is_valid(p2))
                                    neighbors.insert(p2);
                    m_kind2move_ids[i].push_back(n);
                    for (const auto& [p2, ch] : m_pos2piece)
                        if (auto& [r2, c2] = p2;
                            !vert && r == r2 && c <= c2 && c2 < c + len && chars[c2 - c] == ch || vert && c == c2 && r <= r2 && r2 < r + len && chars[r2 - r] == ch)
                            m_pos2move_ids[p2].push_back(n);
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
    bool make_move(int i, const Position& p, int j);
    void make_move2(int i, const Position& p, int j);
    int find_matches(bool init);
    void check_area();

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_kind2num, 0, [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    map<int, int> m_kind2num;
    map<Position, vector<int>> m_pos_matches;
    map<int, vector<int>> m_kind_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
, m_piece_counts_rows(g.m_piece_counts_rows)
, m_piece_counts_cols(g.m_piece_counts_cols)
, m_kind2num(g.m_kind2num)
, m_pos_matches(g.m_pos2move_ids)
, m_kind_matches(g.m_kind2move_ids)
{
    check_area();
    find_matches(true);
}

void puz_state::check_area()
{
    auto f = [](char& ch) {
        if (ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
    };
    for (int i = 0; i < sidelen(); ++i) {
        if (m_piece_counts_rows[i] == 0)
            for (int j = 0; j < sidelen(); ++j)
                f(cells({i, j}));
        if (m_piece_counts_cols[i] == 0)
            for (int j = 0; j < sidelen(); ++j)
                f(cells({j, i}));
    }
}

int puz_state::find_matches(bool init)
{
    auto f = [&](vector<int>& move_ids) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_1, is_vert, piece2char, neighbors] = m_game->m_moves[id];
            auto& p = piece2char.begin()->first;
            return !boost::algorithm::all_of(piece2char, [&](const pair<const Position, char>& kv) {
                return cells(kv.first) == PUZ_SPACE;
            }) || !boost::algorithm::all_of(neighbors, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY;
            }) || (!is_vert ? m_piece_counts_rows : m_piece_counts_cols)[!is_vert ? p.first : p.second] < piece2char.size();
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
                    return make_move2(-1, p, move_ids[0]), 1;
                }
        }
    else
        for (auto& [i, move_ids] : m_kind_matches) {
            f(move_ids);
            if (!init)
                switch(move_ids.size()) {
                case 0:
                    return 0;
                case 1:
                    return make_move2(i, Position::Zero, move_ids[0]), 1;
                }
        }
    return (!is_goal_state() ?
        boost::algorithm::all_of(m_kind2num, [&](const pair<const int, int>& kv) {
            return m_kind_matches[kv.first].size() >= kv.second;
        }) :
        boost::algorithm::all_of_equal(m_piece_counts_rows, 0) &&
            boost::algorithm::all_of_equal(m_piece_counts_cols, 0)) ? 2 : 0;
}

void puz_state::make_move2(int i, const Position& p, int j)
{
    auto& [kind, _2, piece2char, neighbors] = m_game->m_moves[j];
    for (auto& [p2, ch] : piece2char) {
        cells(p2) = ch;
        m_piece_counts_rows[p2.first]--;
        m_piece_counts_cols[p2.second]--;
    }
    for (auto& p2 : neighbors)
        cells(p2) = PUZ_EMPTY;
    if (--m_kind2num.at(kind) == 0)
        m_kind2num.erase(kind), m_kind_matches.erase(kind);
    ++m_distance;
    if (i == -1)
        m_pos_matches.erase(p);
    check_area();
}

bool puz_state::make_move(int i, const Position& p, int j)
{
    m_distance = 0;
    make_move2(i, p, j);
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
            if (!children.emplace_back(*this).make_move(-1, p, n))
                children.pop_back();
    } else {
        auto& [i, move_ids] = *boost::min_element(m_kind_matches, [](
            const pair<const int, vector<int>>& kv1,
            const pair<const int, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : move_ids)
            if (!children.emplace_back(*this).make_move(i, Position::Zero, n))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen())
                out << m_game->m_piece_counts_rows[r] << ' ';
            else if (r == sidelen())
                out << m_game->m_piece_counts_cols[c] << ' ';
            else
                out << cells({r, c}) << ' ';
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
