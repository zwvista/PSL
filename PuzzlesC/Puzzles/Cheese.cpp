#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
 http://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=0558
 チーズ (Cheese)
 問題
 今年も JOI 町のチーズ工場がチーズの生産を始め，ねずみが巣から顔を出した．JOI 町は東西南北に区画整理されていて，各区画は巣，チーズ工場，障害物，空き地のいずれかである．ねずみは巣から出発して全てのチーズ工場を訪れチーズを 1 個ずつ食べる．
 
 この町には，N 個のチーズ工場があり，どの工場も１種類のチーズだけを生産している．チーズの硬さは工場によって異なっており，硬さ 1 から N までのチーズを生産するチーズ工場がちょうど 1 つずつある．
 
 ねずみの最初の体力は 1 であり，チーズを 1 個食べるごとに体力が 1 増える．ただし，ねずみは自分の体力よりも硬いチーズを食べることはできない．
 
 ねずみは，東西南北に隣り合う区画に 1 分で移動することができるが，障害物の区画には入ることができない．チーズ工場をチーズを食べずに通り過ぎることもできる．すべてのチーズを食べ終えるまでにかかる最短時間を求めるプログラムを書け．ただし，ねずみがチーズを食べるのにかかる時間は無視できる．
 
 入力
 入力は H+1 行ある．1 行目には 3 つの整数 H，W，N (1 ≤ H ≤ 1000，1 ≤ W ≤ 1000，1 ≤ N ≤ 9) がこの順に空白で区切られて書かれている．2 行目から H+1 行目までの各行には，'S'，'1', '2', ..., '9'，'X'，'.' からなる W 文字の文字列が書かれており，各々が各区画の状態を表している．北から i 番目，西から j 番目の区画を (i,j) と記述することにすると (1 ≤ i ≤ H, 1 ≤ j ≤ W)，第 i+1 行目の j 番目の文字は，区画 (i,j) が巣である場合は 'S' となり，障害物である場合は 'X' となり，空き地である場合は '.' となり，硬さ 1, 2, ..., 9 のチーズを生産する工場である場合はそれぞれ '1', '2', ..., '9' となる．入力には巣と硬さ 1, 2, ..., N のチーズを生産する工場がそれぞれ 1 つずつある．他のマスは障害物または空き地であることが保証されている．ねずみは全てのチーズを食べられることが保証されている．
 
 出力
 すべてのチーズを食べ終えるまでにかかる最短時間（分）を表す整数を 1 行で出力せよ．
*/

namespace puzzles{ namespace Cheese{

#define PUZ_SPACE       '.'
#define PUZ_NEST        'S'
#define PUZ_BLOCK       'X'
#define PUZ_MOUSE       'M'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
const string dirs = "urdl";

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    Position m_nest;
    map<int, Position> m_cheese2pos;
    map<Position, int> m_pos2cheese;
    int m_cheese_count;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second <= cols();
    }
    char cells(const Position& p) const { return m_start[p.first * cols() + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() - 1, strs[1].length())
{
    for (int r = 0; r < rows(); ++r) {
        auto& str = strs[r + 1];
        for (int c = 0; c < cols(); ++c) {
            char ch = str[c];
            m_start.push_back(ch);
            Position p(r, c);
            if (ch == PUZ_NEST)
                m_nest = p;
            else if (isdigit(ch)) {
                int n = ch - '0';
                m_cheese2pos[n] = p;
                m_pos2cheese[p] = n;
            }
        }
    }
    m_cheese_count = m_cheese2pos.size();
}

struct puz_state : Position
{
    puz_state() {}
    puz_state(const puz_game& g)
        : Position(g.m_nest), m_game(&g), m_move() {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    void make_move(int n);

    // solve_puzzle interface
    bool is_goal_state() const {
        return m_health == m_game->m_cheese_count + 1 && *this == m_game->m_cheese2pos.at(m_game->m_cheese_count);
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        unsigned int d = 0;
        for (int i = m_health; i <= m_game->m_cheese_count; ++i)
            d += manhattan_distance(i == m_health ? *this : m_game->m_cheese2pos.at(i - 1), m_game->m_cheese2pos.at(i));
        return d;
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    int m_health = 1;
    char m_move;
};

void puz_state::make_move(int n)
{
    m_move = dirs[n];
    auto &p = static_cast<Position&>(*this);
    auto it = m_game->m_pos2cheese.find(p = *this + offset[n]);
    if (it != m_game->m_pos2cheese.end() && it->second == m_health) ++m_health;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; i++) {
        auto p = *this + offset[i];
        if (m_game->is_valid(p) && m_game->cells(p) != PUZ_BLOCK) {
            children.push_back(*this);
            children.back().make_move(i);
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            out << (p == *this ? PUZ_MOUSE : m_game->cells(p));
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Cheese()
{
    using namespace puzzles::Cheese;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Cheese.xml", "Puzzles/Cheese.txt");
}
