#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 3/NumberLink

    Summary
    Connect the same numbers without the crossing paths

    Description
    1. Connect the couples of equal numbers (i.e. 2 with 2, 3 with 3 etc)
       with a continuous line.
    2. The line can only go horizontally or vertically and can't cross
       itself or other lines.
    3. Lines must originate on a number and must end in the other equal
       number.
    4. At the end of the puzzle, you must have covered ALL the squares with
       lines and no line can cover a 2*2 area (like a 180 degree turn).
    5. In other words you can't turn right and immediately right again. The
       same happens on the left, obviously. Be careful not to miss this rule.

    Variant
    6. In some levels there will be a note that tells you don't need to cover
       all the squares.
    7. In some levels you will have more than a couple of the same number.
       In these cases, you must connect ALL the same numbers together.
*/

namespace puzzles::NumberLink{

#define PUZ_SPACE            ' '
#define PUZ_NUMBER            'N'
#define PUZ_BOUNDARY        '+'
#define PUZ_LINE_OFF        '0'
#define PUZ_LINE_ON            '1'

const string lineseg_off = "0000";

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2num;
    map<char, vector<Position>> m_num2targets;
    vector<pair<char, int>> m_num2dist;
    bool m_no_board_fill;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
, m_no_board_fill(level.attribute("NoBoardFill").as_int() == 1)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            m_start.push_back(ch);
            if (ch != PUZ_SPACE) {
                Position p(r, c);
                m_pos2num[p] = ch;
                m_num2targets[ch].push_back(p);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for (auto& kv : m_num2targets) {
        auto& targets = kv.second;
        int sz = targets.size();
        unsigned int dist = 100;
        for (int i = 0; i < sz - 1; ++i)
            for (int j = i + 1; j < sz; ++j) {
                unsigned int d = manhattan_distance(targets[i], targets[j]);
                if (d < dist)
                    dist = d;
            }
        m_num2dist.emplace_back(kv.first, dist);
    }
    boost::sort(m_num2dist, [&](
        const pair<const char, int>& kv1,
        const pair<const char, int>& kv2) {
        return kv1.second < kv2.second;
    });
}

struct puz_state : vector<string>
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    string cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    string& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n);
    void new_link() {
        if (m_num2targets.empty()) return;
        auto& targets = m_num2targets.back().second;
        m_link = {targets.back()};
        targets.pop_back();
    }
    bool check_board() const;
    set<Position> get_area(char ch) const;

    //solve_puzzle interface
    bool is_goal_state() const {
        return m_num2targets.empty() && (m_game->m_no_board_fill || get_heuristic() == 0);
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return (sidelen() - 2) * (sidelen() - 2) - 
            boost::count_if(*this, [](const string& s) {
            return s.substr(1) != lineseg_off;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    vector<pair<char, vector<Position>>> m_num2targets;
    vector<Position> m_link;
};

puz_state::puz_state(const puz_game& g)
: vector<string>(g.m_start.size()), m_game(&g)
{
    for (int i = 0; i < size(); ++i)
        (*this)[i] = g.m_start[i] + lineseg_off;
    
    for (auto& kv : g.m_num2dist)
        m_num2targets.emplace_back(kv.first, g.m_num2targets.at(kv.first));

    new_link();
}

set<Position> puz_state::get_area(char ch) const
{
    set<Position> area;
    for (int r = 1; r < sidelen() - 1; ++r)
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (cells(p)[0] == ch)
                area.insert(p);
        }
    return area;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a, const Position& p_start)
        : m_area(&a) { make_move(p_start); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (auto& os : offset) {
        auto p = *this + os;
        if (m_area->count(p) != 0) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

bool puz_state::check_board() const
{
    set<Position> area = get_area(PUZ_SPACE), area2;

    for (auto& kv : m_num2targets) {
        char num = kv.first;
        auto& targets = kv.second;
        set<Position> area3 = area, area4;
        if (m_game->m_num2targets.at(num).size() > 2)
            area4 = get_area(num);
        else {
            area4.insert(targets.begin(), targets.end());
            if (targets.size() == 1)
                area4.insert(m_link.back());
        }

        area3.insert(area4.begin(), area4.end());
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves(
            {area3, targets.empty() ? m_link.back() : targets[0]}, smoves);
        set<Position> area5(smoves.begin(), smoves.end()), area6, area7;
        boost::set_difference(area5, area, inserter(area6, area6.end()));
        // ALL the same numbers must be reachable by the same line
        if (area4.size() != area6.size())
            return false;
        if (!m_game->m_no_board_fill) {
            boost::set_intersection(area, area5, inserter(area7, area7.end()));
            area2.insert(area7.begin(), area7.end());
        }
    }
    // ALL the squares must be reachable by lines
    return m_game->m_no_board_fill || area.size() == area2.size();
}

bool puz_state::make_move(int n)
{
    auto p = m_link.back();
    auto p2 = p + offset[n];
    m_link.push_back(p2);

    if (m_link.size() >= 4) {
        // no line can cover a 2*2 area
        vector<Position> v(m_link.rbegin(), m_link.rbegin() + 4);
        boost::sort(v);
        if (v[1] - v[0] == offset[1] && v[3] - v[2] == offset[1] &&
            v[2] - v[0] == offset[2] && v[3] - v[1] == offset[2])
            return false;
    }

    auto &str = cells(p), &str2 = cells(p2);

    str2[(2 + n) % 4 + 1] = str[n + 1] = PUZ_LINE_ON;
    if (str2[0] == PUZ_SPACE)
        str2[0] = str[0];
    else {
        auto& targets = m_num2targets.back().second;
        boost::remove_erase(targets, p2);
        if (targets.empty())
            m_num2targets.pop_back();
        new_link();
    }

    return check_board();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& p = m_link.back();
    char num1 = cells(p)[0];
    for (int i = 0; i < 4; ++i) {
        auto p2 = p + offset[i];
        char num2 = cells(p2)[0];
        if (num2 == PUZ_SPACE ||
            num2 == num1 && boost::algorithm::none_of_equal(m_link, p2)) {
            children.push_back(*this);
            if (!children.back().make_move(i))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horz-lines
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            auto str = cells(p);
            auto it = m_game->m_pos2num.find(p);
            out << (it != m_game->m_pos2num.end() ? it->second : ' ')
                << (str[2] == PUZ_LINE_ON ? '-' : ' ');
        }
        println(out);
        if (r == sidelen() - 2) break;
        for (int c = 1; c < sidelen() - 1; ++c)
            // draw vert-lines
            out << (cells({r, c})[3] == PUZ_LINE_ON ? "| " : "  ");
        println(out);
    }
    return out;
}

}

void solve_puz_NumberLink()
{
    using namespace puzzles::NumberLink;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/NumberLink.xml", "Puzzles/NumberLink.txt", solution_format::GOAL_STATE_ONLY);
}
