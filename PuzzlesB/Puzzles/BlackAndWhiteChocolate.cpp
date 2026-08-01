#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 2/Black and White Chocolate

    Summary
    Yummy !!

    Description
     1. Your chocolate factory made a mess. Instead of pouring dark and white chocolate
        in the neat usual rectangle shapes, everything got mixed up.
     2. Your brand policy is equality, so you have to sell chocolate bars with have
        equally dark and white chocolate in it.
     3. Divide the board in chocolate 'bars' that contain the same number of dark
        and white chocolate.
     4. Also the shape of the dark chocolate in an area must be the same as the white
        one, although it can be mirrored and/or rotated.
     5. The number on a square tells you how big is that dark or white shape.
        Obviosly in a single shape there must be the same number.
     6. A chocolate 'bar' can have any shape
     7. but it must contain equal number of dark and white squares
     8. which can be indicated on the squares themselves
     9. the shape of the dark squares must be the same of the white ones,
        possibly rotated or mirrored.
     10.Not every bar of dark/white chocolate is marked by numbers
     11.Big numbers indicate a big chocolate 'bar', so look for them first.
        For example a 6 indicates an area of 12 !
*/

namespace puzzles::BlackAndWhiteChocolate{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BLACK = 'B';
constexpr auto PUZ_WHITE = 'W';
constexpr auto PUZ_UNKNOWN = -1;

constexpr auto offset = Position::Directions4;

struct puz_move
{
    Position m_p_num;
    bool m_is_cloud = false;
    set<Position> m_rng;
    set<Position> m_clouds, m_empties;
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    vector<int> m_nums;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    int nums(const Position& p) const { return m_nums[p.first * cols() + p.second]; }
};

struct puz_state2 : set<Position>
{
    puz_state2(const puz_game* game, const Position& p)
        : m_game(game), m_p(p) { make_move(p); }

    bool is_goal_state() const {
        return m_num == PUZ_UNKNOWN || size() == m_num;
    }
    bool make_move(const Position& p);
    void gen_children(list<puz_state2>& children) const;
    unsigned int get_distance(const puz_state2& child) const { return 1; }

    const puz_game* m_game;
    int m_num = PUZ_UNKNOWN;
    Position m_p;
};

bool puz_state2::make_move(const Position& p)
{
    char ch = m_game->cells(p);
    int n = m_game->nums(p);
    if (!(ch == PUZ_WHITE && (m_num == PUZ_UNKNOWN || m_num == n)))
        return false;
    insert(p);
    m_num = n;
    return true;
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    if (size() == m_num)
        return;
    for (auto& p : *this)
        for (auto& os : offset)
            if (auto p2 = p + os;
                m_game->is_valid(p2) && !contains(p2) && p2 > m_p)
                if (!children.emplace_back(*this).make_move(p2))
                    children.pop_back();
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size(), strs[0].length() / 2)
{
    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c) {
            char ch1 = str[c * 2], ch2 = str[c * 2 + 1];
            m_cells.push_back(ch1);
            int n = ch2 == PUZ_SPACE ? PUZ_UNKNOWN : isdigit(ch2) ? ch2 - '0' : ch2 - 'A' + 10;
            m_nums.push_back(n);
        }
    }
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            if (cells(p) != PUZ_WHITE) continue;
            auto smoves = puz_move_generator<puz_state2>::gen_moves({this, p});
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < rows() && p.second >= 0 && p.second < cols();
    }
    char cells(const Position& p) const { return m_cells[p.first * cols() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * cols() + p.second]; }
    bool operator<(const puz_state& x) const {
        return tie(m_cells, m_matches) < tie(x.m_cells, x.m_matches);
    }
    bool make_move(int move_id);
    void make_move2(int move_id);
    int find_matches(bool init);

    //solve_puzzle interface
    // 6. The goal is to pick up every stone.
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
: m_game(&g)
, m_cells(rows()* cols(), PUZ_SPACE)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [_1, move_ids] : m_matches) {
//        boost::remove_erase_if(move_ids, [&](int id) {
//            auto& [_2, is_cloud, rng, clouds, empties] = m_game->m_moves[id];
//            return boost::algorithm::any_of(rng, [&](const Position& p2) {
//                char ch = cells(p2);
//                return ch != PUZ_SPACE && ch != (is_cloud ? PUZ_CLOUD : PUZ_EMPTY);
//            }) || boost::algorithm::any_of(clouds, [&](const Position& p2) {
//                char ch = cells(p2);
//                return ch != PUZ_SPACE && ch != PUZ_CLOUD;
//            }) || boost::algorithm::any_of(empties, [&](const Position& p2) {
//                char ch = cells(p2);
//                return ch != PUZ_SPACE && ch != PUZ_EMPTY;
//            });
//        });

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

void puz_state::make_move2(int move_id)
{
//    auto& [_1, is_cloud, rng, clouds, empties] = m_game->m_moves[move_id];
//    for (auto& p2 : rng)
//        cells(p2) = is_cloud ? PUZ_CLOUD : PUZ_EMPTY, ++m_distance, m_matches.erase(p2);
//    for (auto& p2 : clouds)
//        cells(p2) = PUZ_CLOUD;
//    for (auto& p2 : empties)
//        cells(p2) = PUZ_EMPTY;
}

bool puz_state::make_move(int move_id)
{
    m_distance = 0;
    make_move2(move_id);
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
        if (!children.emplace_back(*this).make_move(move_id))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            Position p(r, c);
            out << cells(p);
//            if (auto it = m_game->m_pos2num.find(p); it == m_game->m_pos2num.end())
//                out << ". ";
//            else
//                out << it->second << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_BlackAndWhiteChocolate()
{
    using namespace puzzles::BlackAndWhiteChocolate;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/BlackAndWhiteChocolate.xml", "Puzzles/BlackAndWhiteChocolate.txt", solution_format::GOAL_STATE_ONLY);
}
