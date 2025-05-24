#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::fling{

#define PUZ_BALL        '@'

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    Position m_size;
    string m_id;
    set<Position> m_balls;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size(), strs[0].length()))
{
    for (int r = 0; r < rows(); r++) {
        auto& str = strs[r];
        for (int c = 0; c < cols(); c++)
            if (str[c] == PUZ_BALL)
                m_balls.insert(Position(r, c));
    }
}

typedef pair<Position, char> puz_step;

ostream& operator<<(ostream& out, const puz_step& a)
{
    out << format("({},{}){}", a.first.first, a.first.second, a.second);
    return out;
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g)
        : m_game(&g), m_balls(g.m_balls) {}
    bool operator<(const puz_state& x) const {return m_balls < x.m_balls;}
    bool is_ball(const Position& p) const {return m_balls.contains(p);}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    bool make_move(const Position& p, int i);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return (int)m_balls.size() - 1;}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move << endl;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    set<Position> m_balls;
    boost::optional<puz_step> m_move;
};

bool puz_state::make_move(const Position& p, int i)
{
    static string_view dirs = "lrud";
    Position os = offset[i];
    vector<Position> moved_balls;

    Position p2 = p;
    for (;;) {
        m_balls.erase(p2);
        bool bvalid, bball;
        Position p3 = p2 + os;
        for (; ; p3 += os) {
            bvalid = is_valid(p3);
            bball = is_ball(p3);
            if (!bvalid || bball) break;
        }
        if (p2 == p && (!bvalid || p3 == p2 + os))
            return false;
        if (bball) {
            moved_balls.push_back(p3 - os);
            p2 = p3;
        } else
            break;
    }

    m_move = puz_step(p, dirs[i]);
    m_balls.insert(moved_balls.begin(), moved_balls.end());
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (const Position& p : m_balls)
        for (int i = 0; i < 4; i++) {
            children.push_back(*this);
            if (!children.back().make_move(p, i))
                children.pop_back();
        }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position pos(r, c);
            out << (is_ball(pos) ? PUZ_BALL : ' ');
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_fling()
{
    using namespace puzzles::fling;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/fling3.xml", "Puzzles/fling.txt", solution_format::MOVES_ONLY);
}
