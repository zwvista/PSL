#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Number Connect

    Summary
    Connect the numbers 

    Description
*/

namespace puzzles::NumberConnect{

constexpr auto PUZ_SPACE = ' ';

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_line
{
    Position m_start, m_end;
    int m_num;
};

using puz_cell = pair<char, int>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, puz_line> m_pos2line;
    vector<puz_cell> m_cells;

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
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (auto s = str.substr(c * 3, 3); s == "   ")
                m_cells.emplace_back(PUZ_SPACE, -1);
            else {
                char ch = s[0];
                auto& line = m_pos2line[ch];
                int n = stoi(string(s.substr(1)));
                if (line.m_num == 0)
                    line.m_start = p;
                else
                    line.m_end = p, line.m_num = n;
                m_cells.emplace_back(ch, n);
            }
        }
    }

}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    string cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    string& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(int n);
    void new_link() {
        if (m_num2targets.empty()) return;
        auto& targets = m_num2targets.back().second;
        m_link = {targets.back()};
        targets.pop_back();
    }
    set<Position> get_area(char ch) const;

    //solve_puzzle interface
    bool is_goal_state() const {
        return m_num2targets.empty() && get_heuristic() == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return 0; }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    vector<string> m_cells;
    vector<pair<char, vector<Position>>> m_num2targets;
    vector<Position> m_link;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells.size()), m_game(&g)
{

    new_link();
}

set<Position> puz_state::get_area(char ch) const
{
    set<Position> area;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p)[0] == ch)
                area.insert(p);
        }
    return area;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a, const Position& p_start)
        : m_area(&a) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->contains(p)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::make_move(int n)
{

    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& p = m_link.back();
    char num1 = cells(p)[0];
    for (int i = 0; i < 4; ++i) {
        auto p2 = p + offset[i];
        char num2 = cells(p2)[0];
        if (num2 == PUZ_SPACE ||
            num2 == num1 && boost::algorithm::none_of_equal(m_link, p2))
            if (children.push_back(*this); !children.back().make_move(i))
                children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 1;; ++r) {
    //    // draw horizontal lines
    //    for (int c = 1; c < sidelen() - 1; ++c) {
    //        Position p(r, c);
    //        auto str = cells(p);
    //        auto it = m_game->m_pos2num.find(p);
    //        out << (it != m_game->m_pos2num.end() ? it->second : ' ')
    //            ;
    //    }
    //    println(out);
    //    if (r == sidelen() - 2) break;
    //    println(out);
    //}
    return out;
}

}

void solve_puz_NumberConnect()
{
    using namespace puzzles::NumberConnect;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberConnect.xml", "Puzzles/NumberConnect.txt", solution_format::GOAL_STATE_ONLY);
}
