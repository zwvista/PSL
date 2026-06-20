#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/Gardener

    Summary
    Hitori Flower Planting

    Description
    1. The Board represents a Garden, divided in many rectangular Flowerbeds.
    2. The owner of the Garden wants you to plant Flowers according to these
       rules.
    3. A number tells you how many Flowers you must plant in that Flowerbed.
       A Flowerbed without number can have any quantity of Flowers.
    4. Flowers can't be horizontally or vertically touching.
    5. All the remaining Garden space where there are no Flowers must be
       interconnected (horizontally or vertically), as he wants to be able
       to reach every part of the Garden without treading over Flowers.
    6. Lastly, there must be enough balance in the Garden, so a straight
       line (horizontally or vertically) of non-planted tiles can't span
       for more than two Flowerbeds.
    7. In other words, a straight path of empty space can't pass through
       three or more Flowerbeds.
*/

namespace puzzles::Gardener{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FLOWER = 'F';
constexpr auto PUZ_BOUNDARY = '`';

constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;
constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_move
{
    set<Position> m_flowers, m_empties;
};

struct puz_fb_info
{
    vector<Position> m_area;
    int m_flower_count = PUZ_UNKNOWN;
    vector<puz_move> m_moves;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2fb;
    vector<puz_fb_info> m_fb_info;
    map<Position, int> m_pos2num;
    vector<vector<Position>> m_balanced_ranges;
    string m_cells;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
        if (!walls.contains(p_wall))
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2 + 2)
, m_cells(m_sidelen * m_sidelen, PUZ_SPACE)
{
    set<Position> rng;
    for (int r = 1;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2 - 2];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (str_h[c * 2 - 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen - 1) break;
        string_view str_v = strs[r * 2 - 1];
        for (int c = 1;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2 - 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen - 1) break;
            char ch = str_v[c * 2 - 1];
            if (ch != ' ')
                m_pos2num[p] = ch - '0';
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        auto& p_start = *rng.begin();
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p_start});
        auto& [area, flower_cnt, _1] = m_fb_info.emplace_back();
        if (auto it = m_pos2num.find(p_start); it != m_pos2num.end())
            flower_cnt = it->second;
        for (auto& p : smoves) {
            area.push_back(p);
            m_pos2fb[p] = n;
            rng.erase(p);
        }
        boost::sort(area);
    }

    for (int i = 0; i < m_sidelen; ++i)
        cells({i, 0}) = cells({i, m_sidelen - 1}) =
        cells({0, i}) = cells({m_sidelen - 1, i}) = PUZ_BOUNDARY;

    // 3. A number tells you how many Flowers you must plant in that Flowerbed.
    //    A Flowerbed without number can have any quantity of Flowers.
    map<pair<int, int>, vector<string>> pair2perms;
    for (auto& [area, flower_cnt, moves] : m_fb_info) {
        int area_sz = area.size();
        auto& perms = pair2perms[{area_sz, flower_cnt}];
        if (perms.empty())
            for (int i = 0; i <= area_sz; ++i) {
                if (flower_cnt != PUZ_UNKNOWN && flower_cnt != i) continue;
                auto perm = string(area_sz - i, PUZ_EMPTY) + string(i, PUZ_FLOWER);
                do
                    perms.push_back(perm);
                while (boost::next_permutation(perm));
            }

        for (const auto& perm : perms) {
            auto& [flowers, empties] = moves.emplace_back();
            for (int i = 0; i < perm.size(); ++i)
                (perm[i] == PUZ_EMPTY ? empties : flowers).insert(area[i]);
            // 4. Flowers can't be horizontally or vertically touching.
            if (![&]{
                for (const auto& p : flowers)
                    for (auto& os : offset)
                        if (auto p2 = p + os; flowers.contains(p2))
                            return false;
                        else if (cells(p2) == PUZ_SPACE)
                            empties.insert(p2);
                return true;
            }())
                moves.pop_back();
        }
    }

    // 6. Lastly, there must be enough balance in the Garden, so a straight
    //    line (horizontally or vertically) of non-planted tiles can't span
    //    for more than two Flowerbeds.
    // 7. In other words, a straight path of empty space can't pass through
    //    three or more Flowerbeds.
    auto f = [this](Position p, const Position& os) {
        int fb_last = -1;
        vector<vector<Position>> rng2D;
        vector<Position> rng;
        for (int i = 1; i < m_sidelen - 1; ++i) {
            int fb = m_pos2fb.at(p);
            if (fb != fb_last) {
                if (fb_last != -1)
                    rng2D.push_back(rng), rng.clear();
                fb_last = fb;
            }
            rng.push_back(p);
            p += os;
        }
        rng2D.push_back(rng);

        for (int i = 0; i < rng2D.size() - 2; ++i) {
            vector<Position> rng;
            rng.push_back(rng2D[i].back());
            rng.insert_range(rng.end(), rng2D[i + 1]);
            rng.push_back(rng2D[i + 2].front());
            m_balanced_ranges.push_back(rng);
        }
    };

    for (int i = 1; i < m_sidelen - 1; ++i) {
        f({i, 1}, {0, 1});        // e
        f({1, i}, {1, 0});        // s
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
    bool make_move(int fb_id, int move_id);
    void make_move2(int fb_id, int move_id);
    int find_matches(bool init);
    bool check_balance();
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches.size() + m_balanced_ids.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    vector<int> m_balanced_ids;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g)
, m_cells(g.m_cells)
, m_balanced_ids(g.m_balanced_ranges.size())
{
    boost::iota(m_balanced_ids, 0);

    for (int i = 0; i < g.m_fb_info.size(); ++i) {
        auto& v = m_matches[i];
        v.resize(g.m_fb_info[i].m_moves.size());
        boost::iota(v, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [fb_id, move_ids] : m_matches) {
        auto& [_1, _2, moves] = m_game->m_fb_info[fb_id];
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [flowers, empties] = moves[id];
            return !boost::algorithm::all_of(flowers, [&](const Position& p) {
                return cells(p) == PUZ_SPACE;
            }) || !boost::algorithm::all_of(empties, [&](const Position& p) {
                char ch = cells(p);
                return ch == PUZ_SPACE || ch == PUZ_EMPTY;
            });
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(fb_id, move_ids.front()), 1;
            }
    }
    return check_balance() && is_interconnected() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p)
        : m_state(s) { make_move(p); }

    int sidelen() const { return m_state->sidelen(); }
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset)
        switch (auto p2 = *this + os; m_state->cells(p2)) {
        case PUZ_SPACE:
        case PUZ_EMPTY:
            children.emplace_back(*this).make_move(p2);
            break;
        }
}

