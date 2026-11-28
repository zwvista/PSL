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

namespace puzzles::FunnyNumbers2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_UNKNOWN = -1;

constexpr auto offset = Position::Directions4;
constexpr auto offset2 = Position::WallsOffset4;

struct puz_numbers : set<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        for (int i = 0; i < num; ++i)
            insert(i + '1');
    }
};

struct puz_move
{
    vector<Position> m_rng;
    vector<char> m_comb;
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
    map<Position, puz_numbers> m_pos2nums;
    string m_cells;
    map<int, vector<puz_move>> m_area2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    char& cells(const Position& p) { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : Position
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    Position m_p_start;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
        if (!walls.contains(p_wall) && p.first >= m_p_start.first)
            children.emplace_back(*this).make_move(p);
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2 - 1)
, m_areas(m_sidelen * 2)
, m_cells(m_sidelen * m_sidelen, PUZ_SPACE)
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
                cells(p) = m_pos2num[p] = ch;
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
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, *rng.begin()});
        auto& a = m_areas.emplace_back();
        for (auto& p : smoves) {
            m_pos2area[p] = n;
            a.push_back(p);
            m_areas[p.first].push_back(p);
            m_areas[m_sidelen + p.second].push_back(p);
            rng.erase(p);
        }
    }

    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_pos2nums[p] = puz_numbers(m_areas[m_pos2area.at(p)].size());
        }
    for (auto& [p, ch] : m_pos2num) {
        m_pos2nums.erase(p);
        for (auto& os : offset) {
            auto p2 = p + os;
            if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end())
                it->second.erase(ch);
        }
        auto& area = m_areas[m_pos2area.at(p)];
        for (auto& p2 : area)
            if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end())
                it->second.erase(ch);
    }

    for (int i = 0; i < m_sidelen * 2; ++i) {
        int& sum = m_area2num.at(i);
        if (sum == PUZ_UNKNOWN) continue;

        auto rng = m_areas[i];
        boost::remove_erase_if(rng, [&](const Position& p2) {
            char ch = cells(p2);
            return ch == PUZ_SPACE ? false : (sum -= ch - '0', true);
        });

        vector<puz_numbers> nums2D;
        for (auto& p2 : rng)
            nums2D.push_back(m_pos2nums.at(p2));

        auto combs = [&] {
            auto& input = nums2D;
            vector<vector<char>> combs;
            vector<char> path;

            function<void(int)> backtrack = [&](int index) {
                if (index == input.size()) {
                    if (boost::accumulate(path, 0, [&](int acc, char ch2) {
                        return acc + (ch2 - '0');
                    }) == sum)
                        combs.push_back(path);
                    return;
                }

                for (int num : input[index]) {
                    path.push_back(num);
                    backtrack(index + 1);
                    path.pop_back();
                }
            };

            backtrack(0);
            return combs;
        }();
        for (auto& comb : combs)
            m_area2moves[i].emplace_back(rng, comb);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    bool make_move(int area_id, int move_id);
    void make_move2(int area_id, int move_id);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    map<int, vector<int>> m_matches;
    string m_cells;
    unsigned int m_distance = 0;
    map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells), m_pos2nums(g.m_pos2nums)
{
    for (auto& [area_id, moves] : g.m_area2moves)
        for (int i = 0; i < moves.size(); ++i)
            m_matches[area_id].push_back(i);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [area_id, move_ids] : m_matches) {
        auto& moves = m_game->m_area2moves.at(area_id);

        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [rng, comb] = moves[id];
            for (int i = 0; i < rng.size(); ++i)
                if (auto& nums = m_pos2nums.at(rng[i]); !nums.contains(comb[i]))
                    return true;
            return false;
        });

        if (!init)
            switch (move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, move_ids[0]), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int area_id, int move_id)
{
    auto& [rng, comb] = m_game->m_area2moves.at(area_id)[move_id];
    for (int i = 0; i < rng.size(); ++i) {
        auto& p = rng[i];
        char ch = comb[i];
        cells(p) = ch;
        m_pos2nums[p] = {ch};
        for (auto& os : offset) {
            auto p2 = p + os;
            if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end())
                it->second.erase(ch);
        }
        auto& area = m_game->m_areas[m_game->m_pos2area.at(p)];
        for (auto& p2 : area)
            if (auto it = m_pos2nums.find(p2); it != m_pos2nums.end())
                it->second.erase(ch);
    }
    m_matches.erase(area_id);
    ++m_distance;
}

bool puz_state::make_move(int area_id, int move_id)
{
    m_distance = 0;
    make_move2(area_id, move_id);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [area_id, comb_ids] = *boost::min_element(m_matches, [](
        const pair<int, vector<int>>& kv1,
        const pair<int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int comb_id : comb_ids)
        if (!children.emplace_back(*this).make_move(area_id, comb_id))
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

void solve_puz_FunnyNumbers2()
{
    using namespace puzzles::FunnyNumbers2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FunnyNumbers.xml", "Puzzles/FunnyNumbers2.txt", solution_format::GOAL_STATE_ONLY);
}
