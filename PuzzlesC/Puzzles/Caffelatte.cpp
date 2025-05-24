#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 2/Puzzle Set 7/Caffelatte

    Summary
    Cows and Coffee

    Description
    1. Make Cappuccino by linking each cup to one or more coffee beans and cows.
    2. Links must be straight lines, not crossing each other.
    3. To each cup there must be linked an equal number of beans and cows. At
       least one of each.
    4. When linking multiple beans and cows, you can also link cows to cows and
       beans to beans, other than linking them to the cup.
*/

namespace puzzles::Caffelatte{

#define PUZ_SPACE             ' '
#define PUZ_CUP               'C'
#define PUZ_MILK              'M'
#define PUZ_BEAN              'B'
#define PUZ_HORZ              '-'
#define PUZ_VERT              '|'
#define PUZ_BOUNDARY          '+'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};
    
using puz_link = pair<int, Position>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    set<Position> m_objects;
    map<Position, vector<puz_link>> m_cup2milklinks, m_cup2beanlinks, m_milk2links, m_bean2links;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; ; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            char ch = str[c - 1];
            m_start.push_back(ch);
            Position p(r * 2 - 1, c * 2 - 1);
            if (ch != PUZ_SPACE)
                m_objects.insert(p);
            if (c == m_sidelen / 2) break;
            m_start.push_back(PUZ_SPACE);
        }
        m_start.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_start.push_back(PUZ_BOUNDARY);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& p : m_objects) {
        char ch = cells(p);
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i];
            for (auto p2 = p + os; ; p2 += os)
                if (char ch2 = cells(p2); ch2 != PUZ_SPACE) {
                    if (ch == PUZ_CUP && ch2 == PUZ_MILK)
                        m_cup2milklinks[p].emplace_back(i, p2);
                    else if (ch == PUZ_CUP && ch2 == PUZ_BEAN)
                        m_cup2beanlinks[p].emplace_back(i, p2);
                    else if (ch == PUZ_MILK && (ch2 == PUZ_CUP || ch2 == PUZ_MILK))
                        m_milk2links[p].emplace_back(i, p2);
                    else if (ch == PUZ_BEAN && (ch2 == PUZ_CUP || ch2 == PUZ_BEAN))
                        m_bean2links[p].emplace_back(i, p2);
                    break;
                }
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches_cup2milk, m_matches_cup2bean, m_matches_milk, m_matches_bean) <
            tie(x.m_cells, x.m_matches_cup2milk, x.m_matches_cup2bean, x.m_matches_milk, x.m_matches_bean);
    }
    bool make_move(const map<Position, vector<int>>& matches, const Position& p, int n);
    void make_move2(const map<Position, vector<int>>& matches, const Position& p, int n);
    int find_matches(bool init);
    bool is_connected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_matches_cup2milk.size() + m_matches_cup2bean.size() + m_matches_milk.size() + m_matches_bean.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the position of the object
    // value.elem: the id of link that connects the object at this position
    map<Position, vector<int>> m_matches_cup2milk, m_matches_cup2bean, m_matches_milk, m_matches_bean;
    map<Position, Position> m_obj2cup;
    Position m_last_cup;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    if (g.m_cup2milklinks.size() != g.m_cup2beanlinks.size() ||
        g.m_milk2links.size() != g.m_bean2links.size())
        return;
    
    auto f = [&](const map<Position, vector<puz_link>>& pos2links, map<Position, vector<int>>& matches) {
        for (auto&& [p, links] : pos2links) {
            auto& perm_ids = matches[p];
            perm_ids.resize(links.size());
            boost::iota(perm_ids, 0);
        }
    };
    f(g.m_cup2milklinks, m_matches_cup2milk);
    f(g.m_cup2beanlinks, m_matches_cup2bean);
    f(g.m_milk2links, m_matches_milk);
    f(g.m_bean2links, m_matches_bean);

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& matches: {m_matches_cup2milk, m_matches_cup2bean, m_matches_milk, m_matches_bean})
        for (auto& kv : matches) {
            const auto& p = kv.first;
            auto& perm_ids = const_cast<vector<int>&>(kv.second);
            auto& links =
                (matches == m_matches_cup2milk ? m_game->m_cup2milklinks :
                matches == m_matches_cup2bean ? m_game->m_cup2beanlinks :
                matches == m_matches_milk ? m_game->m_milk2links :
                m_game->m_bean2links).at(p);

            boost::remove_erase_if(perm_ids, [&](int id) {
                auto& [i, p2] = links[id];
                auto& os = offset[i];
                for(auto p3 = p + os; p3 != p2; p3 += os)
                    if (cells(p3) != PUZ_SPACE)
                        return true;
                return false;
            });

            if (!init)
                switch(perm_ids.size()) {
                case 0:
                    return 0;
                }
        }
    return 2;
}

