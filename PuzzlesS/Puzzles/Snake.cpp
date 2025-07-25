#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 16/Snake

    Summary
    Still lives inside your pocket-sized computer

    Description
    1. Complete the Snake, head to tail, inside the board.
    2. The two tiles given at the start are the head and the tail of the snake
       (it is irrelevant which is which).
    3. Numbers on the border tell you how many tiles the snake occupies in that
       row or column.
    4. The snake can't touch itself, not even diagonally.
*/

namespace puzzles::Snake{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SNAKE = 'S';
constexpr auto PUZ_UNKNOWN = -1;

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_area2piece_count;
    Position m_head_tail[2];
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions forming the area
    vector<vector<Position>> m_area2range;
    map<int, vector<string>> m_num2perms;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_area2piece_count(m_sidelen * 2)
    , m_area2range(m_sidelen * 2)
    , m_cells(m_sidelen * m_sidelen, PUZ_SPACE)
{
    for (int r = 0, n = 0; r <= m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c <= m_sidelen; c++) {
            Position p(r, c);
            if (char ch = str[c]; ch == PUZ_SNAKE)
                cells(m_head_tail[n++] = p) = ch;
            else if (c == m_sidelen || r == m_sidelen) {
                if (c == m_sidelen && r == m_sidelen) continue;
                m_area2piece_count[c == m_sidelen ? r : c + m_sidelen]
                = isdigit(ch) ? ch - '0' : PUZ_UNKNOWN;
            }
        }
    }
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }

    auto& perms_unknown = m_num2perms[PUZ_UNKNOWN];
    for (int i = 0; i <= m_sidelen; ++i) {
        auto& perms = m_num2perms[i];
        auto perm = string(m_sidelen - i, PUZ_EMPTY) + string(i, PUZ_SNAKE);
        do {
            perms.push_back(perm);
            perms_unknown.push_back(perm);
        } while (boost::next_permutation(perm));
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
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);
    bool dec(int& n) const { return n == PUZ_UNKNOWN ? false : (--n, true); }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_area2piece_count, 0, [](int acc, int n) {
            return acc + (n == PUZ_UNKNOWN ? 0 : n);
        }) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    vector<int> m_area2piece_count;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g)
    , m_area2piece_count(g.m_area2piece_count)
{
    for (int i = 0; i < sidelen() * 2; ++i) {
        auto& perm_ids = m_matches[i];
        perm_ids.resize(g.m_num2perms.at(m_area2piece_count[i]).size());
        boost::iota(perm_ids, 0);
    }

    for (auto& p : g.m_head_tail)
        for (int i : {p.first, p.second + sidelen()})
            dec(m_area2piece_count[i]);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, perm_ids] : m_matches) {
        auto& range = m_game->m_area2range[area_id];
        auto& perms = m_game->m_num2perms.at(m_game->m_area2piece_count[area_id]);
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(range, perms[id], [&](const Position& p, char ch2) {
                char ch1 = cells(p);
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_area2range[i];
    auto& perm = m_game->m_num2perms.at(m_game->m_area2piece_count[i])[j];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p = range[k];
        if (char& ch = cells(p); ch == PUZ_SPACE && (ch = perm[k]) == PUZ_SNAKE)
            for (int i : {p.first, p.second + sidelen()})
                if (dec(m_area2piece_count[i]))
                    ++m_distance;
    }
    m_matches.erase(i); ++m_distance;
    
    bool b1 = is_goal_state();
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_SNAKE) continue;
            int n1 = boost::count_if(offset, [&](const Position& os) {
                auto p2 = p + os;
                return is_valid(p2) && cells(p2) == PUZ_SNAKE;
            });
            int n2 = boost::count_if(offset, [&](const Position& os) {
                auto p2 = p + os;
                return !is_valid(p2) || cells(p2) == PUZ_EMPTY;
            });
            bool b2 = p == m_game->m_head_tail[0] || p == m_game->m_head_tail[1];
            if (!(b2 && (!b1 && n1 <= 1 && n2 <= 3 || b1 && n1 == 1 && n2 == 3) ||
                !b2 && (!b1 && n1 <= 2 && n2 <= 2 || b1 && n1 == 2 && n2 == 2)))
                return false;
        }
    return true;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if (!make_move2(i, j))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids)
        if (children.push_back(*this); !children.back().make_move(area_id, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int i) {
        return boost::count_if(m_game->m_area2range[i], [&](const Position& p) {
            return cells(p) == PUZ_SNAKE;
        });
    };
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen())
                out << format("{:<2}", f(r));
            else if (r == sidelen())
                out << format("{:<2}", f(c + sidelen()));
            else
                out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_Snake()
{
    using namespace puzzles::Snake;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Snake.xml", "Puzzles/Snake.txt", solution_format::GOAL_STATE_ONLY);
}
