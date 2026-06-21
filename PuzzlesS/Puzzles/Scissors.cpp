#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 1/Scissors

    Summary
    Tailor's puzzle

    Description
    1. Cut the board into patches.
    2. Each patch should contain the numbers 1 to N exactly once (N being the highest number on the board).
    3. Each patch should end on the border.
*/

namespace puzzles::Scissors{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BACK_SLASH = '\\';
constexpr auto PUZ_FRONT_SLASH = '/';
    
constexpr auto offset = Position::Directions4;
constexpr auto offset2 = Position::Square2x2Offset;
constexpr Position offset3[] = {
    {-1, -1},
    {-1, 1},
    {1, -1},
    {1, 1},
};

using puz_slash = pair<Position, Position>;

// first: position of the cell/triangle
// second: 15 if the cell contains no slash
// second: 3 or 12 if the cell contains a back slash
// second: 6 or 9 if the cell contains a front slash
using puz_position = pair<Position, int>;
using puz_slash_char = pair<Position, char>;
using puz_move = vector<puz_slash_char>;

void add_slash(set<puz_position>& positions, const Position& p, char ch)
{
    positions.erase(puz_position(p, 15));
    if (ch == PUZ_BACK_SLASH)
        positions.emplace(p, 3), positions.emplace(p, 12);
    else
        positions.emplace(p, 6), positions.emplace(p, 9);
}

struct puz_state2 : puz_position
{
    puz_state2(const set<puz_position>* positions)
        : m_positions(positions) { make_move(*positions->begin()); }

    void make_move(const puz_position& kv) { static_cast<puz_position&>(*this) = kv; }
    void gen_children(list<puz_state2>& children) const;

    const set<puz_position>* m_positions;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    auto& [p, n] = static_cast<const puz_position&>(*this);
    for (int i = 0; i < 4; ++i)
        if (n & (1 << i)) {
            auto p2 = p + offset[i];
            int j = (i + 2) % 4;
            for (auto& kv : *m_positions)
                if (auto& [p3, n3] = kv; p3 == p2 && n3 & (1 << j)) {
                    children.emplace_back(*this).make_move(kv);
                    break;
                }
        }
}

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_cells;
    char m_max_num = '1';
    set<puz_slash> m_slashes;
    vector<puz_move> m_moves;
    map<Position, vector<int>> m_pos2move_ids;
    set<puz_position> m_positions;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_cells[p.first * m_sidelen + p.second]; }
    bool is_border_dot(const Position& p) const {
        return p.first == 0 || p.second == 0 || p.first == m_sidelen || p.second == m_sidelen;
    };
    pair<bool, vector<Position>> check_move(const list<puz_state2>& smoves) const;
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            m_cells.push_back(ch);
            m_max_num = max(m_max_num, ch);
            m_positions.emplace(p, 15);
            if (ch == PUZ_SPACE) {
                // back slash
                m_slashes.emplace(p + offset2[0], p + offset2[3]);
                // front slash
                m_slashes.emplace(p + offset2[1], p + offset2[2]);
            }
        }
    }

    set<Position> border;
    for (int i = 0; i <= m_sidelen; ++i) {
        auto f = [&](int r, int c) {
            if (r == m_sidelen && c == m_sidelen) return;
            Position p0(r, c);
            if (!border.insert(p0).second) return;
            puz_move move;
            set dots{p0};
            auto dfs = [&](this const auto& self, const Position& p1) {
                if (p1 != p0 && is_border_dot(p1)) {
                    if (p1 > p0) {
                        auto positions = m_positions;
                        for (auto& [p, ch] : move)
                            add_slash(positions, p, ch);
                        while (!positions.empty()) {
                            auto smoves = puz_move_generator<puz_state2>::gen_moves(&positions);
                            auto [b, rng] = check_move(smoves);
                            if (!b) break;
                            if (rng.size() == 1) {
                                m_pos2move_ids[rng[0]].push_back(m_moves.size());
                                m_moves.push_back(move);
                                break;
                            }
                            for (auto& s : smoves)
                                positions.erase(s);
                        }
                    }
                    return;
                }
                for (auto& os : offset3) {
                    auto p2 = p1 + os;
                    if (dots.contains(p2)) continue;
                    auto p3 = min(p1, p2), p4 = max(p1, p2);
                    if (!m_slashes.contains({p3, p4})) continue;
                    auto& [r1, c1] = p3;
                    auto& [r2, c2] = p4;
                    int r0 = min(r1, r2), c0 = min(c1, c2);
                    move.emplace_back(Position(r0, c0), r0 == r1 && c0 == c1 ? PUZ_BACK_SLASH : PUZ_FRONT_SLASH);
                    dots.insert(p2);
                    self(p2);
                    move.pop_back();
                    dots.erase(p2);
                }
            };
            dfs(p0);
        };
        // check all dots on the border
        f(0, i), f(m_sidelen, i), f(i, 0), f(i, m_sidelen);
    }
}

pair<bool, vector<Position>> puz_game::check_move(const list<puz_state2>& smoves) const
{
    vector<char> nums;
    vector<Position> rng;
    for (auto& s : smoves) {
        auto& [p, n] = static_cast<const puz_position&>(s);
        if (n != 15) continue;
        char ch = cells(p);
        if (ch == PUZ_SPACE) continue;
        if (ch == '1') rng.push_back(p);
        nums.push_back(ch);
    }
    int sz = nums.size(), max_num = m_max_num - '0';
    if (sz == 0 || sz % max_num != 0)
        return {false, rng};
    int cnt = sz / max_num;
    boost::sort(nums);
    for (int i = 0; i < sz; ++i)
        if (nums[i] != (i / cnt + '1'))
            return {false, rng};
    return {true, rng};
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool check_cuts() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    string m_cells;
    set<puz_position> m_positions;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_cells)
, m_positions(g.m_positions)
, m_matches(g.m_pos2move_ids)
{
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, move_ids] : m_matches) {
        boost::remove_erase_if(move_ids, [&](int id) {
            return !boost::algorithm::all_of(m_game->m_moves[id], [&](const puz_slash_char& slash_char2) {
                auto& [p2, ch2] = slash_char2;
                char ch = cells(p2);
                return ch == PUZ_SPACE || ch == ch2;
            });
        });

        if (!init)
            switch(move_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, move_ids[0]), 1;
            }
    }
    return check_cuts() ? 2 : 0;
}

bool puz_state::check_cuts() const
{
    auto positions = m_positions;
    vector<int> v;
    while (!positions.empty()) {
        auto smoves = puz_move_generator<puz_state2>::gen_moves(&positions);
        auto [b, rng] = m_game->check_move(smoves);
        if (!b) return false;
        v.push_back(rng.size());
        for (auto& s : smoves)
            positions.erase(s);
    }
    return !is_goal_state() || boost::algorithm::all_of_equal(v, 1);
}

void puz_state::make_move2(const Position& p, int n)
{
    for (auto& [p2, ch2] : m_game->m_moves[n])
        if (char& ch = cells(p2); ch == PUZ_SPACE)
            add_slash(m_positions, p2, ch = ch2);
    ++m_distance, m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, m_move_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (int n : m_move_ids)
        if (!children.emplace_back(*this).make_move(p, n))
            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            out << format("{:<2}", (ch == PUZ_SPACE ? '.' : ch));
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Scissors()
{
    using namespace puzzles::Scissors;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Scissors.xml", "Puzzles/Scissors.txt", solution_format::GOAL_STATE_ONLY);
}
