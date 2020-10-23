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
                m_pos2num[{r * 2, c * 2}] = ch - '0';
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
    puz_state() {}
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
: m_game(&g), m_cells(g.m_start), m_matches(g.m_pos2indexes)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& b = m_game->m_bridges[id];
            return boost::algorithm::any_of(b.m_rng, [&](const pair<Position, char>& kv) {
                return cells(kv.first) != PUZ_SPACE;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s) : m_state(&s) {
        make_move(s.m_matches.begin()->first);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& b = m_state->m_game->m_bridges;
}

bool puz_state::is_connected() const
{
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    return smoves.size() == boost::count(m_cells, PUZ_ISLAND);
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& rng = m_game->m_bridges[n].m_rng;
    for (auto&& [p2, ch] : rng)
        cells(p2) = ch;

    ++m_distance;
    m_matches.erase(p);
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

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_ISLAND)
                out << format("%2d") % m_game->m_pos2num.at(p);
            else
                out << ' ' << ch;
        }
        out << endl;
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
