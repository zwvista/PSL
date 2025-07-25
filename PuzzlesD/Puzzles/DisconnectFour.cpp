#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 11/Disconnect Four

    Summary
    Win by not winning!

    Description
    1. The opposite of the famous game 'Connect Four', where you must line
       up four tokens of the same colour.
    2. In this puzzle you have to ensure that there are no more than three
       tokens of the same colour lined up horizontally, vertically or
       diagonally.
*/

namespace puzzles::DisconnectFour{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_R_FIXED = 'R';
constexpr auto PUZ_R_ADDED = 'r';
constexpr auto PUZ_Y_FIXED = 'Y';
constexpr auto PUZ_Y_ADDED = 'y';
constexpr auto PUZ_BOUNDARY = '`';

const string RY = {PUZ_R_ADDED, PUZ_Y_ADDED};

bool is_token_r(char ch) { return ch == PUZ_R_FIXED || ch == PUZ_R_ADDED; }
bool is_token_y(char ch) { return ch == PUZ_Y_FIXED || ch == PUZ_Y_ADDED; }

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c)
            m_cells.push_back(str[c - 1]);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);
    void make_move2(const Position& p, char ch);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    // key: position of the token
    // value: r / y / ry
    map<Position, string> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p) == PUZ_SPACE)
                m_matches[p] = RY;
        }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, str] : m_matches) {
        boost::remove_erase_if(str, [&](char ch) {
            auto f = ch == PUZ_R_ADDED ? is_token_r : is_token_y;
            vector<int> counts;
            for (auto& os : offset) {
                int n = 0;
                for (auto p2 = p + os; f(cells(p2)); p2 += os)
                    ++n;
                counts.push_back(n);
            }
            // there are no more than three tokens of the same colour
            // lined up horizontally, vertically or    diagonally.
            for (int i = 0; i < 4; ++i)
                if (counts[i] + counts[4 + i] >= 3)
                    return true;
            return false;
        });

        if (!init)
            switch(str.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, str.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, char ch)
{
    cells(p) = ch;
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, char ch)
{
    m_distance = 0;
    make_move2(p, ch);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, str] = *boost::min_element(m_matches, [](
        const pair<const Position, string>& kv1,
        const pair<const Position, string>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : str)
        if (children.push_back(*this); !children.back().make_move(p, ch))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c});
        println(out);
    }
    return out;
}

}

void solve_puz_DisconnectFour()
{
    using namespace puzzles::DisconnectFour;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DisconnectFour.xml", "Puzzles/DisconnectFour.txt", solution_format::GOAL_STATE_ONLY);
}
