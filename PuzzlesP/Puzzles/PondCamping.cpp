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

namespace puzzles::PondCamping{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FOREST = '=';
constexpr auto PUZ_POND = 'P';

constexpr array<Position, 4> offset = Position::Directions4;

struct puz_move
{
    Position m_p_hint;
    set<Position> m_empties;
    set<Position> m_forests;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num, m_pos2num_big;
    string m_cells;
    // key: position of the number (hint)
    map<Position, vector<puz_move>> m_pos2moves;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p, int num)
        : m_game(game), m_num(num) { make_move(p); }

    bool is_goal_state() const { return size() == m_num + 1; }
    void make_move(const Position& p) { insert(p); }
    void gen_children(list<puz_state2>& children) const;

    const puz_game* m_game;
    int m_num;
};

void puz_state2::gen_children(list<puz_state2>& children) const {
    if (is_goal_state())
        return;
    for (auto& p : *this)
        for (auto& os : offset)
            // 1. The numbers are Ponds.From each Pond you can have a hike of that many
            //     tiles as the number marked on it.
            if (auto p2 = p + os;
                !contains(p2) && m_game->cells(p2) == PUZ_SPACE)
                children.emplace_back(*this).make_move(p2);
}

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
                int num = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                (num > 10 ? m_pos2num_big : m_pos2num)[{r, c}] = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
            }
        m_cells.push_back(PUZ_FOREST);
    }
    m_cells.append(m_sidelen, PUZ_FOREST);

    for (auto& [p, num] : m_pos2num) {
        auto& moves = m_pos2moves[p];
        auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p, num});
        for (auto& s : smoves)
            if (s.is_goal_state()) {
                auto& [p_hint, empties, forests] = moves.emplace_back();
                p_hint = p, (empties = s).erase(p);
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os;
                            !s.contains(p3) && cells(p3) == PUZ_SPACE)
                            forests.insert(p3);
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
        return tie(m_cells, m_matches, m_moves_big) < tie(x.m_cells, x.m_matches, x.m_moves_big);
    }
    bool make_move_hike(const Position& p, int n);
    void make_move_hike2(const Position& p, int n);
    void make_move_forest();
    void make_move_big();
    int find_matches(bool init);
    bool check_ponds() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size() + (m_is_phase_big ? 0 : m_game->m_pos2num_big.size());
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    map<Position, vector<int>> m_matches;
    shared_ptr<vector<puz_move>> m_moves_big;
    unsigned int m_distance = 0;
    bool m_is_phase_big = false;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    for (auto& [p, moves] : g.m_pos2moves) {
        auto& move_ids = m_matches[p];
        move_ids.resize(moves.size());
        boost::iota(move_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        auto& moves = m_is_phase_big ? *m_moves_big : m_game->m_pos2moves.at(p);
        boost::remove_erase_if(move_ids, [&](int id) {
            auto& [_1, empties, forests] = moves[id];
            return !boost::algorithm::all_of(empties, [&](const Position& p2) {
                char ch = cells(p2);
                return ch == PUZ_SPACE || ch == PUZ_EMPTY;
            }) || !boost::algorithm::all_of(forests, [&](const Position& p2) {
                char ch = cells(p2);
                return ch == PUZ_SPACE || ch == PUZ_FOREST;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hike2(p, move_ids.front()), 1;
            }
    }
    return check_ponds() ? 2 : 0;
}

struct puz_state3 : Position
{
    puz_state3(const puz_state* s, const Position& p) : m_state(s) { make_move(p); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (char ch = m_state->cells(p2); ch == PUZ_SPACE || ch == PUZ_EMPTY)
            children.emplace_back(*this).make_move(p2);
    }
}

bool puz_state::check_ponds() const
{
    if (!m_is_phase_big) {
        for (auto& [p, num] : m_game->m_pos2num_big) {
            auto smoves = puz_move_generator<puz_state3>::gen_moves({this, p});
            if (smoves.size() < num + 1)
                return false;
        }
    }
    return true;
}

void puz_state::make_move_hike2(const Position& p, int n)
{
    auto& moves = m_is_phase_big ? *m_moves_big : m_game->m_pos2moves.at(p);
    auto& [_1, empties, forests] = moves[n];
    for (auto& p2 : empties)
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = PUZ_EMPTY, ++m_distance;
    for (auto& p2 : forests)
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            ch = PUZ_FOREST, ++m_distance;
    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move_hike(const Position& p, int n)
{
    m_distance = 0;
    make_move_hike2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

struct puz_state5 : set<Position>
{
    puz_state5(const puz_state* state, int num, const Position& p)
        : m_state(state), m_num(num) { make_move(p); }
    void make_move(const Position& p);

    void gen_children(list<puz_state5>& children) const;

    const puz_state* m_state;
    int m_num;
    bool m_is_goal;
};

void puz_state5::make_move(const Position& p)
{
    insert(p);
    m_is_goal = size() == m_num + 1 && boost::algorithm::none_of(*this, [&](const Position& p2) {
        return boost::algorithm::any_of(offset, [&](const Position& os) {
            auto p3 = p2 + os;
            return !contains(p3) && m_state->cells(p3) == PUZ_EMPTY;
        });
    });
}

void puz_state5::gen_children(list<puz_state5>& children) const {
    if (size() == m_num + 1)
        return;
    for (auto& p : *this)
        for (auto& os : offset) {
            // 1. The numbers are Ponds.From each Pond you can have a hike of that many
            //     tiles as the number marked on it.
            auto p2 = p + os;
            if (char ch2 = m_state->cells(p2);
                (ch2 == PUZ_SPACE || ch2 == PUZ_EMPTY) && !contains(p2))
                children.emplace_back(*this).make_move(p2);
        }
}

void puz_state::make_move_big()
{
    m_moves_big = make_shared<vector<puz_move>>();
    for (auto& [p, num] : m_game->m_pos2num_big) {
        m_matches[p];
        auto smoves = puz_move_generator<puz_state5>::gen_moves({this, num, p});
        for (auto& s : smoves)
            if (s.m_is_goal) {
                m_matches[p].push_back(m_moves_big->size());
                auto& [p_hint, empties, forests] = m_moves_big->emplace_back();
                p_hint = p, (empties = s).erase(p);
                for (auto& p2 : s)
                    for (auto& os : offset)
                        if (auto p3 = p2 + os;
                            !s.contains(p3) && cells(p3) == PUZ_SPACE)
                            forests.insert(p3);
            }
    }
    m_is_phase_big = true;
}

void puz_state::make_move_forest()
{
    for (char& ch : m_cells)
        if (ch == PUZ_SPACE)
            ch = PUZ_FOREST, ++m_distance;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, move_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int n : move_ids)
            if (!children.emplace_back(*this).make_move_hike(p, n))
                children.pop_back();
    } else
        !m_is_phase_big ? children.emplace_back(*this).make_move_big() :
        children.emplace_back(*this).make_move_forest();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (auto it = m_game->m_pos2num.find(p); it != m_game->m_pos2num.end())
                out << format("{:<2}", it->second);
            else if (auto it = m_game->m_pos2num_big.find(p); it != m_game->m_pos2num_big.end())
                out << format("{:<2}", it->second);
            else
                out << cells(p) << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_PondCamping()
{
    using namespace puzzles::PondCamping;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PondCamping.xml", "Puzzles/PondCamping.txt", solution_format::GOAL_STATE_ONLY);
}
