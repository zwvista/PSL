#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 3/Hidden Path

    Summary
    Jump once on every tile, following the arrows

    Description
    Starting at the tile number 1, reach the last tile by jumping from tile to tile.
    1. When jumping from a tile, you have to follow the direction of the arrow and
       land on a tile in that direction
    2. Although you have to follow the direction of the arrow, you can land on any
       tile in that direction, not just the one next to the current tile.
    3. The goal is to jump on every tile, only once and reach the last tile.
*/

namespace puzzles::HiddenPath{

constexpr auto PUZ_UNKNOWN = 0;

constexpr array<Position, 8> offset = Position::Directions8;

// key: direction, value: step
using puz_move = pair<int, int>;
constexpr puz_move PUZ_UNKNOWN_MOVE{8, 0};

struct puz_generator
{
    int m_sidelen, m_max_num;
    vector<int> m_cells;
    vector<puz_move> m_moves;
    vector<vector<puz_move>> m_valid_moves;
    Position m_start_p, m_end_p;
    bool m_solved = false;
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    int index(const Position& p) const { return p.first * m_sidelen + p.second; }
    int& cells(const Position& p) { return m_cells[index(p)]; }
    puz_move& moves(const Position& p) { return m_moves[index(p)]; }
    vector<puz_move>& valid_moves(const Position& p) { return m_valid_moves[index(p)]; }

    puz_generator(int n);
    void gen_puzzle() { gen_move(1, m_start_p); }
    void gen_move(int n, Position p);
    string to_string();
};

puz_generator::puz_generator(int n)
    : m_sidelen(n)
    , m_max_num(n * n)
    , m_cells(m_max_num, PUZ_UNKNOWN)
    , m_moves(m_max_num, PUZ_UNKNOWN_MOVE)
    , m_valid_moves(m_max_num)
{
    int start_n = 0, end_n = m_max_num - 1;
    // start and end can be randomized
//     do {
//         start_n = rand() % m_max_num;
//         end_n = rand() % m_max_num;
//     } while (start_n == end_n);
    m_start_p = {start_n / n, start_n % n}, m_end_p = {end_n / n, end_n % n};
    cells(m_start_p) = 1, cells(m_end_p) = m_max_num;

    for (int r = 0; r < n; ++r)
        for (int c = 0; c < n; ++c) {
            Position p(r, c);
            if (p == m_end_p)
                continue;
            auto& mv = valid_moves(p);
            for (int i = 0; i < offset.size(); ++i) {
                auto& os = offset[i];
                auto p2 = p + os;
                for (int j = 1; is_valid(p2) && p2 != m_start_p; p2 += os, ++j)
                    mv.emplace_back(i, j);
            }
            boost::random_shuffle(mv);
        }
}

void puz_generator::gen_move(int n, Position p)
{
    for (auto& mv : valid_moves(p)) {
        auto& [dir, step] = mv;
        auto& os = offset[dir];
        auto p2 = p;
        for (int i = 0; i < step; ++i)
            p2 += os;
        if (n == m_max_num - 1 && cells(p2) == m_max_num) {
            m_solved = true;
            moves(p) = mv;
            return;
        }
        if (cells(p2) == PUZ_UNKNOWN) {
            cells(p2) = n + 1;
            gen_move(n + 1, p2);
            if (m_solved) {
                moves(p) = mv;
                return;
            }
            cells(p2) = PUZ_UNKNOWN;
        }
    }
}

string puz_generator::to_string()
{
    stringstream ss;
    ss << endl;
    for (int r = 0; r < m_sidelen; r++) {
        for (int c = 0; c < m_sidelen; c++) {
            Position p(r, c);
            auto& [dir, _1] = moves(p);
            auto s = p == m_start_p || p == m_end_p ? format("{:02d}", cells(p)) : "  ";
            //auto s = format("{:02d}", cells(p));
            ss << s << char(dir + '0');
        }
        ss << '`' << endl;
    }
    return ss.str();
}

}

extern void solve_puz_HiddenPathTest();

bool is_valid_HiddenPath(const string& s)
{
    xml_document doc;
    auto levels = doc.append_child("puzzle").append_child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = "test";
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("../Test.xml");
    solve_puz_HiddenPathTest();
    ifstream in("../Test.txt");
    string x;
    getline(in, x);
    getline(in, x);
    return x != "Solution 1:";
}

void save_new_HiddenPath(const string& id, const string& s)
{
    xml_document doc;
    doc.load_file("Puzzles/HiddenPath/HiddenPathGen.xml");
    auto levels = doc.child("puzzle").child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = id.c_str();
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("Puzzles/HiddenPath/HiddenPathGen.xml");
}

void gen_puz_HiddenPath()
{
    using namespace puzzles::HiddenPath;

    for (int i = 4; i <= 4; ++i)
        for (int j = 1; j <= 8; ++j) {
            string s;
            for (;;) {
                puz_generator g(i);
                g.gen_puzzle();
                if (g.m_solved)
                    if (s = g.to_string(); is_valid_HiddenPath(s)) {
                        print("{}", s);
                        break;
                    }
            }
            stringstream ss;
            print(ss, "{}-{}", i, j);
            save_new_HiddenPath(ss.str(), s);
        }
}
