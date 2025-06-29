#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Move the Box
*/

namespace puzzles::MoveTheBox{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};
constexpr string_view dirs = "urdl";

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length())
{
    m_cells = boost::accumulate(strs, string());
}

struct puz_step : pair<Position, int>
{
    puz_step(const Position& p, int n) : pair<Position, int>(p + Position(1, 1), n) {}
};

ostream& operator<<(ostream &out, const puz_step &mi)
{
    out << format("move: {} {}\n", mi.first, dirs[mi.second]);
    return out;
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    char cells(const Position& p) const { return (*this)[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * cols() + p.second]; }
    bool make_move(const Position& p, int n);
    bool clear_boxes();
    bool fall_boxes();
    bool check_boxes() const;

    //solve_puzzle interface
    bool is_goal_state() const {
        return boost::count(*this, PUZ_SPACE) == rows() * cols();
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return 1; }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const { if (m_move) out << *m_move; }
    ostream& dump(ostream& out) const;
    
    const puz_game* m_game = nullptr;
    boost::optional<puz_step> m_move;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_cells), m_game(&g)
{
}

bool puz_state::clear_boxes()
{
    set<Position> boxes;
    auto f = [&](Position p, int d, int& n, char ch, char& ch_last) {
        if (ch_last != PUZ_SPACE && ch != ch_last && n > 2) {
            auto& os = offset[d];
            for (int i = 0; i < n; i++)
                boxes.insert(p -= os);
        }
        n = ch == PUZ_SPACE ? 0 : ch == ch_last ? n + 1 : 1;
        ch_last = ch;
    };
    for (int r = 0; r < rows(); ++r) {
        char ch_last = PUZ_SPACE;
        for (int c = 0, n = 0; c <= cols(); ++c) {
            Position p(r, c);
            f(p, 1, n, c < cols() ? cells(p) : PUZ_SPACE, ch_last);
        }
    }
    for (int c = 0; c < cols(); ++c) {
        char ch_last = PUZ_SPACE;
        for (int r = 0, n = 0; r <= rows(); ++r) {
            Position p(r, c);
            f(p, 2, n, r < rows() ? cells(p) : PUZ_SPACE, ch_last);
        }
    }
    for (auto& p : boxes)
        cells(p) = PUZ_SPACE;
    return !boxes.empty();
}

bool puz_state::fall_boxes()
{
    bool did_fall = false;
    for (int c = 0; c < cols(); ++c) {
        int n = 0;
        string boxes;
        for (int r = rows() - 1; r >= 0; --r) {
            char ch = cells({r, c});
            if (ch != PUZ_SPACE) {
                boxes = ch + boxes;
                n = rows() - r;
            }
        }
        int sz = boxes.size();
        if (n > sz) {
            did_fall = true;
            int r = rows() - n;
            for (int i = 0; i < n - sz; ++i)
                cells({r++, c}) = PUZ_SPACE;
            for (int i = 0; i < sz; ++i)
                cells({r++, c}) = boxes[i];
        }
    }
    return did_fall;
}

bool puz_state::check_boxes() const
{
    map<char, int> box2cnt;
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            char ch = cells({r, c});
            if (ch != PUZ_SPACE)
                box2cnt[ch]++;
        }
    return boost::algorithm::all_of(box2cnt, [](const pair<const char, int>& kv) {
        return kv.second > 2;
    });
}

bool puz_state::make_move(const Position& p, int n)
{
    m_move = puz_step(p, n);
    auto p2 = p + offset[n];
    ::swap(cells(p), cells(p2));
    while (clear_boxes() | fall_boxes());
    return check_boxes();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            auto f = [&](int n) {
                auto p2 = p + offset[n];
                char ch2 = cells(p2);
                if (ch == ch2 || ch == PUZ_SPACE && n == 2) return;
                if (ch == PUZ_SPACE) // n == 1
                    p = p2, n = 3;
                children.push_back(*this);
                if (!children.back().make_move(p, n))
                    children.pop_back();
            };
            if (r < rows() - 1)
                f(2);
            if (c < cols() - 1)
                f(1);
        }
}

ostream& puz_state::dump(ostream& out) const
{
    dump_move(out);
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << (ch == PUZ_SPACE ? '.' : ch) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_MoveTheBox()
{
    using namespace puzzles::MoveTheBox;
    //solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
    //    "Puzzles/MoveTheBox_Boston.xml", "Puzzles/MoveTheBox_Boston.txt");
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MoveTheBox_NewYork.xml", "Puzzles/MoveTheBox_NewYork.txt");
}
