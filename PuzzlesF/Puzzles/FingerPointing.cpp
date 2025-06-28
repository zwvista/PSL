#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "bfs_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Finger Pointing

    Summary
    Blame is in the air

    Description
    1. Fill the board with fingers. Two fingers pointing in the same direction
       can't be orthogonally adjacent.
    2. the number tells you how many fingers and finger 'trails' point to that tile.
*/

namespace puzzles::FingerPointing{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BLOCK = 'B';

const string_view dirs = "^>v<";

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position invalid_pos = {-1, -1};

using puz_paths = vector<vector<Position>>;
using puz_pointer = pair<Position, char>;
using puz_move = vector<vector<puz_pointer>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    map<Position, int> m_pos2num;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : vector<Position>
{
    puz_state2(const puz_game& game, const Position& p, int i, int n)
        : m_game(&game), m_last_dir(i), m_num(n) { make_move(i, p); }

    bool is_goal_state() const { return m_distance == m_num; }
    void make_move(int i, Position p2) {
        push_back(p2), m_last_dir = i, ++m_distance;
    }
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game = nullptr;
    int m_last_dir;
    int m_num;
    int m_distance = 0;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& p = back();
    for (int i = 0; i < 4; ++i) {
        if (m_last_dir == (i + 2) % 4 || m_last_dir == i) continue;
        if (auto p2 = p + offset[i];
            boost::algorithm::none_of_equal(*this, p2))
            if (m_game->cells(p2) == PUZ_SPACE) {
                children.push_back(*this);
                children.back().make_move(i, p2);
            }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BLOCK);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BLOCK);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            m_cells.push_back(ch);
            if (isdigit(ch))
                m_pos2num[{r, c}] = ch - '0';
        }
        m_cells.push_back(PUZ_BLOCK);
    }
    m_cells.append(m_sidelen, PUZ_BLOCK);

    for (auto& [p, sum] : m_pos2num) {
        vector<vector<int>> dir_nums(4);
        for (int i = 0; i < 4; ++i)
            if (cells(p + offset[i]) != PUZ_SPACE)
                dir_nums[i] = {0};
            else {
                dir_nums[i].resize(sum + 1);
                boost::iota(dir_nums[i], 0);
            }
        vector<vector<int>> perms;
        for (int n0 : dir_nums[0])
            for (int n1 : dir_nums[1])
                for (int n2 : dir_nums[2])
                    for (int n3 : dir_nums[3])
                        if (n0 + n1 + n2 + n3 == sum)
                            perms.push_back({n0, n1, n2, n3});
                                
        for (auto& v : perms) {
            vector<puz_paths> dir_paths(4);
            for (int i = 0; i < 4; ++i) {
                int n = v[i];
                if (n == 0)
                    dir_paths[i].push_back({invalid_pos});
                else {
                    puz_state2 sstart(*this, p + offset[i], i, n);
                    list<list<puz_state2>> spaths;
                    if (auto [found, _1] = puz_solver_bfs<puz_state2, true, false, false>::find_solution(sstart, spaths); found)
                        for (auto& spath : spaths)
                            dir_paths[i].push_back(spath.back());
                }
            }
            for (auto& path0 : dir_paths[0])
                for (auto& path1 : dir_paths[1])
                    for (auto& path2 : dir_paths[2])
                        for (auto& path3 : dir_paths[3]) {
                            puz_paths paths = {path0, path1, path2, path3};

                            puz_move move(4);
                            if ([&] {
                                vector<puz_pointer> pointersAll;
                                set<Position> rng;
                                for (auto& path : paths) {
                                    auto& pointers = move.emplace_back();
                                    boost::remove_erase(path, invalid_pos);
                                    for (int i = 0; i < path.size(); ++i) {
                                        auto& p2 = path[i];
                                        if (auto [_1, success] = rng.insert(p2); !success)
                                            return false;
                                        auto os2 = (i == 0 ? p : path[i - 1]) - p2;
                                        pointers.emplace_back(p2, dirs[boost::find(offset, os2) - offset]);
                                        pointersAll.push_back(pointers.back());
                                    }
                                }
                                for (auto& pointer1 : pointersAll) {
                                    auto& [p1, ch1] = pointer1;
                                    for (auto& pointer2 : pointersAll) {
                                        auto& [p2, ch2] = pointer2;
                                        if (p1 < p2 && ch1 == ch2 &&
                                            boost::algorithm::any_of_equal(offset, p2 - p1))
                                            return false;
                                    }
                                }
                                return true;
                            }()) {
                                int n = m_moves.size();
                                m_moves.push_back(move);
                                for (auto& pointers : move)
                                    for (auto& pointer: pointers)
                                        m_pos2move_id[pointer.first].push_back(n);
                            }
                        }
        }
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i);
    void make_move2(int i);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    // key: the position of the board
    // value: the index of the permutations
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_cells), m_game(&g)
, m_matches(g.m_pos2move_id)
{
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& move = m_game->m_moves[id];
            return !boost::algorithm::all_of(move, [&](const vector<puz_pointer>& pointers) {
                return boost::algorithm::all_of(pointers, [&](const puz_pointer& pointer) {
                    auto& [p2, ch2] = pointer;
                    return cells(p2) == PUZ_SPACE &&
                        boost::algorithm::none_of(offset, [&](const Position& os) {
                            return cells(p2 + os) == ch2;
                        });
                });
            });
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(move_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i)
{
    auto& move = m_game->m_moves[i];
    for (auto& pointers : move)
    for (auto& pointer : pointers) {
        auto& [p, ch] = pointer;
        cells(p) = ch;
        ++m_distance;
        m_matches.erase(p);
    }
}

bool puz_state::make_move(int i)
{
    m_distance = 0;
    make_move2(i);
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
    for (int n : move_ids) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_FingerPointing()
{
    using namespace puzzles::FingerPointing;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FingerPointing.xml", "Puzzles/FingerPointing.txt", solution_format::GOAL_STATE_ONLY);
}
