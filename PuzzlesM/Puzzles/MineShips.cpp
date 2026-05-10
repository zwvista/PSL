#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 8/Mine Ships

    Summary
    Warning! Naval Mines in the water!

    Description
    1. There are actually no mines in the water, but this is a mix between
       Minesweeper and Battle Ships.
    2. You must find the same set of ships like 'Battle Ships'
       (1*4, 2*3, 3*2, 4*1).
    3. However this time the hints are given in the same form as 'Minesweeper',
       where a number tells you how many pieces of ship are around it.
    4. Usual Battle Ships rules apply!

    Variant
    5. Some puzzle can also have a:
       1 Supertanker (5 squares)
*/

namespace puzzles::MineShips{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_NUMBER = 'N';

constexpr array<Position, 8> offset = Position::Directions8;

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
    map<Position, int> m_mine2count;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_has_supertanker;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, int> m_mine2count;
    string m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_mine2move_ids;
    map<int, vector<int>> m_ship2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
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
            char ch = str[c];
            m_cells.push_back(ch == PUZ_SPACE ? PUZ_SPACE : PUZ_NUMBER);
            if (ch != PUZ_SPACE)
                m_mine2count[{r, c}] = ch - '0';
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
                for (int c = 0; c < m_sidelen - (vert ? 0 : len - 1); ++c)
                    if ([&]{
                        for (int dc = 0; dc < len; ++dc)
                            if (cells({!vert ? r : r + dc, vert ? c : c + dc}) != PUZ_SPACE)
                                return false;
                        return true;
                    }()) {
                        int n = m_moves.size();
                        auto& [ship, is_vert, pos2char, neighbors, mine2count] = m_moves.emplace_back();
                        ship = i, is_vert = vert;
                        for (int dr = -1; dr <= 1; ++dr)
                            for (int dc = -1; dc <= len; ++dc)
                                if (dr == 0 && dc >= 0 && dc < len) {
                                    Position p2(!vert ? r : r + dc, vert ? c : c + dc);
                                    pos2char[p2] = chars[dc];
                                    for (auto& [p3, _1] : m_mine2count)
                                        if (boost::algorithm::any_of_equal(offset, p2 - p3))
                                            mine2count[p3]++, m_mine2move_ids[p3].push_back(n);
                                } else if (Position p2(!vert ? r + dr : r + dc, vert ? c + dr : c + dc); is_valid(p2))
                                    neighbors.insert(p2);
                        m_ship2move_ids[i].push_back(n);
                    }
        }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_mine_matches) < tie(x.m_cells, x.m_mine_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    void check_mine();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv) {
            return acc + kv.second;
        }) + m_mine_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, int> m_mine2count;
    map<int, int> m_ship2num;
    map<Position, vector<int>> m_mine_matches;
    map<int, vector<int>> m_ship_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_mine2count(g.m_mine2count)
, m_ship2num(g.m_ship2num)
, m_ship_matches(g.m_ship2move_ids)
, m_mine_matches(g.m_mine2move_ids)
{
    check_mine();
    find_matches(true);
}

void puz_state::check_mine()
{
    for (auto it = m_mine2count.begin(); it != m_mine2count.end();)
        if (auto [p, cnt] = *it; cnt != 0)
            it++;
        else {
            for (auto& os : offset)
                if (auto p2 = p + os; is_valid(p2))
                    if (char& ch = cells(p2); ch == PUZ_SPACE)
                        ch = PUZ_EMPTY;
            it = m_mine2count.erase(it);
            m_mine_matches.erase(p);
            ++m_distance;
        }
}

int puz_state::find_matches(bool init)
{
    auto f = [&](vector<int>& move_ids) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [ship, _1, pos2char, neighbors, mine2count] = m_game->m_moves[id];
            auto it = m_ship2num.find(ship);
            return !(it != m_ship2num.end() && m_ship_matches.at(ship).size() >= it->second
            && boost::algorithm::all_of(pos2char, [&](const pair<const Position, char>& kv) {
                return cells(kv.first) == PUZ_SPACE;
            }) && boost::algorithm::all_of(neighbors, [&](const Position& p2) {
                char ch2 = cells(p2);
                return ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY || ch2 == PUZ_NUMBER;
            }) && boost::algorithm::all_of(mine2count, [&](const pair<const Position, int>& kv) {
                auto it = m_mine2count.find(kv.first);
                return it != m_mine2count.end() && it->second >= kv.second;
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
    for (auto& [_1, move_ids] : m_mine_matches) {
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
    auto& [ship, _1, pos2char, neighbors, mine2count] = m_game->m_moves[n];
    for (auto& [p2, ch] : pos2char)
        cells(p2) = ch;
    for (auto& p2 : neighbors)
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            cells(p2) = PUZ_EMPTY;
    for (auto& [p2, cnt] : mine2count)
        m_mine2count.at(p2) -= cnt;
    if (--m_ship2num.at(ship) == 0)
        m_ship2num.erase(ship), m_ship_matches.erase(ship);
    ++m_distance;
    check_mine();
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
    auto& [_2, move_ids2] = *boost::min_element(m_mine_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    auto& move_ids = move_ids1.size() <= move_ids2.size() ? move_ids1 : move_ids2;
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_NUMBER)
                out << format("{:<2}", m_game->m_mine2count.at(p));
            else {
                if (ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
                out << format("{:<2}", ch);
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_MineShips()
{
    using namespace puzzles::MineShips;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MineShips.xml", "Puzzles/MineShips.txt", solution_format::GOAL_STATE_ONLY);
}
