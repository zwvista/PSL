#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Number Connect

    Summary
    Connect the numbers 

    Description
*/

namespace puzzles::NumberConnect{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_UNKNOWN = -1;

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

struct puz_line
{
    char m_letter;
    Position m_start, m_end;
    int m_num;
};

using puz_cell = pair<char, int>;

struct puz_move
{
    char m_letter;
    vector<Position> m_path;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<char, puz_line> m_letter2line;
    vector<puz_cell> m_cells;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    const puz_cell& cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game* game, const puz_line* line)
        : m_game(game), m_line(line) {make_move(line->m_start);}

    bool is_goal_state() const { return size() == m_line->m_num + 1; }
    void make_move(const Position& p) { push_back(p); }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    const puz_line* m_line;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    auto& p = back();
    if (manhattan_distance(m_line->m_end, p) > m_line->m_num + 1 - size())
        return; // too far away from the end
    for (auto& os : offset) {
        auto p2 = p + os;
        auto& [ch, num] = m_game->cells(p2);
        if (ch == m_line->m_letter && num == m_line->m_num ||
            ch == PUZ_SPACE && boost::algorithm::none_of_equal(*this, p2)) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            if (auto s = str.substr(c * 3, 3); s == "   ")
                m_cells.emplace_back(PUZ_SPACE, PUZ_UNKNOWN);
            else {
                char ch = s[0];
                auto& [_1, start, end, num] = m_letter2line[ch];
                int n = stoi(string(s.substr(1)));
                if (n == 0)
                    start = p;
                else
                    end = p, num = n;
                m_cells.emplace_back(ch, n);
            }
        }
    }
    for (auto& [letter, line] : m_letter2line) {
        puz_state2 sstart(this, &line);
        list<list<puz_state2>> spaths;
        if (auto [found, _1] = puz_solver_bfs<puz_state2, false, false>::find_solution(sstart, spaths); found)
            for (auto& spath : spaths) {
                auto& s = spath.back();
                int n = m_moves.size();
                m_moves.emplace_back(letter, s);
                for (auto& p2 : s)
                    m_pos2move_ids[p2].push_back(n);
            }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    const puz_cell& cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    puz_cell& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_cell> m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells.size()), m_game(&g)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        //boost::remove_erase_if(move_ids, [&](int id) {
        //    auto& [rng, _2, rng2D] = m_game->m_areas[id];
        //    return !boost::algorithm::all_of(rng2D, [&](const set<Position>& rng2) {
        //        return boost::algorithm::all_of(rng2, [&](const Position& p2) {
        //            return cells(p2) == PUZ_SPACE ||
        //                boost::algorithm::any_of_equal(rng, p2);
        //            });
        //        });
        //    });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int n)
{
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
    auto& [_1, move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& move_id : move_ids)
        if (children.push_back(*this); !children.back().make_move(move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    //for (int r = 0; r < sidelen(); ++r) {
    //    for (int c = 0; c < sidelen(); ++c) {
    //        Position p(r, c);
    //        out << cells(p);
    //        if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
    //            out << ". ";
    //        else
    //            out << it->second << ' ';
    //    }
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
