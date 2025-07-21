#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Puzzle Retreat
*/

namespace puzzles::PuzzleRetreat{

constexpr auto PUZ_HOLE_EMPTY = ' ';
constexpr auto PUZ_BLOCK_STOP = 'S';
constexpr auto PUZ_BLOCK_FIRE = 'F';
constexpr auto PUZ_BLOCK_FIXED = '#';
constexpr auto PUZ_HOLE_BONSAI = 'b';

constexpr auto PUZ_BLOCK_ICE = 'O';
constexpr auto PUZ_HOLE_ICE = 'o';
constexpr auto PUZ_HOLE_STOP = 's';
constexpr auto PUZ_HOLE_FIRE = 'f';
constexpr auto PUZ_HOLE_TREE = 't';
constexpr auto PUZ_BLOCK_ARROW = 'A';
constexpr auto PUZ_BLOCK_USED = '*';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};
constexpr string_view arrows = "^>v<";
constexpr string_view dirs = "urdl";

enum class puz_block_type
{
    ICE,
    STOP,
    FIRE,
};
struct puz_block
{
    puz_block_type m_type;
    int m_num;
    puz_block() {}
    puz_block(puz_block_type t, int n)
        : m_type(t), m_num(n) {}
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    map<Position, puz_block> m_pos2block;
    map<Position, int> m_pos2dir;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_cells.append(cols(), PUZ_BLOCK_FIXED);
    for (int r = 1; r < rows() - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK_FIXED);
        for (int c = 1; c < cols() - 1; ++c)
            switch(Position p(r, c); char ch = str[c - 1]) {
            case PUZ_HOLE_EMPTY:
            case PUZ_HOLE_BONSAI:
            case PUZ_BLOCK_FIXED:
                m_cells.push_back(ch);
                break;
            case PUZ_BLOCK_STOP:
                m_cells.push_back(ch);
                m_pos2block[p] = {puz_block_type::STOP, 1};
                break;
            case PUZ_BLOCK_FIRE:
                m_cells.push_back(ch);
                m_pos2block[p] = {puz_block_type::FIRE, 1};
                break;
            default:
                if (isdigit(ch)) {
                    m_pos2block[p] = {puz_block_type::ICE, ch - '0'};
                    m_cells.push_back(PUZ_BLOCK_ICE);
                } else {
                    int n = arrows.find(ch);
                    if (n != -1) {
                        m_pos2dir[p] = n;
                        m_cells.push_back(PUZ_BLOCK_ARROW);
                    }
                }
            }
        m_cells.push_back(PUZ_BLOCK_FIXED);
    }
    m_cells.append(cols(), PUZ_BLOCK_FIXED);
}

struct puz_move : pair<Position, int>
{
    puz_move(const Position& p, int n) : pair<Position, int>(p, n) {}
};

ostream& operator<<(ostream &out, const puz_move &mi)
{
    out << format("move: {} {}\n", mi.first, dirs[mi.second]);
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, Position os, bool is_test);

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0 && boost::count_if(m_cells, [](char ch) {
            return ch == PUZ_HOLE_EMPTY || ch == PUZ_HOLE_BONSAI;
        }) == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_pos2block.size(); }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const { if (m_move) out << *m_move; }
    ostream& dump(ostream& out) const;
    
    const puz_game* m_game;
    string m_cells;
    boost::optional<puz_move> m_move;
    map<Position, puz_block> m_pos2block;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g), m_pos2block(g.m_pos2block)
{
}

bool puz_state::make_move(const Position& p, int n)
{
    if (!make_move2(p, offset[n], false))
        return false;

    m_move = puz_move(p, n);
    m_pos2block.erase(p);

    return boost::algorithm::any_of(m_pos2block, [&](const pair<const Position, puz_block>& kv) {
        return kv.second.m_type == puz_block_type::FIRE;
    }) || boost::algorithm::all_of(m_pos2block, [&](const pair<const Position, puz_block>& kv) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            return make_move2(kv.first, os, true);
        });
    });
}

bool puz_state::make_move2(const Position& p, Position os, bool is_test)
{
    auto& blk = m_pos2block.at(p);
    int n = blk.m_num;
    auto t = blk.m_type;
    if (!is_test)
        cells(p) = PUZ_BLOCK_USED;
    set<Position> p_arrows;
    for (auto p2 = p + os;; p2 += os)
        switch(char& ch = cells(p2)) {
        case PUZ_BLOCK_ARROW:
            if (p_arrows.contains(p2))
                return false;
            os = offset[m_game->m_pos2dir.at(p2)];
            p_arrows.insert(p2);
            break;
        case PUZ_HOLE_EMPTY:
            if (!is_test)
                ch = t == puz_block_type::STOP ? PUZ_HOLE_STOP :
                    t == puz_block_type::FIRE ? PUZ_HOLE_FIRE :
                    PUZ_HOLE_ICE;
            if (--n == 0)
                return true;
            break;
        case PUZ_HOLE_BONSAI:
            if (t != puz_block_type::ICE)
                return false;
            if (!is_test)
                ch = PUZ_HOLE_TREE;
            if (--n == 0)
                return true;
            break;
        case PUZ_HOLE_ICE:
            if (!is_test && t == puz_block_type::FIRE)
                ch = PUZ_HOLE_EMPTY;
            break;
        default:
            if (t == puz_block_type::FIRE) {
                char& ch2 = cells(p2 - os);
                if (is_test && ch2 == PUZ_HOLE_ICE ||
                    !is_test && ch2 == PUZ_HOLE_EMPTY) {
                    if (!is_test)
                        ch2 = PUZ_HOLE_FIRE;
                    return true;
                }
            }
            return false;
        }

    // should not reach here
    return false;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (auto& [p, block] : m_pos2block)
        for (int n = 0; n < 4; ++n)
            if (children.push_back(*this); !children.back().make_move(p, n))
                children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 1; r < rows() - 1; ++r) {
        for (int c = 1; c < cols() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (ch == PUZ_BLOCK_ICE ? char(m_game->m_pos2block.at(p).m_num + '0') :
                ch == PUZ_BLOCK_ARROW ? arrows[m_game->m_pos2dir.at(p)] :
                ch == PUZ_HOLE_EMPTY ? '.' : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PuzzleRetreat()
{
    using namespace puzzles::PuzzleRetreat;
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
    //    "Puzzles/PuzzleRetreat_Welcome.xml", "Puzzles/PuzzleRetreat_Welcome.txt");
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
    //    "Puzzles/PuzzleRetreat_Morning.xml", "Puzzles/PuzzleRetreat_Morning.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PuzzleRetreat_Everest.xml", "Puzzles/PuzzleRetreat_Everest.txt");
}
