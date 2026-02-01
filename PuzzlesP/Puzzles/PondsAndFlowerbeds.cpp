#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 3/Ponds and Flowerbeds

    Summary
    Mad Gardener!

    Description
    1. The aim is to locate some Flowerbeds and Ponds in the field.
    2. A Flowerbed is an area of 3 cells, containing one flower.
    3. A Pond is an area of any size without flower.
    4. Each 2x2 area must contain at least a Hedge or a Pond.
    5. Hedges when presents, are given in light green.
*/

namespace puzzles::PondsAndFlowerbeds{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_FLOWER = 'F';
constexpr auto PUZ_POND = '=';
constexpr auto PUZ_HEDGE = '.';

constexpr array<Position, 4> offset = Position::Directions4;

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

using puz_piece = tuple<Position, int, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    set<Position> m_flowers;
    // elem.first: top-left position of the flowerbed
    // elem.second: index of the flowerbed
    vector<pair<Position, int>> m_flowerbeds;
    map<Position, vector<int>> m_pos2flowerbed_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c)
            if (char ch = str[c]; ch != ' ')
                m_flowers.emplace(r, c);
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            for (int i = 0; i < flowerbeds_offset.size(); ++i) {
                auto& fb_offset = flowerbeds_offset[i];
                if (vector<Position> rng; [&] {
                    for (auto& os : fb_offset) {
                        auto p2 = p + os;
                        if (!is_valid(p2))
                            return false;
                        if (m_flowers.contains(p2))
                            rng.push_back(p2);
                    }
                    return rng.size() == 1;
                }()) {
                    int n = m_flowerbeds.size();
                    m_flowerbeds.emplace_back(p, i);
                    m_pos2flowerbed_ids[rng[0]].push_back(n);
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

    const puz_game* m_game;
    string m_cells;
    // key: the position of the number
    // value.elem: the index of the flowerbed
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
    char m_ch = 'A';
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_matches(g.m_pos2flowerbed_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, piece_ids] : m_matches) {
//        auto& [num, pieces] = m_game->m_pos2info.at(p);
//
        boost::remove_erase_if(piece_ids, [&](int id) {
//            auto& [p2, i, j] = pieces[id];
//            auto& [flowerbed, shrubs, num2perms] = m_game->m_flowerbed_shrubs[i];
//            auto& perm = num2perms.at(num)[j];
//
//            if (boost::algorithm::any_of(flowerbed, [&](const Position& os) {
//                char ch = cells(p2 + os);
//                return ch != PUZ_SPACE && ch != PUZ_POND;
//            }))
//                return true;
//
//            for (int i = 0; i < shrubs.size(); ++i) {
//                auto p3 = p2 + shrubs[i];
//                if (!is_valid(p3)) continue;
//                char ch = cells(p3), ch2 = perm[i];
//                if (ch2 == PUZ_POND && ch == PUZ_HEDGE ||
//                    ch2 == PUZ_HEDGE && ch != PUZ_SPACE && ch != ch2)
//                    return true;
//            }
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
//    auto& [num, pieces] = m_game->m_pos2info.at(p);
//    auto& [p2, i, j] = pieces[n];
//    auto& [flowerbed, shrubs, num2perms] = m_game->m_flowerbed_shrubs[i];
//    auto& perm = num2perms.at(num)[j];
//    for (auto& os : flowerbed)
//        cells(p2 + os) = m_ch;
//    for (int i = 0; i < shrubs.size(); ++i) {
//        auto p3 = p2 + shrubs[i];
//        if (!is_valid(p3)) continue;
//        char& ch = cells(p3), ch2 = perm[i];
//        if (ch == PUZ_SPACE && (ch = ch2) == PUZ_HEDGE)
//            for (auto& os : offset) {
//                auto p4 = p3 + os;
//                if (is_valid(p4) && cells(p4) == PUZ_SPACE)
//                    cells(p4) = PUZ_POND;
//            }
//    }

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
    for (int n : piece_ids)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](const Position& p1, const Position& p2) {
        return !is_valid(p1) || !is_valid(p2) || cells(p1) != cells(p2);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (f({r, c}, {r - 1, c}) ? " --" : "   ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (f(p, {r, c - 1}) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p) << (m_game->m_flowers.contains(p) ? PUZ_FLOWER : ' ');
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PondsAndFlowerbeds()
{
    using namespace puzzles::PondsAndFlowerbeds;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PondsAndFlowerbeds.xml", "Puzzles/PondsAndFlowerbeds.txt", solution_format::GOAL_STATE_ONLY);
}
