#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 5/Unreliable hints

    Summary
    Can't trust them all

    Description
    1. Shade some tiles according to the following rules:
    2. Shaded tiles must not be orthogonally connected.
    3. You can shade tiles with arrows and numbers.
    4. All tiles which are not shaded must form an orthogonally continuous area.
    5. A cell containing a number and an arrow tells you how many tiles are shaded
       in that direction.
    6. However not all tiles that are shaded tell you lies.
*/

namespace puzzles::UnreliableHints{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SHADED = 'X';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const string tool_dirs = "^>v<";

struct puz_hint
{
    int m_num, m_dir;
    vector<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_hint> m_pos2hint;
    map<pair<int, int>, vector<string>> m_info2perms;

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
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (auto s = str.substr(c * 2, 2); s != "  ") {
                auto& o = m_pos2hint[p];
                o.m_num = s[0] - '0';
                auto& os = offset[o.m_dir = tool_dirs.find(s[1])];
                for (auto p2 = p; is_valid(p2); p2 += os)
                    o.m_rng.push_back(p2);
            }
        }
    }

    for (auto&& [p, o] : m_pos2hint) {
        int sz = o.m_rng.size();
        auto& perms = m_info2perms[pair{o.m_num, sz}];
        if (!perms.empty()) continue;
        for (int i = 0; i < 3; ++i) {
            char ch = i == 0 ? PUZ_EMPTY : PUZ_SHADED;
            bool is_lie = i == 2;
            for (int j = 0; j <= sz - 1; ++j) {
                if (is_lie == (j == o.m_num)) continue;
                auto perm = ch + string(sz - 1 - j, PUZ_EMPTY) + string(j, PUZ_SHADED);
                auto begin = boost::next(perm.begin()), end = perm.end();
                do {
                    for (int k = 0; k < sz - 1; ++k)
                        if (perm[k] == PUZ_SHADED && perm[k + 1] == PUZ_SHADED)
                            goto next;
                    perms.push_back(perm);
                next:;
                } while (next_permutation(begin, end));
            }
        };
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
{
    for (auto&& [p, o] : g.m_pos2hint) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_info2perms.at(pair{o.m_num, o.m_rng.size()}).size());
        boost::iota(perm_ids, 0);
    }
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        auto& o = m_game->m_pos2hint.at(p);
        auto& rng = o.m_rng;
        int sz = rng.size();
        auto& perms = m_game->m_info2perms.at(pair{o.m_num, sz});

        string chars;
        for (auto& p : rng)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            if (!boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }))
                return true;
            // 2. Shaded tiles must not be orthogonally connected.
            for (int k = 0; k < sz; ++k) {
                auto& p = rng[k];
                if (char ch1 = perm[k]; ch1 == PUZ_SHADED)
                    for (auto& os : offset)
                        if (auto p2 = p + os; is_valid(p2) && ch1 == cells(p2))
                            return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move(p, perm_ids[0]), 1;
            }
    }
    return is_continuous() ? 2 : 0;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset)
        if (auto p2 = *this + os; m_rng->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
}

bool puz_state::is_continuous() const
{
    set<Position> a;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch != PUZ_SHADED)
                a.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& o = m_game->m_pos2hint.at(p);
    auto& range = o.m_rng;
    int sz = range.size();
    auto& perm = m_game->m_info2perms.at({o.m_num, sz})[n];

    for (int k = 0; k < sz; ++k)
        cells(range[k]) = perm[k];

    ++m_distance;
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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2hint.find(p); it == m_game->m_pos2hint.end())
                out << "  ";
            else
                out << it->second.m_num << tool_dirs[it->second.m_dir];
        }
        println(out);
    }
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_UnreliableHints()
{
    using namespace puzzles::UnreliableHints;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/UnreliableHints.xml", "Puzzles/UnreliableHints.txt", solution_format::GOAL_STATE_ONLY);
}
