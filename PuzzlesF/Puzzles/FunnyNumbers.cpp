#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 2/Funny Numbers

    Summary
    Hahaha ... haha ... ehm ...

    Description
    1. Fill each region with numbers 1 to X where the X is the region area.
    2. Same numbers can't touch each other horizontally or vertically across regions.
    3. The numbers outside tell you the sum of the row or column.
*/

namespace puzzles::FunnyNumbers{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_UNKNOWN = -1;

constexpr array<Position, 4> offset = Position::Directions4;

constexpr array<Position, 4> offset2 = Position::WallsOffset4;

struct puz_numbers : set<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        for (int i = 0; i < num; ++i)
            insert(i + '1');
    }
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2num;
    map<int, int> m_area2num;
    vector<vector<Position>> m_areas;
    map<Position, int> m_pos2area;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls), m_p_start(p_start) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
    Position m_p_start;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall) && p.first >= m_p_start.first)
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 - 1)
    , m_areas(m_sidelen * 2)
{
    set<Position> rng;
    for (int r = 0;; ++r) {
        // horizontal walls
        string_view str_h = strs[r * 2];
        for (int c = 0; c < m_sidelen; ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen) break;
        string_view str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen) break;
            char ch = str_v[c * 2 + 1];
            if (ch != PUZ_SPACE)
                m_pos2num[p] = ch;
            rng.insert(p);
        }
    }

    auto f = [&](int area_id, string s) {
         m_area2num[area_id] = s == "  " ? PUZ_UNKNOWN : stoi(s);
    };
    for (int i = 0; i < m_sidelen; ++i) {
        f(i, strs[i * 2 + 1].substr(m_sidelen * 2 + 1, 2));
        f(i + m_sidelen, strs[m_sidelen * 2 + 1].substr(i * 2, 2));
    }

    for (int n = m_sidelen * 2; !rng.empty(); ++n) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, *rng.begin()});
        m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            m_areas.back().push_back(p);
            m_areas[p.first].push_back(p);
            m_areas[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, char ch);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(m_cells, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<int, int> m_area2sum;
    map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_game(&g)
{
    for (int r = 0; r < sidelen(); ++r)
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            m_pos2nums[p] = puz_numbers(g.m_areas[g.m_pos2area.at(p)].size());
        }

    for (int i = 0; i < sidelen() * 2; ++i)
        m_area2sum[i] = g.m_area2num.at(i);

    for (auto& [p, ch] : g.m_pos2num)
        make_move(p, ch);
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2nums.erase(p);

    auto f = [&](const Position& p2, char ch2) {
        if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end()) {
            it->second.erase(ch2);
            return !it->second.empty();
        }
        return true;
    };

    for (auto& p2 : m_game->m_areas[m_game->m_pos2area.at(p)])
        if (p2 != p && !f(p2, ch))
            return false;

    // 2. Same numbers can't touch each other horizontally or vertically across regions.
    for (auto& os : offset)
        if (auto p2 = p + os; is_valid(p2) && !f(p2, ch))
            return false;

    auto& [r, c] = p;
    for (int area_id : {r, c + sidelen()}) {
        int& sum = m_area2sum.at(area_id);
        if (sum == PUZ_UNKNOWN) continue;
        sum -= ch - '0';

        auto rng = m_game->m_areas[area_id];
        boost::remove_erase_if(rng, [&](const Position& p2) {
            return cells(p2) != PUZ_SPACE;
        });
        int sz = rng.size();
        vector<puz_numbers> nums2D;
        for (auto& p2 : rng)
            nums2D.push_back(m_pos2nums.at(p2));

        auto nums2D_result = [&] {
            auto& input = nums2D;
            vector<vector<int>> result_nums;
            vector<puz_numbers> result(sz);
            vector<int> path;

            function<void(int)> backtrack = [&](int index) {
                if (index == input.size()) {
                    if (boost::accumulate(path, 0, [&](int acc, char ch2) {
                        return acc + (ch2 - '0');
                    }) == sum) {
                        result_nums.push_back(path);
                        for (int i = 0; i < sz; ++i)
                            result[i].insert(path[i]);
                    }
                    return;
                }

                for (int num : input[index]) {
                    path.push_back(num);
                    backtrack(index + 1);
                    path.pop_back();
                }
            };

            backtrack(0);
            return result;
        }();

        for (int i = 0; i < sz; ++i) {
            set<int> s;
            boost::set_difference(nums2D[i], nums2D_result[i], inserter(s, s.end()));
            for (char ch2 : s)
                if (!f(rng[i], ch2))
                    return false;
        }
    }

    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, nums] = *boost::min_element(m_pos2nums, [](
        const pair<const Position, puz_numbers>& kv1,
        const pair<const Position, puz_numbers>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (char ch : nums)
        if (!children.emplace_back(*this).make_move(p, ch))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int area_id) {
        //int sum = m_game->m_area2num.at(area_id);
        //if (sum == PUZ_UNKNOWN)
        //    out << "  ";
        //else
        //    out << format("{:2}", sum);
        auto& rng = m_game->m_areas[area_id];
        int sum = boost::accumulate(rng, 0, [&](int acc, const Position& p) {
            return acc + (cells(p) - '0');
        });
        out << format("{:2}", sum);
    };
    for (int r = 0;; ++r) {
        // draw horizontal lines
        for (int c = 0; c < sidelen(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen()) break;
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen()) {
                f(r);
                break;
            }
            out << cells(p);
        }
        println(out);
    }
    for (int c = 0; c < sidelen(); ++c)
        f(c + sidelen());
    println(out);
    return out;
}

}

void solve_puz_FunnyNumbers()
{
    using namespace puzzles::FunnyNumbers;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FunnyNumbers.xml", "Puzzles/FunnyNumbers.txt", solution_format::GOAL_STATE_ONLY);
}
