#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 2/Flowerbed Shrubs

    Summary
    A lively garden

    Description
    1. Divide the board in Flowerbeds of exactly three tiles. Each Flowerbed
       contains a number.
    2. Single tiles left outside Flowerbeds are Shrubs. Shrubs cannot touch
       each other orthogonally.
    3. The number on each Flowerbed tells you how many Shrubs are adjacent to it.
*/

namespace puzzles::FlowerbedShrubs{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SHRUB = '=';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

const vector<vector<Position>> flowerbeds_offset = {
    // L
    {{0, 0}, {0, 1}, {1, 0}},
    {{0, 0}, {0, 1}, {1, 1}},
    {{0, 0}, {1, 0}, {1, 1}},
    {{0, 1}, {1, 0}, {1, 1}},
    // I
    {{0, 0}, {1, 0}, {2, 0}},
    {{0, 0}, {0, 1}, {0, 2}},
};

struct puz_flowerbed_shrub
{
    vector<Position> m_flowerbed;
    vector<Position> m_shrubs;
    map<int, vector<string>> m_num2perms;
};

using puz_piece = tuple<Position, int, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_flowerbed_shrub> m_flowerbed_shrubs;
    map<Position, pair<int, vector<puz_piece>>> m_pos2info;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (auto& v : flowerbeds_offset) {
        auto& o = m_flowerbed_shrubs.emplace_back();
        o.m_flowerbed = v;
        auto& v2 = o.m_shrubs;
        for (auto& p : v)
            for (auto& os : offset) {
                auto p2 = p + os;
                if (boost::algorithm::none_of_equal(v, p2) &&
                    boost::algorithm::none_of_equal(v2, p2))
                    v2.push_back(p2);
            }
    }

    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_pos2info[{r, c}].first = ch - '0';
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < m_flowerbed_shrubs.size(); ++i) {
                auto& [flowerbed, shrubs, num2perms] = m_flowerbed_shrubs[i];
                vector<Position> rng;
                for (auto& os : flowerbed) {
                    auto p2 = p + os;
                    if (!is_valid(p2)) {
                        rng.clear();
                        break;
                    }
                    if (m_pos2info.contains(p2))
                        rng.push_back(p2);
                }
                if (rng.size() != 1) continue;
                auto& [num, v] = m_pos2info.at(rng[0]);
                auto& perms = num2perms[num];
                int sz = shrubs.size();
                if (perms.empty()) {
                    auto perm = string(sz - num, PUZ_EMPTY) + string(num, PUZ_SHRUB);
                    do {
                        if ([&] {
                            vector<Position> rng2;
                            for (int i = 0; i < sz; ++i)
                                if (perm[i] == PUZ_SHRUB)
                                    rng2.push_back(shrubs[i]);
                            for (auto& p1 : rng2)
                                for (auto& p2 : rng2)
                                    if (boost::algorithm::any_of_equal(offset, p1 - p2))
                                        return false;
                            return true;
                        }())
                            perms.emplace_back(perm);
                    } while (boost::next_permutation(perm));
                }
                for (int j = 0; j < perms.size(); ++j) {
                    auto& perm = perms[j];
                    if ([&] {
                        vector<Position> rng2;
                        for (int i = 0; i < sz; ++i)
                            if (perm[i] == PUZ_SHRUB)
                                rng2.push_back(p + shrubs[i]);
                        return boost::algorithm::all_of(rng2, [&](const Position& p2) {
                            return is_valid(p2) && !m_pos2info.contains(p2);
                        });
                    }())
                        v.emplace_back(p, i, j);
                }
            }
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
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    string m_cells;
    unsigned int m_distance = 0;
    char m_ch = 'A';
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (auto& [p, info] : g.m_pos2info) {
        auto& [num, pieces] = info;
        for (int i = 0; i < pieces.size(); ++i)
            m_matches[p].push_back(i);
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, piece_ids] : m_matches) {
        auto& [num, pieces] = m_game->m_pos2info.at(p);

        boost::remove_erase_if(piece_ids, [&](int id) {
            auto& [p2, i, j] = pieces[id];
            auto& [flowerbed, shrubs, num2perms] = m_game->m_flowerbed_shrubs[i];
            auto& perm = num2perms.at(num)[j];

            if (boost::algorithm::any_of(flowerbed, [&](const Position& os) {
                char ch = cells(p2 + os);
                return ch != PUZ_SPACE && ch != PUZ_EMPTY;
            }))
                return true;

            for (int i = 0; i < shrubs.size(); ++i) {
                auto p3 = p2 + shrubs[i];
                if (!is_valid(p3)) continue;
                char ch = cells(p3), ch2 = perm[i];
                if (ch2 == PUZ_EMPTY && ch == PUZ_SHRUB ||
                    ch2 == PUZ_SHRUB && ch != PUZ_SPACE && ch != ch2)
                    return true;
            }
            return false;
        });

        if (!init)
            switch (piece_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, piece_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& [num, pieces] = m_game->m_pos2info.at(p);
    auto& [p2, i, j] = pieces[n];
    auto& [flowerbed, shrubs, num2perms] = m_game->m_flowerbed_shrubs[i];
    auto& perm = num2perms.at(num)[j];
    for (auto& os : flowerbed)
        cells(p2 + os) = m_ch;
    for (int i = 0; i < shrubs.size(); ++i) {
        auto p3 = p2 + shrubs[i];
        if (!is_valid(p3)) continue;
        char& ch = cells(p3), ch2 = perm[i];
        if (ch == PUZ_SPACE && (ch = ch2) == PUZ_SHRUB)
            for (auto& os : offset) {
                auto p4 = p3 + os;
                if (is_valid(p4) && cells(p4) == PUZ_SPACE)
                    cells(p4) = PUZ_EMPTY;
            }
    }

    ++m_ch, ++m_distance;
    m_matches.erase(p);
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
    auto& [p, piece_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : piece_ids) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    set<Position> horz_walls, vert_walls;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            for (int i = 0; i < 4; ++i) {
                auto p2 = p + offset[i];
                auto p_wall = p + offset2[i];
                auto& walls = i % 2 == 0 ? horz_walls : vert_walls;
                if (!is_valid(p2) || cells(p) != cells(p2))
                    walls.insert(p_wall);
            }
        }

    for (int r = 0;; ++r) {
        // draw horizontal walls
        for (int c = 0; c < sidelen(); ++c)
            out << (horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            char ch = cells(p);
            out << ch;
            if (auto it = m_game->m_pos2info.find(p); it != m_game->m_pos2info.end())
                out << it->second.first;
            else
                out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_FlowerbedShrubs()
{
    using namespace puzzles::FlowerbedShrubs;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FlowerbedShrubs.xml", "Puzzles/FlowerbedShrubs.txt", solution_format::GOAL_STATE_ONLY);
}
