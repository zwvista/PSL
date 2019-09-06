#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 6/Trebuchet

    Summary
    Fire!

    Description
    1. On the board you can see some trebuchets.
    2. The number on a Trebuchet indicates the distance it shoots. Only one of
       the four directions can be marked with a target, the others should be empty.
    3. Two target cells must not be orthogonally adjacent.
    4. All of the non-targeted cells must be connected.
*/

namespace puzzles::Trebuchet{

#define PUZ_TREBUCHET       'B'
#define PUZ_TARGET          'G'
#define PUZ_SPACE           ' '
#define PUZ_EMPTY           '.'
    
const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_trebuchet
{
    int m_distance;
    vector<Position> m_rng;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, puz_trebuchet> m_pos2obj;
    map<int, vector<string>> m_num2perms;

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
            if (char ch = str[c]; ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_TREBUCHET);
                auto& o = m_pos2obj[p];
                o.m_distance = ch - '0';
                for (auto& os : offset) {
                    auto p2 = p;
                    for (int i = 0; i < o.m_distance; ++i)
                        p2 += os;
                    if (is_valid(p2))
                        o.m_rng.push_back(p2);
                }
            }
        }
    }

    for (auto&& [p, o] : m_pos2obj) {
        int n = o.m_rng.size();
        auto& perms = m_num2perms[n];
        if (!perms.empty()) continue;
        // 2. The number on a Trebuchet indicates the distance it shoots. Only one of
        // the four directions can be marked with a target, the others should be empty.
        auto perm = string(n - 1, PUZ_EMPTY) + PUZ_TARGET;
        do
            perms.push_back(perm);
        while (boost::next_permutation(perm));
    }
}

struct puz_state
{
    puz_state() {}
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
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;

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
: m_game(&g), m_cells(g.m_start)
{
    for (auto&& [p, o] : g.m_pos2obj) {
        auto& perms = g.m_num2perms.at(o.m_rng.size());
        vector<int> perm_ids(perms.size());
        boost::iota(perm_ids, 0);
        m_matches[p] = perm_ids;
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perm_ids = kv.second;
        auto& o = m_game->m_pos2obj.at(p);
        auto& perms = m_game->m_num2perms.at(o.m_rng.size());

        string chars;
        for (auto& p : o.m_rng)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
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
    puz_state2(const set<Position>& a): m_area(&a) { make_move(*a.begin()); }

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

// 4. All of the non-targeted cells must be connected.
bool puz_state::is_continuous() const
{
    set<Position> area;
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_TARGET)
                area.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(area, smoves);
    return smoves.size() == area.size();
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& o = m_game->m_pos2obj.at(p);
    auto& range = o.m_rng;
    auto& perm = m_game->m_num2perms.at(range.size())[n];

    for (int k = 0; k < perm.size(); ++k) {
        auto& p2 = range[k];
        // 3. Two target cells must not be orthogonally adjacent.
        if ((cells(p2) = perm[k]) == PUZ_TARGET &&
            boost::algorithm::any_of(offset, [&](const Position& os){
            auto p3 = p2 + os;
            return is_valid(p3) && cells(p3) == PUZ_TARGET;
        }))
            return false;
    }

    ++m_distance;
    m_matches.erase(p);
    return is_continuous();
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

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_TREBUCHET)
                out << m_game->m_pos2obj.at(p).m_distance << ' ';
            else
                out << ch << ' ';
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_Trebuchet()
{
    using namespace puzzles::Trebuchet;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Trebuchet.xml", "Puzzles/Trebuchet.txt", solution_format::GOAL_STATE_ONLY);
}