void puz_state::make_move2(const map<Position, vector<int>>& matches, const Position& p, int n)
{
    auto f = [&](const Position& p, int i, const Position& p2) {
        auto& os = offset[i];
        char ch = i % 2 == 1 ? PUZ_HORZ : PUZ_VERT;
        for (auto p3 = p + os; p3 != p2; p3 += os)
            cells(p3) = ch;
    };
    
    if (matches == m_matches_cup2milk) {
        auto&& [i, p2] = m_game->m_cup2milklinks.at(p)[n];
        f(p, i, p2);
        m_distance += 2;
        m_obj2cup[p] = p, m_obj2cup[p2] = p;
        m_last_cup = p;
        m_matches_cup2milk.erase(p), m_matches_milk.erase(p2);
    } else if (matches == m_matches_cup2bean) {
        auto&& [i, p2] = m_game->m_cup2beanlinks.at(p)[n];
        f(p, i, p2);
        m_distance += 2;
        m_obj2cup[p] = p, m_obj2cup[p2] = p;
        m_matches_cup2bean.erase(p), m_matches_bean.erase(p2);
    } else if (matches == m_matches_milk) {
        auto&& [i, p2] = m_game->m_milk2links.at(p)[n];
        f(p, i, p2);
        ++m_distance;
        m_obj2cup[p] = m_last_cup = m_obj2cup.at(p2);
        m_matches_milk.erase(p);
    } else {
        auto&& [i, p2] = m_game->m_bean2links.at(p)[n];
        f(p, i, p2);
        ++m_distance;
        m_obj2cup[p] = m_obj2cup.at(p2);
        m_matches_bean.erase(p);
    }
}

bool puz_state::make_move(const map<Position, vector<int>>& matches, const Position& p, int n)
{
    m_distance = 0;
    make_move2(matches, p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int sz1 = m_matches_cup2milk.size(), sz2 = m_matches_cup2bean.size();
    if (sz1 != 0 || sz2 != 0)
        if (sz1 == sz2) {
            auto& kv = *boost::min_element(m_matches_cup2milk, [](
                const pair<const Position, vector<int>>& kv1,
                const pair<const Position, vector<int>>& kv2) {
                return kv1.second.size() < kv2.second.size();
            });

            for (int n : kv.second) {
                children.push_back(*this);
                if (!children.back().make_move(m_matches_cup2milk, kv.first, n))
                    children.pop_back();
            }
        } else
            for (int n : m_matches_cup2bean.at(m_last_cup)) {
                children.push_back(*this);
                if (!children.back().make_move(m_matches_cup2bean, m_last_cup, n))
                    children.pop_back();
            }
    else {
        sz1 = m_matches_milk.size(), sz2 = m_matches_bean.size();
        if (sz1 == sz2) {
            auto matches = m_matches_milk;
            for (auto& kv : matches) {
                auto& p = kv.first;
                auto& perm_ids = kv.second;
                auto& links = m_game->m_milk2links.at(p);
                boost::remove_erase_if(perm_ids, [&](int id){
                    auto&& [i, p2] = links[id];
                    return !m_obj2cup.contains(p2);
                });
            }
            auto& kv = *boost::min_element(matches, [](
                const pair<const Position, vector<int>>& kv1,
                const pair<const Position, vector<int>>& kv2) {
                return kv1.second.size() < kv2.second.size();
            });

            for (int n : kv.second) {
                children.push_back(*this);
                if (!children.back().make_move(m_matches_milk, kv.first, n))
                    children.pop_back();
            }
        } else
            for (auto& kv : m_matches_bean) {
                auto& p = kv.first;
                auto& links = m_game->m_bean2links.at(p);
                for (int n : kv.second) {
                    auto&& [i, p2] = links[n];
                    auto it = m_obj2cup.find(p2);
                    if (it != m_obj2cup.end() && it->second == m_last_cup) {
                        children.push_back(*this);
                        if (!children.back().make_move(m_matches_bean, p, n))
                            children.pop_back();
                    }
                }
            }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c)
            out << cells({r, c});
        println(out);
    }
    return out;
}

}

void solve_puz_Caffelatte()
{
    using namespace puzzles::Caffelatte;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Caffelatte.xml", "Puzzles/Caffelatte.txt", solution_format::GOAL_STATE_ONLY);
}
