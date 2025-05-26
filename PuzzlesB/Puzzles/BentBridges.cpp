#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
#include "BentBridges.h"

namespace puzzles::BentBridges{

struct puz_bridge {
    vector<pair<Position, char>> m_rng;
    Position m_p1, m_p2;
    bool m_is_bent = false;
    Position m_p_bent;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;
    vector<puz_bridge> m_bridges;
    map<Position, vector<int>> m_pos2indexes;
    int m_bridge_count = 0;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
{
    int sz = strs.size();
    m_sidelen = sz * 2 - 1;
    for (int r = 0; r < sz; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < sz; ++c) {
            char ch = str[c];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_ISLAND);
                int n = ch - '0';
                m_bridge_count += n;
                m_pos2num[{r * 2, c * 2}] = n;
            }
            if (c < sz - 1)
                m_start.push_back(PUZ_SPACE);
        }
        if (r < sz - 1)
            m_start.append(m_sidelen, PUZ_SPACE);
    }

    for (auto&& [p1, n1] : m_pos2num)
        for (auto&& [p2, n2] : m_pos2num) {
            if (p1 >= p2) continue;
            puz_bridge b;

            auto f = [&, p1 = p1, p2 = p2](const Position& p3, const Position& p4, const Position& os, char ch_hv) {
                if (p1 == p3)
                    b.m_rng.clear();
                for (auto p = p3; p != p4;) {
                    p += os; b.m_rng.emplace_back(p, ch_hv);
                    char ch = cells(p += os);
                    if (ch == PUZ_SPACE)
                        b.m_rng.emplace_back(p, PUZ_BRIDGE);
                    else if (p != p4 || p2 != p4)
                        return false;
                }
                if (p2 == p4) {
                    int sz = m_bridges.size();
                    m_pos2indexes[b.m_p1 = p1].push_back(sz);
                    m_pos2indexes[b.m_p2 = p2].push_back(sz);
                    b.m_is_bent = p1 != p3;
                    b.m_p_bent = b.m_is_bent ? p3 : Position(-1, -1);
                    m_bridges.push_back(b);
                }
                return true;
            };

            if (p1.first == p2.first)
                f(p1, p2, offset[1], PUZ_HORZ);
            else if (p1.second == p2.second)
                f(p1, p2, offset[2], PUZ_VERT);
            else {
                Position p3(p1.first, p2.second), p4(p2.first, p1.second);
                int i = p1.second < p2.second ? 1 : 3;
                f(p1, p3, offset[i], PUZ_HORZ) && f(p3, p2, offset[2], PUZ_VERT);
                f(p1, p4, offset[2], PUZ_VERT) && f(p4, p2, offset[i], PUZ_HORZ);
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
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_connected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_game->m_bridge_count - boost::accumulate(m_moves, 0, [](int acc, const pair<const Position, vector<int>>& kv) {
            return acc + kv.second.size();
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    map<Position, vector<int>> m_moves;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start), m_matches(g.m_pos2indexes)
{
    for (auto&& [k, v] : g.m_pos2indexes)
        m_moves[k];
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    int n = 2;
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& bridge_ids = kv.second;

        auto f = [&](int id) {
            auto& b = m_game->m_bridges[id];
            boost::remove_erase(m_matches[b.m_p1], id);
            boost::remove_erase(m_matches[b.m_p2], id);
            n = 1;
        };

        for (int id : bridge_ids)
            if (boost::algorithm::any_of(m_game->m_bridges[id].m_rng, [&](const pair<Position, char>& kv) {
                return cells(kv.first) != PUZ_SPACE;
            }))
                f(id);

        if (!init) {
            int sz = bridge_ids.size(), sz2 = m_moves[p].size(), sz3 = m_game->m_pos2num.at(p);
            if (sz2 == sz3)
                while (!bridge_ids.empty())
                    f(bridge_ids[0]);
            else if (sz + sz2 < sz3)
                return 0;
        }
    }
    return n;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s) : m_state(&s) {
        make_move(s.m_game->m_pos2num.begin()->first);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto f = [&](const vector<int>& indexes) {
        for (int i : indexes) {
            children.push_back(*this);
            children.back().make_move(m_state->m_game->m_bridges[i].m_p2);
        }
    };
    f(m_state->m_matches.at(*this));
    f(m_state->m_moves.at(*this));
}

bool puz_state::is_connected() const
{
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    return smoves.size() == m_game->m_pos2indexes.size();
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& b = m_game->m_bridges[n];
    for (auto&& [p2, ch] : b.m_rng)
        cells(p2) = ch;

    boost::remove_erase(m_matches[b.m_p1], n);
    m_moves[b.m_p1].push_back(n);
    boost::remove_erase(m_matches[b.m_p2], n);
    m_moves[b.m_p2].push_back(n);
    m_distance += 2;

    return is_connected();
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
    auto f = [](const pair<const Position, vector<int>>& kv) {
        int n = kv.second.size();
        return n == 0 ? 100 : n;
    };
    auto& kv = *boost::min_element(m_matches, [&](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return f(kv1) < f(kv2);
    });

    for (int n : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_ISLAND)
                out << m_game->m_pos2num.at(p);
            else
                out << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BentBridges()
{
    using namespace puzzles::BentBridges;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BentBridges.xml", "Puzzles/BentBridges.txt", solution_format::GOAL_STATE_ONLY);
}
