#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 7/Caffelatte

    Summary
    Cows and Coffee

    Description
    1. Make Cappuccino by linking each cup to one or more coffee beans and cows.
    2. Links must be straight lines, not crossing each other.
    3. To each cup there must be linked an equal number of beans and cows. At
       least one of each.
    4. When linking multiple beans and cows, you can also link cows to cows and
       beans to beans, other than linking them to the cup.
*/

namespace puzzles::Caffelatte{

#define PUZ_SPACE             ' '
#define PUZ_COFFEE            'F'
#define PUZ_COW               'W'
#define PUZ_BEAN              'B'
#define PUZ_HORZ              '-'
#define PUZ_VERT              '|'
#define PUZ_BOUNDARY          '+'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
    
using puz_link = pair<int, Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, vector<puz_link>> m_pos2links;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; ; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            char ch = str[c - 1];
            m_start.push_back(ch);
            Position p(r * 2 - 1, c * 2 - 1);
            if (ch != PUZ_SPACE)
                m_pos2links[p];
            if (c == m_sidelen / 2) break;
            m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_start.push_back(PUZ_BOUNDARY);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto&& [p, links] : m_pos2links) {
        char ch = cells(p);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            for (auto p2 = p + os; ; p2 += os)
                if (char ch2 = cells(p2); ch2 != PUZ_SPACE) {
                    if (ch == PUZ_COFFEE && (ch2 == PUZ_COW || ch2 == PUZ_BEAN) ||
                        ch2 == PUZ_COFFEE && (ch == PUZ_COW || ch == PUZ_BEAN) || ch == ch2)
                        links.emplace_back(i, p2);
                    break;
                }
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
    bool make_move(int n);
    void make_move2(int n);
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
    // key: the position of the island
    // value.elem: the id of link that connects the object at this position
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        const auto& p = kv.first;
        auto& perms = kv.second;

        boost::remove_erase_if(perms, [&](int id) {
            return true;
        });

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(perms.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{

    m_distance += 3;
}

bool puz_state::make_move(int n)
{
    m_distance = 0;
    make_move2(n);
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
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c});
        out << endl;
    }
    return out;
}

}

void solve_puz_Caffelatte()
{
    using namespace puzzles::Caffelatte;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Caffelatte.xml", "Puzzles/Caffelatte.txt", solution_format::GOAL_STATE_ONLY);
}
