#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 7/Caffelatte

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

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_CUP = 'C';
constexpr auto PUZ_MILK = 'M';
constexpr auto PUZ_BEAN = 'B';
constexpr auto PUZ_HORZ = '-';
constexpr auto PUZ_VERT = '|';
constexpr auto PUZ_BOUNDARY = '`';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_path
{
    vector<Position> m_path;
    char m_ch_path = ' ';
};
struct puz_move
{
    Position m_cup;
    set<Position> m_objects;
    vector<puz_path> m_paths;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : map<Position, set<Position>>
{
    puz_state2(const puz_game* g, const Position& p)
        : m_game(g), m_pos2dirs{{p, {0, 1, 2, 3}}} {}

    void make_move(const Position& p, int i, const Position& p2,
        const puz_path& path, const map<Position, vector<int>>& used_pos2dirs);
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    bool m_need_milk = true;
    map<Position, vector<int>> m_pos2dirs;
    vector<puz_path> m_paths;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& [p, dirs] : m_pos2dirs) {
        // 4. When linking multiple beans and cows, you can also link cows to cows and
        //    beans to beans, other than linking them to the cup.
        if (char ch = m_game->cells(p);
            m_need_milk && ch == PUZ_BEAN ||
            !m_need_milk && ch == PUZ_MILK) continue;
        map<Position, vector<int>> used_pos2dirs;
        for (int i : dirs) {
            vector<Position> path;
            char ch_path = i % 2 == 0 ? PUZ_VERT : PUZ_HORZ;
            auto& os = offset[i];
            if (auto p2 = p + os; [&] {
                for (;; p2 += os)
                    switch (m_game->cells(p2)) {
                    case PUZ_SPACE:
                        // 2. Links must be straight lines, not crossing each other.
                        if (boost::algorithm::any_of(m_paths, [&](const puz_path& o) {
                            return boost::algorithm::any_of_equal(o.m_path, p2);
                        }))
                            return used_pos2dirs[p].push_back(i), false;
                        else {
                            path.push_back(p2);
                            break;
                        }
                    case PUZ_MILK:
                        return m_need_milk;
                    case PUZ_BEAN:
                        return !m_need_milk;
                    default:
                        return used_pos2dirs[p].push_back(i), false;
                    }
            }() && !m_pos2dirs.contains(p2) && !(contains(p) && at(p).contains(p2)))
                used_pos2dirs[p].push_back(i),
                children.emplace_back(*this).make_move(p, i, p2, {path, ch_path}, used_pos2dirs);
        }
    }
}

void puz_state2::make_move(const Position& p, int i, const Position& p2,
    const puz_path& path, const map<Position, vector<int>>& used_pos2dirs)
{
    (*this)[p].insert(p2);
    for (auto& [p3, dirs] : used_pos2dirs)
        for (int j : dirs)
            boost::remove_erase(m_pos2dirs[p3], j);
    vector<int> v{0, 1, 2, 3};
    boost::remove_erase(v, (i + 2) % 4);
    m_pos2dirs.emplace(p2, v);
    m_paths.push_back(path);
    m_need_milk = !m_need_milk;
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() * 2 + 1)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    vector<Position> cups;
    for (int r = 1; ; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; ; ++c) {
            char ch = str[c - 1];
            m_cells.push_back(ch);
            Position p(r * 2 - 1, c * 2 - 1);
            if (ch == PUZ_CUP)
                cups.push_back(p);
            if (ch != PUZ_SPACE)
                m_pos2move_ids[p];
            if (c == m_sidelen / 2) break;
            m_cells.push_back(PUZ_SPACE);
        }
        m_cells.push_back(PUZ_BOUNDARY);
        if (r == m_sidelen / 2) break;
        m_cells.push_back(PUZ_BOUNDARY);
        m_cells.append(m_sidelen - 2, PUZ_SPACE);
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& p : cups) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p});
        for (auto& s: smoves) {
            // 3. To each cup there must be linked an equal number of beans and cows. At
            //    least one of each.
            if (s.empty() || !s.m_need_milk) // s.size() should be even
                continue;
            set<Position> objects{p};
            for (auto& [_1, rng] : s)
                for (auto& p2 : rng)
                    objects.insert(p2);
            int n = m_moves.size();
            m_moves.emplace_back(p, objects, s.m_paths);
            m_pos2move_ids[p].push_back(n);
            for (auto& [_1, rng] : s)
                for (auto& p2 : rng)
                    m_pos2move_ids[p2].push_back(n);
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int n);
    void make_move2(int n);
    int find_matches(bool init);
    bool is_interconnected() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_cells(g.m_cells), m_game(&g), m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_2, objects, paths] = m_game->m_moves[id];
            // 2. Links must be straight lines, not crossing each other.
            return !boost::algorithm::all_of(paths, [&](const puz_path& o) {
                return boost::algorithm::all_of(o.m_path, [&](const Position& p2) {
                    return cells(p2) == PUZ_SPACE;
                });
            }) || !boost::algorithm::all_of(objects, [&](const Position& p2) {
                return m_matches.contains(p2);
            });
        });
        if (!init)
            switch(move_ids.size()) {
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
    auto& [_1, objects, paths] = m_game->m_moves[n];
    for (auto& [path, ch] : paths)
        for (auto& p : path)
            cells(p) = ch;
    for (auto& p : objects)
        ++m_distance, m_matches.erase(p);
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
    for (int n : move_ids)
        if (!children.emplace_back(*this).make_move(n))
            children.pop_back();
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
