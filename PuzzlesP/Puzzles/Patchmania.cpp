#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Patchmania
*/

namespace puzzles::Patchmania{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_HOLE = 'O';
constexpr auto PUZ_MUSHROOM = 'm';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

constexpr string_view dirs = "^>v<o";

struct puz_bunny_info
{
    char m_bunny_name, m_food_name;
	Position m_bunny;
	set<Position> m_food;
    bool m_teleported = false;
    int steps() const { return m_food.size() + 1; }
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_start;
    map<char, puz_bunny_info> m_bunny2info;
    set<Position> m_holes, m_mushrooms;
    set<Position> m_horz_walls, m_vert_walls;
    map<Position, pair<char, Position>> m_teleports;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
	, m_size(strs.size() / 2, strs[0].size() / 2)
{
    vector<vector<Position>> teleports;
    for (int r = 0;; ++r) {
        // horizontal walls
        auto& str_h = strs[r * 2];
        for (int c = 0; c < cols(); ++c)
            if (str_h[c * 2 + 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == rows()) break;
        auto& str_v = strs[r * 2 + 1];
        for (int c = 0;; ++c) {
            Position p(r, c);
            if (str_v[c * 2] == '|')
                m_vert_walls.insert(p);
            if (c == cols()) break;
            char ch = str_v[c * 2 + 1];
            m_start.push_back(ch);
            switch(ch) {
            case 'C': // Calvin
            case 'T': // Otto
            case 'P': // Peanut
            case 'D': // Daisy
            {
                auto& info = m_bunny2info[ch];
                info.m_bunny_name = ch;
                info.m_bunny = p;
                break;
            }
            case 'c':
            case 't':
            case 'p':
            case 'd':
            {
                auto& info = m_bunny2info[toupper(ch)];
                info.m_food_name = ch;
                info.m_food.insert(p);
                break;
            }
            case '1':
            case '2':
            case '3':
            case '4':
            {
                int n = ch - '0';
                if (teleports.size() < n) teleports.resize(n);
                teleports[n - 1].push_back(p);
                m_start.back() = PUZ_SPACE;
                break;
            }
            case PUZ_MUSHROOM:
                m_mushrooms.insert(p);
        		break;
            case PUZ_HOLE:
            	m_holes.insert(p);
        		break;
            }
        }
    }
    for (int i = 0; i < teleports.size(); ++i) {
        auto& v = teleports[i];
        for (int j = 0; j < 2; ++j)
            m_teleports[v[j]] = {i + '1', v[1 - j]};
    }
}

struct puz_step : vector<Position> { using vector::vector; };

ostream & operator<<(ostream &out, const puz_step &mi)
{
    if (!mi.empty()) {
        out << "move: ";
        for (int i = 0; i < mi.size(); ++i) {
            auto& p = mi[i];
            out << format("({},{})", p.first + 1, p.second + 1);
            if (i < mi.size() - 1)
                out << " -> ";
        }
        println(out);
    }
    return out;
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool is_teleport(const Position& p) const { return m_game->m_teleports.contains(p); }
    const pair<char, Position>& get_teleport(const Position& p) const {
        return m_game->m_teleports.at(p);
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const { 
        return m_cells < x.m_cells;
    }
    bool make_move(const Position& p1, const Position& p2, bool teleported);

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { 
        return boost::accumulate(m_bunny2info, 0, [](int acc, const pair<const char, puz_bunny_info>& kv) {
            return acc + kv.second.steps();
        }) + m_mushrooms.size();
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out, const map<Position, char>& pos2dir, const puz_step& move) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out, {}, {});
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<char, puz_bunny_info> m_bunny2info;
    set<Position> m_mushrooms;
    char m_curr_bunny = 0;
    Position m_last_teleport{-1, -1};
    unsigned int m_distance = 0;
    puz_step m_move;
};

struct puz_state2 : pair<Position, bool>
{
    puz_state2(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info) { make_move(info.m_bunny, info.m_teleported); }

    void make_move(const Position& p, bool teleported) {
        first = p, second = teleported;
    }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    char ch = m_state->cells(first);
    if (ch == m_info->m_food_name || ch == PUZ_MUSHROOM || second && ch == PUZ_SPACE) return;
    if (!second && m_state->is_teleport(first)) {
        children.push_back(*this);
        children.back().make_move(m_state->get_teleport(first).second, true);
    } else
        for (int i = 0; i < 4; ++i) {
            auto p = first + offset[i];
            auto p_wall = first + offset2[i];
            auto& walls = i % 2 == 0 ? m_state->m_game->m_horz_walls : m_state->m_game->m_vert_walls;
            if (walls.contains(p_wall)) continue;
            ch = m_state->cells(p);
            if (ch == m_info->m_food_name || ch == PUZ_MUSHROOM || ch == PUZ_SPACE &&
                (!m_state->is_teleport(p) || m_state->cells(m_state->get_teleport(p).second) == PUZ_SPACE)) {
                children.push_back(*this);
                children.back().make_move(p, false);
            }
        }
}

struct puz_state3 : pair<Position, bool>
{
    puz_state3(const puz_state& s, const puz_bunny_info& info)
        : m_state(&s), m_info(&info), m_last_teleport(s.m_last_teleport) {
        make_move(info.m_bunny, info.m_teleported);
    }

    void make_move(const Position& p, bool teleported) {
        first = p;
        if (second = teleported) m_last_teleport = p;
    }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
    const puz_bunny_info* m_info;
    Position m_last_teleport;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    char ch = m_state->cells(first);
    if (ch == PUZ_HOLE) return;
    if (!second && m_state->is_teleport(first)) {
        children.push_back(*this);
        children.back().make_move(m_state->get_teleport(first).second, true);
    } else
        for (int i = 0; i < 4; ++i) {
            auto p = first + offset[i];
            auto p_wall = first + offset2[i];
            auto& walls = i % 2 == 0 ? m_state->m_game->m_horz_walls : m_state->m_game->m_vert_walls;
            if (walls.contains(p_wall)) continue;
            ch = m_state->cells(p);
            if (ch == PUZ_HOLE || ch == m_info->m_food_name || ch == PUZ_MUSHROOM ||
                m_state->is_teleport(p) && p != m_last_teleport &&
                m_state->cells(m_state->get_teleport(p).second) == PUZ_SPACE) {
                children.push_back(*this);
                children.back().make_move(p, false);
            }
        }
}

struct puz_state4 : pair<Position, bool>
{
    puz_state4() {}
    puz_state4(const puz_state& s, const puz_bunny_info& info, const Position& dest)
        : m_state(&s), m_info(&info), m_dest(&dest) { make_move(info.m_bunny, info.m_teleported); }

    void make_move(const Position& p, bool teleported) {
        first = p, second = teleported;
    }
    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state4>& children) const;
    unsigned int get_heuristic() const { return manhattan_distance(first, *m_dest); }
    unsigned int get_distance(const puz_state4& child) const { return 1; }
    void dump_move(ostream& out) const {}

    const puz_state* m_state = nullptr;
    const puz_bunny_info* m_info = nullptr;
    const Position* m_dest = nullptr;
};

void puz_state4::gen_children(list<puz_state4>& children) const
{
    if (second && first != m_info->m_bunny) return;
    if (!second && m_state->is_teleport(first)) {
        children.push_back(*this);
        children.back().make_move(m_state->get_teleport(first).second, true);
    } else
        for (int i = 0; i < 4; ++i) {
            auto p = first + offset[i];
            auto p_wall = first + offset2[i];
            auto& walls = i % 2 == 0 ? m_state->m_game->m_horz_walls : m_state->m_game->m_vert_walls;
            if (walls.contains(p_wall)) continue;
            if (p == *m_dest && !m_state->is_teleport(p) || m_state->cells(p) == PUZ_SPACE &&
                (!m_state->is_teleport(p) || m_state->get_teleport(p).second == *m_dest)) {
                children.push_back(*this);
                children.back().make_move(p, false);
            }
        }
}

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
, m_bunny2info(g.m_bunny2info), m_mushrooms(g.m_mushrooms)
{
}
    
bool puz_state::make_move(const Position& p1, const Position& p2, bool teleported)
{
    char &ch1 = cells(p1), &ch2 = cells(p2);
    auto& info = m_bunny2info.at(ch1);
    if (ch2 == PUZ_HOLE) {
        if (!info.m_food.empty())
            return false;
        m_bunny2info.erase(ch1);
        if (m_bunny2info.empty() && !m_mushrooms.empty())
            return false;
        ch1 = PUZ_SPACE;
        m_curr_bunny = 0;
        m_move = {p1, p2};
    } else {
        if (m_curr_bunny == 0) {
            puz_state4 sstart(*this, info, p2);
            list<list<puz_state4>> spaths;
            puz_solver_astar<puz_state4>::find_solution(sstart, spaths);
            m_move.clear();
            for (auto& kv : spaths.front())
                m_move.push_back(kv.first);
        } else
            m_move = {p1, p2};
        (ch2 == PUZ_MUSHROOM ? m_mushrooms : info.m_food).erase(info.m_bunny = p2);
        ch2 = exchange(ch1, PUZ_SPACE);
        m_curr_bunny = m_curr_bunny == 0 && teleported ? 0 : ch2;
    }
    
    m_distance = 1;
    if (info.m_teleported = teleported) m_last_teleport = p2;

    // pruning
    if (m_curr_bunny != 0) {
        auto& info = m_bunny2info.at(m_curr_bunny);
        auto smoves = puz_move_generator<puz_state3>::gen_moves({*this, info});
        // 1. The bunny must reach one of the holes after taking all its own food
        //    and/or some mushrooms.
        // 2. The bunny must take all mushrooms if he is the last bunny.
        if (boost::algorithm::none_of(smoves, [&](const puz_state3& kv) {
            return cells(kv.first) == PUZ_HOLE;
        }) || boost::count_if(smoves, [&](const puz_state3& kv) {
            return cells(kv.first) == info.m_food_name;
        }) != info.m_food.size() || m_bunny2info.size() == 1 &&
            boost::count_if(smoves, [&](const puz_state3& kv) {
            return cells(kv.first) == PUZ_MUSHROOM;
        }) != m_mushrooms.size())
            return false;
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (m_curr_bunny == 0)
        for (auto& [bunny, info] : m_bunny2info) {
            auto smoves = puz_move_generator<puz_state2>::gen_moves({*this, info});
            smoves.remove_if([&](const puz_state2& kv) {
                char ch = cells(kv.first);
                return !(ch == info.m_food_name || ch == PUZ_MUSHROOM || kv.second && kv.first != info.m_bunny);
            });
            for (auto& kv : smoves) {
                children.push_back(*this);
                if (!children.back().make_move(info.m_bunny, kv.first, kv.second))
                    children.pop_back();
            }
        }
    else {
        auto& info = m_bunny2info.at(m_curr_bunny);
        auto& p1 = info.m_bunny;
        if (!info.m_teleported && is_teleport(p1)) {
            children.push_back(*this);
            if (!children.back().make_move(p1, get_teleport(p1).second, true))
                children.pop_back();
        } else
            for (int i = 0; i < 4; ++i) {
                auto p_wall = p1 + offset2[i];
                auto& walls = i % 2 == 0 ? m_game->m_horz_walls : m_game->m_vert_walls;
                if (walls.contains(p_wall)) continue;
                auto p2 = p1 + offset[i];
                char ch = cells(p2);
                if (ch == PUZ_HOLE || ch == info.m_food_name || ch == PUZ_MUSHROOM ||
                    is_teleport(p2) && p2 != m_last_teleport) {
                    children.push_back(*this);
                    if (!children.back().make_move(p1, p2, false))
                        children.pop_back();
                }
            }
    }
}

ostream& puz_state::dump(ostream& out, const map<Position, char>& pos2dir, const puz_step& move) const
{
    out << move;
    for (int c = 0; c < cols(); ++c)
        out << format("{:3}", c + 1);
    println(out);
    for (int r = 0;; ++r) {
        out << ' ';
        // draw horizontal lines
        for (int c = 0; c < cols(); ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " --" : "   ");
        println(out);
        if (r == rows()) break;
        out << (r + 1);
        for (int c = 0;; ++c) {
            Position p(r, c);
            // draw vertical lines
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == cols()) break;
            char ch = cells(p);
            out << (ch != PUZ_SPACE ? ch : is_teleport(p) ? get_teleport(p).first : ch)
                << (pos2dir.contains(p) ? pos2dir.at(p) : ' ');
        }
        out << (r + 1) << endl;
    }
    for (int c = 0; c < cols(); ++c)
        out << format("{:3}", c + 1);
    println(out);
    return out;
}

void dump_all(ostream& out, const list<puz_state>& spath)
{
    map<Position, char> pos2dir;
    puz_step move(2);
    for (auto it_last = spath.cbegin(), it = next(it_last); it != spath.cend(); it++) {
        auto& m = it->m_move;
        if (pos2dir.empty()) move[0] = m[0];
        for (int i = 0; i < m.size() - 1; ++i)
            pos2dir[m[i]] = dirs[boost::find(offset, m[i + 1] - m[i]) - offset];
        if (it->m_curr_bunny == 0 &&
            (next(it) == spath.end() || next(it)->m_move[0] != m.back())) {
            pos2dir[move[1] = m.back()] = '#';
            it_last->dump(out, pos2dir, move);
            pos2dir.clear();
            it_last = it;
        }
    }
}

}

void solve_puz_Patchmania()
{
    using namespace puzzles::Patchmania;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Patchmania.xml", "Puzzles/Patchmania.txt", solution_format::CUSTOM_STATES, dump_all);
}
