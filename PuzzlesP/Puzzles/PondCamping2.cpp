#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 5/Pond camping

    Summary
    Splash!

    Description
    1. The numbers are Ponds. From each Pond you can have a hike of that many
       tiles as the number marked on it.
*/

namespace puzzles::PondCamping2{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FOREST = '=';
constexpr auto PUZ_POND = 'P';

constexpr auto offset = Position::Directions4;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_FOREST);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_FOREST);
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (char ch = str[c - 1]; ch == PUZ_SPACE)
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_POND);
                m_pos2num[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        m_cells.push_back(PUZ_FOREST);
    }
    m_cells.append(m_sidelen, PUZ_FOREST);
}

using puz_pos2area = map<Position, set<Position>>;

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(Position p);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_pos2area, 0, [&](int acc, const puz_pos2area::value_type& kv) {
            return acc + m_game->m_pos2num.at(kv.first) + 1 - kv.second.size();
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    puz_pos2area m_pos2area;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [p, _1] : g.m_pos2num)
        m_pos2area[p].insert(p);
}

bool puz_state::make_move(Position p)
{
    auto h = get_heuristic();
    cells(p) = PUZ_EMPTY;

    for (bool changed = true; changed;) {
        changed = false;

        set<Position> all_reachable;

        for (auto it = m_pos2area.begin(); it != m_pos2area.end();) {
            auto& [pnum, inner] = *it;
            int num = m_game->m_pos2num.at(pnum);

            auto mark_forest_around = [&] {
                for (auto& p2 : inner)
                    for (auto& os : offset) {
                        auto p3 = p2 + os;
                        if (char& ch = cells(p3); ch == PUZ_SPACE)
                            ch = PUZ_FOREST;
                    }
            };

            // 1. 轻量吸收周围已有的 PUZ_EMPTY 邻居
            bool inner_expanded = true;
            while (inner_expanded) {
                inner_expanded = false;
                vector<Position> to_add;
                for (auto& p2 : inner) {
                    for (auto& os : offset) {
                        auto p3 = p2 + os;
                        if (cells(p3) == PUZ_EMPTY && !inner.contains(p3)) {
                            to_add.push_back(p3);
                        }
                    }
                }
                if (!to_add.empty()) {
                    inner.insert(to_add.begin(), to_add.end());
                    inner_expanded = true;
                }
            }

            int sz = inner.size() - 1;
            if (sz > num)
                return false; // 超过步数，剪枝
            else if (sz == num) {
                mark_forest_around();
                it = m_pos2area.erase(it);
                changed = true;
                continue;
            }

            // 2. 计算当前向外延伸的边界 (outer)
            set<Position> outer;
            for (auto& p2 : inner)
                for (auto& os : offset) {
                    auto p3 = p2 + os;
                    if (cells(p3) == PUZ_SPACE)
                        outer.insert(p3);
                }

            if (outer.empty())
                return false; // 无路可走，剪枝

            // 规则 A：唯一出口强行填充
            if (outer.size() == 1) {
                auto& p2 = *outer.begin();
                cells(p2) = PUZ_EMPTY;
                inner.insert(p2);
                changed = true;
            }

            // 3. 计算最大连通上限 (用带距离制限的队列 BFS)
            set<Position> max_reachable = inner;
            map<Position, int> dist;
            queue<Position> q;

            for (auto& p2 : inner) {
                dist[p2] = 0;
                q.push(p2);
            }

            int rem = num + 1 - inner.size();
            while (!q.empty()) {
                auto curr = q.front();
                q.pop();

                int d = dist[curr];
                if (d > rem) continue;

                for (auto& os : offset) {
                    auto next_p = curr + os;
                    char ch = cells(next_p);
                    if ((ch == PUZ_SPACE || ch == PUZ_EMPTY) && !dist.contains(next_p)) {
                        dist[next_p] = d + 1;
                        max_reachable.insert(next_p);
                        q.push(next_p);
                    }
                }
            }

            // 收集所有在步数限制内可达的格子
            for (auto& [p_reach, d] : dist)
                if (d <= rem)
                    all_reachable.insert(p_reach);

            int sz2 = max_reachable.size() - 1;
            if (sz2 < num)
                return false; // 可达空间不足，剪枝
            else if (sz2 == num) {
                // 规则 B：如果可达空间刚好等于所需空间 -> 全部置为 EMPTY
                for (auto& p2 : max_reachable)
                    if (cells(p2) == PUZ_SPACE)
                        cells(p2) = PUZ_EMPTY;

                inner = max_reachable;
                mark_forest_around();
                it = m_pos2area.erase(it);
                changed = true;
            } else {
                it++;
            }
        }

        // 4. 清理全图不可达的孤立空格
        for (int r = 1; r < sidelen() - 1; ++r) {
            for (int c = 1; c < sidelen() - 1; ++c) {
                Position p_cell(r, c);
                if (char& ch = cells(p_cell); ch == PUZ_SPACE && !all_reachable.contains(p_cell)) {
                    ch = PUZ_FOREST;
                    changed = true;
                }
            }
        }
    }

    m_distance = h - get_heuristic();
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    puz_pos2area pos2outer;
    for (auto& [pnum, inner] : m_pos2area) {
        auto& outer = pos2outer[pnum];
        outer.clear();
        for (auto& p : inner)
            for (auto& os : offset)
                if (auto p2 = p + os; cells(p2) == PUZ_SPACE)
                    outer.insert(p2);
    }
    auto& [pnum, _1] = *boost::min_element(m_pos2area, [&](
        const puz_pos2area::value_type& kv1,
        const puz_pos2area::value_type& kv2) {
            auto f = [&](const puz_pos2area::value_type& kv) {
            auto& [pnum, inner] = kv;
            auto& outer = pos2outer.at(pnum);
            return pair(outer.size(), m_game->m_pos2num.at(pnum) - inner.size());
        };
        return f(kv1) < f(kv2);
    });
    for (auto& p : pos2outer.at(pnum))
        if (!children.emplace_back(*this).make_move(p))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it != m_game->m_pos2num.end())
                out << format("{:<2}", it->second);
            else {
                char ch = cells(p);
                out << (ch == PUZ_SPACE ? PUZ_FOREST : ch) << ' ';
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PondCamping2()
{
    using namespace puzzles::PondCamping2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PondCamping.xml", "Puzzles/PondCamping2.txt", solution_format::GOAL_STATE_ONLY);
}
