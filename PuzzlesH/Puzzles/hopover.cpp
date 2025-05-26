#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    Instructions:
    A frog can either move forward or jump a frog in front of it.
    The object is to exchange the frogs.
    http://www.freeonlinepcgames.net/play/hop-over-puzzle-classic-puzzle-can-you-switch-t/flash-game/
*/

namespace puzzles::hopover{

constexpr auto PUZ_WALL = '#';
constexpr auto PUZ_RED = '>';
constexpr auto PUZ_BLUE = '<';
constexpr auto PUZ_SPACE = ' ';

struct puz_game
{
    string m_id;
    string m_start, m_goal;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_start(strs[0])
    , m_goal(strs[1])
{
}

struct puz_step : pair<int, int> {
    puz_step(int n1, int n2) : pair<int, int>(n1, n2) {}
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: {} -> {}\n", mi.first, mi.second);
    return out;
}

struct puz_state : pair<string, boost::optional<puz_step>>
{
    puz_state(const puz_game& g)
        : pair<string, boost::optional<puz_step>>{g.m_start, {}}, m_game(&g) {}
    void make_move(int i, int j) {
        ::swap(first[i], first[j]);
        second = puz_step(i, j);
    }

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        int n = 0;
        for (size_t i = 1; i < first.length() - 1; ++i)
            n += myabs(first[i] - m_game->m_goal[i]);
        return n;
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(second) out << *second;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
};

void puz_state::gen_children(list<puz_state>& children) const
{
    for (size_t i = 1; i < first.length() - 1; ++i) {
        char n = first[i];
        if (n == PUZ_RED || n == PUZ_BLUE) {
            bool can_move = false;
            // Red frogs move rightward and blue ones, leftward.
            int delta = n == PUZ_RED ? 1 : -1;
            int j = i + delta;
            char n2 = first[j];
            // A frog can move forward
            if (n2 == PUZ_SPACE)
                can_move = true;
            // or jump a frog in front of it
            else if (n2 == PUZ_RED || n2 == PUZ_BLUE) {
                j += delta;
                n2 = first[j];
                if (n2 == PUZ_SPACE)
                    can_move = true;
            }
            if (can_move) {
                children.push_back(*this);
                children.back().make_move(i, j);
            }
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (size_t j = 1; j < first.size() - 1; ++j)
        out << first[j] << " ";
    println(out);
    return out;
}

}

void solve_puz_hopover()
{
    using namespace puzzles::hopover;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, true, true, false>>(
        "Puzzles/hopover.xml", "Puzzles/hopover.txt");
}
