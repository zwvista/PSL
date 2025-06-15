#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 16/Carpenter's Square

    Summary
    Angled Borders

    Description
    1. Similar to Carpenter's Wall, this time you have to respect the same
       rules, but instead of forming a Nurikabe, you just have to divide the
       board into many.Capenter's Squares (L shaped tools) of different size.
    2. The circled numbers on the board indicate the corner of the L.
    3. When a number is inside the circle, that indicates the total number of
       squares occupied by the L.
    4. The arrow always sits at the end of an arm and points to the corner of
       an L.
    5. All the tiles in the board have to be part of a Carpenter's Square.
*/

namespace puzzles::CarpentersSquare{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_CORNER = 'O';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const vector<vector<int>> tool_dirs2 = {
    {0, 1}, {0, 3}, {1, 2}, {2, 3}
};

constexpr string_view tool_dirs = "^>v<";

enum class tool_hint_type {
    CORNER,
    NUMBER,
    ARM_END
};

struct puz_tool
{
    Position m_hint_pos;
    char m_hint = PUZ_CORNER;
    puz_tool() {}
    puz_tool(const Position& p, char ch) : m_hint_pos(p), m_hint(ch) {}
    tool_hint_type hint_type() const {
        return m_hint == PUZ_CORNER ? tool_hint_type::CORNER :
            dir() != -1 ? tool_hint_type::ARM_END : tool_hint_type::NUMBER;
    }
    int len() const { return isdigit(m_hint) ? m_hint - '0' : m_hint - 'A' + 10; }
    int dir() const { return tool_dirs.find(m_hint); }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2ch;
    map<char, puz_tool> m_ch2tool;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    char ch_t = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else {
                Position p(r, c);
                m_pos2ch[p] = ch;
                m_ch2tool[ch_t] = {p, ch};
                m_start.push_back(ch_t++);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(char ch, int n);
    bool make_move2(char ch, int n);
    bool make_move_hidden(char ch, int n);
    int adjust_area(bool init);
    const puz_tool& get_tool(char ch) const {
        auto it = m_game->m_ch2tool.find(ch);
        return it == m_game->m_ch2tool.end() ? m_next_tool : it->second;
    }
    void add_tool(const Position& p) {
        m_next_tool.m_hint_pos = p;
        m_matches[m_next_ch];
        adjust_area(true);
    }

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0 && find(PUZ_SPACE) == string::npos;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<char, vector<vector<Position>>> m_matches;
    puz_tool m_next_tool;
    char m_next_ch;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_next_ch('a' + g.m_ch2tool.size())
{
    for (auto& [ch, tool] : g.m_ch2tool)
        m_matches[ch];

    adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
    for (auto& [ch, ranges] : m_matches) {
        ranges.clear();
        auto& t = get_tool(ch);
        auto& p = t.m_hint_pos;

        auto f = [&](const vector<Position>& a0, const vector<Position>& a1, int i, int j) {
            vector<Position> rng;
            for (int k = 0; k < i; ++k)
                rng.push_back(a0[k]);
            for (int k = 0; k < j; ++k)
                rng.push_back(a1[k]);
            rng.push_back(p);
            boost::sort(rng);
            ranges.push_back(rng);
        };

        // start from an arm corner
        auto g1 = [&](int len) {
            vector<vector<Position>> arms(4);
            vector<vector<int>> arm_lens(4);
            Position p2;
            char ch2;
            for (int i = 0; i < 4; ++i) {
                auto& os = offset[i];
                int j = 1;
                for (p2 = p + os; (ch2 = cells(p2)) == PUZ_SPACE; p2 += os) {
                    arms[i].push_back(p2);
                    arm_lens[i].push_back(j++);
                }
                if (!m_matches.contains(ch2)) continue;
                auto& t = get_tool(ch2);
                if (t.hint_type() == tool_hint_type::ARM_END &&
                    (t.dir() + 2) % 4 == i) {
                    arms[i].push_back(p2);
                    arm_lens[i].push_back(j);
                }
            }
            for (auto& dirs : tool_dirs2) {
                auto &a0 = arms[dirs[0]], &a1 = arms[dirs[1]];
                auto &lens0 = arm_lens[dirs[0]], &lens1 = arm_lens[dirs[1]];
                if (a0.empty() || a1.empty()) continue;
                for (int i : lens0)
                    for (int j : lens1)
                        if (len == -1 || i + j + 1 == len)
                            f(a0, a1, i, j);
            }
        };

        // start from an arm end
        auto g2 = [&](int d) {
            vector<Position> a0, a1;
            auto& os = offset[d];
            int d21 = (d + 3) % 4, d22 = (d + 1) % 4;
            Position p2;
            int len = -1;
            char ch2;
            for (p2 = p + os; ; p2 += os) {
                ch2 = cells(p2);
                if (ch2 != PUZ_SPACE && !m_matches.contains(ch2)) break;
                if (ch2 == PUZ_SPACE)
                    a0.push_back(p2);
                else {
                    auto& t = get_tool(ch2);
                    auto ht = t.hint_type();
                    if (ht == tool_hint_type::ARM_END) break;
                    if (ht == tool_hint_type::NUMBER)
                        len = t.len();
                    a0.push_back(p2);
                }
                char ch_corner = cells(p2);
                int i = a0.size();
                for (int d2 : {d21, d22}) {
                    auto& os2 = offset[d2];
                    a1.clear();
                    Position p3;
                    cells(p2) = ch;
                    for (p3 = p2 + os2; (ch2 = cells(p3)) == PUZ_SPACE; p3 += os2)
                        a1.push_back(p3);
                    cells(p2) = ch_corner;
                    vector<int> arm_lens;
                    int j;
                    for (j = 1; j <= a1.size(); ++j)
                        arm_lens.push_back(j);
                    if (m_matches.contains(ch2)) {
                        auto& t = get_tool(ch2);
                        if (t.hint_type() == tool_hint_type::ARM_END &&
                            (t.dir() + 2) % 4 == d2) {
                            a1.push_back(p3);
                            arm_lens.push_back(++j);
                        }
                    }
                    if (a1.empty()) continue;
                    for (int j : arm_lens)
                        if (len == -1 || i + j + 1 == len)
                            f(a0, a1, i, j);
                }
            }
        };

        switch(t.hint_type()) {
        case tool_hint_type::CORNER:
            g1(-1);
            break;
        case tool_hint_type::NUMBER:
            g1(t.len());
            break;
        case tool_hint_type::ARM_END:
            g2(t.dir());
            break;
        }

        if (!init)
            switch(ranges.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(ch, 0) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(char ch, int n)
{
    auto& rng = m_matches.at(ch)[n];

    set<char> chars;
    for (auto& p : rng)
        chars.insert(cells(p));
    chars.erase(PUZ_SPACE);
    if (chars.empty())
        chars.insert(ch);

    for (auto& p : rng)
        cells(p) = ch;
    for (char ch2 : chars) {
        m_matches.erase(ch2);
        ++m_distance;
    }

    return true;
}

bool puz_state::make_move(char ch, int n)
{
    m_distance = 0;
    if (!make_move2(ch, n))
        return false;
    int m;
    while ((m = adjust_area(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_hidden(char ch, int n)
{
    if (!make_move2(ch, n))
        return false;
    m_next_ch++;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [ch, ranges] = *boost::min_element(m_matches, [](
            const pair<const char, vector<vector<Position>>>& kv1,
            const pair<const char, vector<vector<Position>>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });
        for (int i = 0; i < ranges.size(); ++i) {
            children.push_back(*this);
            if (!children.back().make_move(ch, i))
                children.pop_back();
        }
    } else {
        set<Position> rng;
        for (int r = 1; r < sidelen() - 1; ++r)
            for (int c = 1; c < sidelen() - 1; ++c) {
                Position p(r, c);
                char ch = cells(p);
                if (ch != PUZ_SPACE) continue;
                auto s = *this;
                s.add_tool(p);
                auto& [ch2, ranges] = *s.m_matches.begin();
                for (auto& rng2 : ranges)
                    rng.insert(rng2.begin(), rng2.end());
                for (int i = 0; i < ranges.size(); ++i) {
                    children.push_back(s);
                    if (!children.back().make_move_hidden(ch, i))
                        children.pop_back();
                }
            }

        // pruning
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horizontal walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (cells({r, c}) != cells({r - 1, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (cells({r, c}) != cells({r, c - 1}) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            auto it = m_game->m_pos2ch.find(p);
            if (it == m_game->m_pos2ch.end())
                out << ".";
            else
                out << it->second;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CarpentersSquare()
{
    using namespace puzzles::CarpentersSquare;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CarpentersSquare.xml", "Puzzles/CarpentersSquare.txt", solution_format::GOAL_STATE_ONLY);
}
