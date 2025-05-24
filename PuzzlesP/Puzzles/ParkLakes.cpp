#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
/*
    iOS Game: Logic Games/Puzzle Set 15/Park Lakes

    Summary
    Find the Lakes

    Description
    1. The board represents a park, where there are some hidden lakes, all square
       in shape.
    2. You have to find the lakes with the aid of hints, knowing that:
    3. A number tells you the total size of the any lakes orthogonally touching it,
       while a question mark tells you that there is at least one lake orthogonally
       touching it.
    4. Lakes aren't on tiles with numbers or question marks.
    5. All the land tiles are connected horizontally or vertically.
*/

namespace puzzles::ParkLakes{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_NUM          'N'
#define PUZ_WATER        'L'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_hint_perm
{
    set<Position> m_water;
    set<Position> m_empty;
};
struct puz_hint_info
{
    int m_sum;
    vector<puz_hint_perm> m_hint_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_hint_info> m_pos2hintinfo;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
{
    map<int, vector<vector<int>>> num2perms;
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            auto s = str.substr(c * 2, 2);
            if (s == "  ")
                m_start.push_back(PUZ_SPACE);
            else {
                m_start.push_back(PUZ_NUM);
                num2perms[m_pos2hintinfo[{r, c}].m_sum = s == " ?" ? -1 : stoi(s)];
            }
        }
    }
    for (auto& kv : num2perms) {
        int num = kv.first;
        auto& perms = kv.second;
        for (int n0 = 0; n0 < m_sidelen; ++n0)
            for (int n1 = 0; n1 < m_sidelen; ++n1)
                for (int n2 = 0; n2 < m_sidelen; ++n2)
                    for (int n3 = 0; n3 < m_sidelen; ++n3) {
                        int sum = n0 * n0 + n1 * n1 + n2 * n2 + n3 * n3;
                        if (num == -1 && sum != 0 || num == sum)
                            perms.push_back({n0, n1, n2, n3});
                    }
    }
    for (auto& kv : m_pos2hintinfo) {
        auto& p0 = kv.first;
        int r0 = p0.first, c0 = p0.second;
        auto& info = kv.second;
        auto& perms = num2perms[info.m_sum];
        for (auto& perm : perms) {
            vector<int> indexes(4);
            for (int i = 0; i < 4;) {
                puz_hint_perm hp;
                auto f = [&](const Position& p1) {
                    if (is_valid(p1) && cells(p1) == PUZ_SPACE)
                        hp.m_empty.insert(p1);
                };
                for (int j = 0; j < 4; ++j) {
                    int n = perm[j];
                    if (n == 0)
                        f(p0 + offset[j]);
                    else {
                        int k = indexes[j];
                        int r1 = j == 0 ? r0 - n : j == 2 ? r0 + 1 : r0 - n + 1 + k;
                        int c1 = j == 3 ? c0 - n : j == 1 ? c0 + 1 : c0 - n + 1 + k;
                        int r2 = r1 + n - 1;
                        int c2 = c1 + n - 1;
                        if (!is_valid({r1, c1}) || !is_valid({r2, c2}))
                            goto next_perm;
                        for (int r = r1; r <= r2; ++r)
                            for (int c = c1; c <= c2; ++c) {
                                Position p1(r, c);
                                if (cells(p1) == PUZ_SPACE && !hp.m_empty.contains(p1))
                                    hp.m_water.insert(p1);
                                else
                                    goto next_perm;
                            }
                        for (int r = r1; r <= r2; ++r)
                            f({r, c1 - 1}), f({r, c2 + 1});
                        for (int c = c1; c <= c2; ++c)
                            f({r1 - 1, c1}), f({r2 + 1, c});
                    }
                }
                info.m_hint_perms.push_back(hp);
            next_perm:
                for (i = 0; i < 4 && ++indexes[i] == std::max(1, perm[i]); ++i)
                    indexes[i] = 0;
            }
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;
    int get_hint(const Position& p) const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g) ,m_cells(g.m_start)
{
    for (auto& kv : g.m_pos2hintinfo) {
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(kv.second.m_hint_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perm_ids = kv.second;

        auto& hint_perms = m_game->m_pos2hintinfo.at(p).m_hint_perms;
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& hp = hint_perms[id];
            return boost::algorithm::any_of(hp.m_water, [&](const Position& p2) {
                return cells(p2) == PUZ_EMPTY;
            }) || boost::algorithm::any_of(hp.m_empty, [&](const Position& p2) {
                return cells(p2) == PUZ_WATER;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a, const Position& p_start)
        : m_area(&a) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->count(p) != 0) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// 5. All the land tiles are connected horizontally or vertically.
bool puz_state::is_continuous() const
{
    set<Position> rng1;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_WATER)
                rng1.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves({rng1, m_game->m_pos2hintinfo.begin()->first}, smoves);
    set<Position> rng2(smoves.begin(), smoves.end()), rng3;
    boost::set_difference(rng1, rng2, inserter(rng3, rng3.end()));
    return boost::algorithm::all_of(rng3, [this](const Position& p) {
        return cells(p) != PUZ_NUM;
    });
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& hp = m_game->m_pos2hintinfo.at(p).m_hint_perms[n];
    for (auto& p2 : hp.m_water)
        cells(p2) = PUZ_WATER;
    for (auto& p2 : hp.m_empty)
        cells(p2) = PUZ_EMPTY;
    if (!is_continuous())
        return false;

    ++m_distance;
    m_matches.erase(p);
    return true;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

int puz_state::get_hint(const Position& p) const
{
    int sum = 0;
    for (int i = 0; i < 4; ++i) {
        auto p2 = p + offset[i];
        if (!is_valid(p2) || cells(p2) != PUZ_WATER) continue;
        int n = 1;
        auto os = offset[i % 2 == 0 ? 1 : 2];
        for (auto p3 = p2 - os; is_valid(p3) && cells(p3) == PUZ_WATER; p3 -= os, ++n);
        for (auto p3 = p2 + os; is_valid(p3) && cells(p3) == PUZ_WATER; p3 += os, ++n);
        sum += n * n;
    }
    return sum;
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_NUM) {
                int n = m_game->m_pos2hintinfo.at(p).m_sum;
                if (n == -1)
                    n = get_hint(p);
                out << format("{:2}", n);
            } else
                out << ' ' << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_ParkLakes()
{
    using namespace puzzles::ParkLakes;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/ParkLakes.xml", "Puzzles/ParkLakes.txt", solution_format::GOAL_STATE_ONLY);
}
