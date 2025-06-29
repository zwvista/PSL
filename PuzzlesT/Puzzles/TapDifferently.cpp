#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 10/Tap Differently

    Summary
    Tapa Landscaper

    Description
    1. Plays with the same rules as Tapa with these variations:
    2. Each row must have a different number of filled cells.
    3. Each column must have a different number of filled cells.
*/

namespace puzzles::TapDifferently{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_QM = '?';
constexpr auto PUZ_FILLED = 'F';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_HINT = 'H';
constexpr auto PUZ_BOUNDARY = '`';
constexpr auto PUZ_UNKNOWN = -1;

constexpr Position offset[] = {
    {-1, 0},       // n
    {-1, 1},       // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},       // sw
    {0, -1},       // w
    {-1, -1},      // nw
};

using puz_hint = vector<int>;

puz_hint compute_hint(const vector<int>& filled)
{
    vector<int> hint;
    for (int j = 0; j < filled.size(); ++j)
        if (j == 0 || filled[j] - filled[j - 1] != 1)
            hint.push_back(1);
        else
            ++hint.back();
    if (filled.size() > 1 && hint.size() > 1 && filled.back() - filled.front() == 7)
        hint.front() += hint.back(), hint.pop_back();
    boost::sort(hint);
    return hint;
}

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_hint> m_pos2hint;
    map<puz_hint, vector<string>> m_hint2perms;
    string m_cells;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            auto s = str.substr(c * 4 - 4, 4);
            if (s == "    ")
                m_cells.push_back(PUZ_SPACE);
            else {
                m_cells.push_back(PUZ_HINT);
                auto& hint = m_pos2hint[{r, c}];
                for (int i = 0, n = 0; i < 4; ++i) {
                    char ch = s[i];
                    if (ch != PUZ_SPACE)
                        hint.push_back(ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0');
                }
                m_hint2perms[hint];
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);

    // A tile is surrounded by at most 8 tiles, each of which has two states:
    // filled or empty. So all combinations of the states of the
    // surrounding tiles can be coded into an 8-bit number(0 -- 255).
    for (int i = 1; i < 256; ++i) {
        vector<int> filled;
        for (int j = 0; j < 8; ++j)
            if (i & (1 << j))
                filled.push_back(j);

        string perm(8, PUZ_EMPTY);
        for (int j : filled)
            perm[j] = PUZ_FILLED;

        auto hint = compute_hint(filled);

        for (auto& [hint2, perms] : m_hint2perms) {
            if (hint2 == hint)
                perms.push_back(perm);
            else if (boost::algorithm::any_of_equal(hint2, PUZ_UNKNOWN) && hint2.size() == hint.size()) {
                auto hint3 = hint2;
                boost::remove_erase(hint3, PUZ_UNKNOWN);
                if (boost::includes(hint, hint3))
                    perms.push_back(perm);
            }
        }
    }

    // A cell with a 0 means all its surrounding cells are empty.
    if (auto it = m_hint2perms.find({0}); it != m_hint2perms.end())
        it->second = {string(8, PUZ_EMPTY)};
}

using puz_path = pair<vector<Position>, int>;

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move_hint(const Position& p, int n);
    bool make_move_hint2(const Position& p, int n);
    bool make_move_space(const Position& p, char ch);
    int find_matches(bool init);
    puz_hint compute_hint(const Position& p) const;
    bool is_valid_move() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return boost::count(*this, PUZ_SPACE); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& os : offset)
            chars.push_back(cells(p + os));

        auto& perms = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p));
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return (ch1 == PUZ_BOUNDARY || ch1 == PUZ_HINT) && ch2 == PUZ_EMPTY ||
                    ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move_hint2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

