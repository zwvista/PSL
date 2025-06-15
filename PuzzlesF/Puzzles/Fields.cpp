#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 1/Fields

    Summary
    Twice of the blessings of a Nurikabe

    Description
    1. Fill the board with either meadows or soil, creating a path of soil
       and a path of meadows, with the same rules for each of them.
    2. The path is continuous and connected horizontally or vertically, but
       cannot touch diagonally.
    3. The path can't form 2x2 squares.
    4. These type of paths are called Nurikabe.
*/

namespace puzzles::Fields{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SOIL = 'S';
constexpr auto PUZ_MEADOW = 'M';
constexpr auto PUZ_BOUNDARY = '"';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
constexpr Position offset2[] = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // all permutations
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            m_start.push_back(ch);
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    
    for (int i = 1; i <= 3; ++i) {
        auto perm = string(i, PUZ_MEADOW) + string(4 - i, PUZ_SOIL);
        do
            m_perms.push_back(perm);
        while (boost::next_permutation(perm));
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
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    int adjust_area();

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
: m_game(&g), m_cells(g.m_start)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int r = 1; r < sidelen() - 2; ++r)
        for (int c = 1; c < sidelen() - 2; ++c)
            m_matches[{r, c}] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& os : offset2)
            chars.push_back(cells(p + os));

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
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return adjust_area();
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng, const Position& starting) : m_rng(&rng) { make_move(starting); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (m_rng->contains(p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

int puz_state::adjust_area()
{
    map<Position, vector<char>> pos2kinds;
    for (char chPath : {PUZ_SOIL, PUZ_MEADOW}) {
        // The path is continuous and connected horizontally or vertically.
        set<Position> a;
        Position starting;
        int n1 = 0;
        for (int r = 1; r < sidelen() - 1; ++r)
            for (int c = 1; c < sidelen() - 1; ++c) {
                Position p(r, c);
                if (char ch = cells(p); ch == PUZ_SPACE || ch == chPath) {
                    a.insert(p);
                    if (ch == chPath && n1++ == 0)
                        starting = p;
                }
            }
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({a, starting}, smoves);
        
        int n2 = 0;
        for (auto& p : smoves)
            if (char ch = cells(p); ch == chPath)
                ++n2;
            else
                pos2kinds[p].push_back(chPath);
        
        if (n1 != n2)
            return 0;
    }
    int n = 2;
    for (auto& [p, kinds] : pos2kinds)
        if (kinds.size() == 1)
            cells(p) = kinds[0], n = 1;
    return n;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_perms[n];

    for (int k = 0; k < 4; ++k)
        cells(p + offset2[k]) = perm[k];

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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_Fields()
{
    using namespace puzzles::Fields;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Fields.xml", "Puzzles/Fields.txt", solution_format::GOAL_STATE_ONLY);
}
