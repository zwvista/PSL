#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 5/Ripple Effect

    Summary
    Fill the Room with the numbers, but take effect of the Ripple Effect

    Description
    1. The goal is to fill the Rooms you see on the board, with numbers 1 to room size.
    2. While doing this, you must consider the Ripple Effect. The same number
       can only appear on the same row or column at the distance of the number
       itself.
    3. For example a 2 must be separated by another 2 on the same row or
       column by at least two tiles.
*/

namespace puzzles::RippleEffect{

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

// first: the remaining positions in the room that should be filled
// second: all permutations of the remaining numbers that should be used to fill the room
using puz_room_info = pair<vector<Position>, vector<vector<int>>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_start;
    map<Position, pair<int, int>> m_pos2info;
    map<int, puz_room_info> m_room2info;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horz-walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vert-walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            if (ch != ' ')
                m_start[{r, c}] = ch - '0';
            rng.insert(p);
        }
    }

    vector<vector<Position>> rooms;
    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()}, smoves);
        rooms.resize(n + 1);
        for (auto& p : smoves) {
            m_pos2info[p].first = n;
            rooms[n].push_back(p);
            rng.erase(p);
        }
    }

    for (int i = 0; i < rooms.size(); ++i) {
        const auto& room = rooms[i];
        auto& info = m_room2info[i];
        auto& ps = info.first;
        auto& perms = info.second;

        vector<int> perm;
        perm.resize(room.size());
        boost::iota(perm, 1);

        vector<Position> filled;
        for (const auto& p : room) {
            auto it = m_start.find(p);
            if (it != m_start.end()) {
                filled.push_back(p);
                boost::range::remove_erase(perm, it->second);
            }
        }
        boost::set_difference(room, filled, back_inserter(ps));
        for (int j = 0; j < ps.size(); ++j)
            m_pos2info[ps[j]].second = j;

        if (info.first.empty())
            m_room2info.erase(i);
        else
            do
                perms.push_back(perm);
            while (boost::next_permutation(perm));
    }
}

struct puz_state : vector<int>
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    int cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    int& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i, const vector<int>& nums);
    void apply_ripple_effect(const Position& p, int n);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, 0);}
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<int, puz_room_info> m_room2info;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: vector<int>(g.m_sidelen * g.m_sidelen)
, m_game(&g), m_room2info(g.m_room2info)
{
    for (auto& [p, n] : g.m_start)
        cells(p) = n;

    for (auto& [p, n] : g.m_start)
        apply_ripple_effect(p, n);
}

void puz_state::apply_ripple_effect(const Position& p, int n)
{
    for (auto& os : offset) {
        auto p2 = p;
        for (int k = 0; k < n; ++k) {
            p2 += os;
            if (!is_valid(p2)) break;
            if (cells(p2) != 0) continue;
            int i, j;
            tie(i, j) = m_game->m_pos2info.at(p2);
            boost::range::remove_erase_if(m_room2info.at(i).second, [=](const vector<int>& nums) {
                return nums[j] == n;
            });
        }
    }
}

bool puz_state::make_move(int i, const vector<int>& nums)
{
    const auto& info = m_room2info.at(i);
    m_distance = nums.size();
    for (int j = 0; j < m_distance; ++j) {
        const auto& p = info.first[j];
        int n = nums[j];
        cells(p) = n;
        apply_ripple_effect(p, n);
    }
    m_room2info.erase(i);
    return boost::algorithm::none_of(m_room2info, [](const pair<const int, puz_room_info>& kv) {
        return kv.second.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [i, info] = *boost::min_element(m_room2info, [](
        const pair<const int, puz_room_info>& kv1,
        const pair<const int, puz_room_info>& kv2) {
        return kv1.second.second.size() < kv2.second.second.size();
    });
    for (auto& nums : info.second) {
        children.push_back(*this);
        if (!children.back().make_move(i, nums))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0;; ++r) {
        // draw horz-walls
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vert-walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_RippleEffect()
{
    using namespace puzzles::RippleEffect;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/RippleEffect.xml", "Puzzles/RippleEffect.txt", solution_format::GOAL_STATE_ONLY);
}