// 5. All the remaining Garden space where there are no Flowers must be
//    interconnected (horizontally or vertically), as he wants to be able
//    to reach every part of the Garden without treading over Flowers.
bool puz_state::is_interconnected() const
{
    int i = m_cells.find(PUZ_EMPTY);
    if (i == -1) return true;
    auto smoves = puz_move_generator<puz_state3>::gen_moves(
        {this, {i / sidelen(), i % sidelen()}});
    return boost::count_if(smoves, [&](const Position& p) {
        return cells(p) == PUZ_EMPTY;
    }) == boost::count(m_cells, PUZ_EMPTY);
}

bool puz_state::check_balance()
{
    bool balanced = true;
    int sz = m_balanced_ids.size();
    boost::remove_erase_if(m_balanced_ids, [&](int id) {
        // No need to continue if unbalanced
        if (!balanced) return false;
        auto& rng = m_game->m_balanced_ranges[id];
        int n_flower = 0, n_space = 0;
        for (auto& p : rng)
            switch (cells(p)) {
            case PUZ_FLOWER: n_flower++; break;
            case PUZ_SPACE: n_space++; break;
            }
        // already balanced
        if (n_flower > 0) return true;
        // impossible to be balanced
        if (n_flower == 0 && n_space == 0)
            balanced = false;
        return false;
    });
    m_distance += sz - m_balanced_ids.size();
    return balanced;
}

void puz_state::make_move2(int fb_id, int move_id)
{
    auto& [flowers, empties] = m_game->m_fb_info[fb_id].m_moves[move_id];
    for (auto& p : flowers)
        cells(p) = PUZ_FLOWER;
    for (auto& p : empties)
        cells(p) = PUZ_EMPTY;
    ++m_distance, m_matches.erase(fb_id);
}

bool puz_state::make_move(int fb_id, int move_id)
{
    m_distance = 0;
    make_move2(fb_id, move_id);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [fb_id, move_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int move_id : move_ids)
        if (!children.emplace_back(*this).make_move(fb_id, move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horizontal lines
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Gardener()
{
    using namespace puzzles::Gardener;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Gardener.xml", "Puzzles/Gardener.txt", solution_format::GOAL_STATE_ONLY);
}