puz_state::puz_state(const puz_game& g)
: string(g.m_cells), m_game(&g)
{
    for (auto& [p, hint] : g.m_pos2hint) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_hint2perms.at(hint).size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, const Position& starting)
        : m_state(&s) {    make_move(starting); }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p2 = *this + offset[i * 2];
        char ch = m_state->cells(p2);
        if (ch == PUZ_SPACE || ch == PUZ_FILLED) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::make_move_hint2(const Position& p, int n)
{
    auto& perm = m_game->m_hint2perms.at(m_game->m_pos2hint.at(p))[n];
    for (int i = 0; i < 8; ++i) {
        auto p2 = p + offset[i];
        char& ch = cells(p2);
        if (ch == PUZ_SPACE)
            ch = perm[i], ++m_distance;
    }

    m_matches.erase(p);
    return is_valid_move();
}

bool puz_state::make_move_hint(const Position& p, int n)
{
    m_distance = 0;
    if (!make_move_hint2(p, n))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_space(const Position& p, char ch)
{
    cells(p) = ch;
    m_distance = 1;
    return is_valid_move();
}

puz_hint puz_state::compute_hint(const Position& p) const
{
    vector<int> filled;
    for (int j = 0; j < 8; ++j)
        if (cells(p + offset[j]) == PUZ_FILLED)
            filled.push_back(j);

    return puzzles::TapDifferently::compute_hint(filled);
}

bool puz_state::is_valid_move() const
{
    auto is_interconnected = [&]{
        int i = find(PUZ_FILLED);
        if (i == -1)
            return true;
        auto smoves = puz_move_generator<puz_state2>::gen_moves(
            {*this, {i / sidelen(), i % sidelen()}});
        return boost::count_if(smoves, [&](const Position& p) {
            return cells(p) == PUZ_FILLED;
        }) == boost::count(*this, PUZ_FILLED);
    };

    auto is_same_color = [&](const vector<Position>& rng, const vector<char>& color) {
        return boost::algorithm::all_of(rng, [&](const Position& p) {
            return boost::algorithm::any_of_equal(color, cells(p));
        });
    };

    auto is_valid_square = [&](const vector<char>& color) {
        for (int r = 1; r < sidelen() - 2; ++r)
            for (int c = 1; c < sidelen() - 2; ++c)
                if (is_same_color({{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}}, color))
                    return false;
        return true;
    };

    auto is_tap_differently = [&](const std::function<Position(int, int)>& g) {
        set<int> nums;
        for (int i = 1; i < sidelen() - 1; ++i) {
            int n = 0;
            bool has_space = false;
            for (int j = 1; j < sidelen() - 1; ++j) {
                char ch = cells(g(i, j));
                if (ch == PUZ_SPACE) {
                    has_space = true;
                    break;
                }
                if (ch == PUZ_FILLED)
                    n++;
            }
            if (has_space)
                continue;
            if (nums.contains(n))
                return false;
            else
                nums.insert(n);
        }
        return true;
    };

    return is_interconnected() && is_valid_square({PUZ_FILLED}) &&
        is_tap_differently([](int i, int j) { return Position(i, j); }) &&
        is_tap_differently([](int i, int j) { return Position(j, i); });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_matches.empty()) {
        auto& [p, perm_ids] = *boost::min_element(m_matches, [](
            const pair<const Position, vector<int>>& kv1,
            const pair<const Position, vector<int>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

        for (int n : perm_ids) {
            children.push_back(*this);
            if (!children.back().make_move_hint(p, n))
                children.pop_back();
        }
    } else {
        int n = find(PUZ_SPACE);
        if (n == -1)
            return;
        Position p(n / sidelen(), n % sidelen());
        for (char ch : {PUZ_FILLED, PUZ_EMPTY}) {
            children.push_back(*this);
            if (!children.back().make_move_space(p, ch))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_HINT) {
                auto hint = m_game->m_pos2hint.at(p);
                if (boost::algorithm::any_of_equal(hint, PUZ_UNKNOWN))
                    hint = compute_hint(p);
                for (int n : hint)
                    out << char(n + '0');
                out << string(4 - hint.size(), ' ');
            } else
                out << ch << "   ";
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TapDifferently()
{
    using namespace puzzles::TapDifferently;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TapDifferently.xml", "Puzzles/TapDifferently.txt", solution_format::GOAL_STATE_ONLY);
}
