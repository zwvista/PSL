#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 9/Tapa

    Summary
    Turkish art of PAint(TAPA)

    Description
    1. The goal is to fill some tiles forming a single orthogonally continuous
       path. Just like Nurikabe.
    2. A number indicates how many of the surrounding tiles are filled. If a
       tile has more than one number, it hints at multiple separated groups
       of filled tiles.
    3. For example, a cell with a 1 and 3 means there is a continuous group
       of 3 filled cells around it and one more single filled cell, separated
       from the other 3. The order of the numbers in this case is irrelevant.
    4. Filled tiles can't cover an area of 2*2 or larger (just like Nurikabe).
       Tiles with numbers can be considered 'empty'.

    Variations
    5. Tapa has plenty of variations. Some are available in the levels of this
       game. Stronger variations are B-W Tapa, Island Tapa and Pata and have
       their own game.
    6. Equal Tapa - The board contains an equal number of white and black tiles.
       Tiles with numbers or question marks are NOT counted as empty or filled
       for this rule (i.e. they're left out of the count).
    7. Four-Me-Tapa - Four-Me-Not rule apply: you can't have more than three
       filled tiles in line.
    8. No Square Tapa - No 2*2 area of the board can be left empty.
*/

namespace puzzles::Tapa{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_QM = '?';
constexpr auto PUZ_FILLED = 'F';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_HINT = 'H';
constexpr auto PUZ_BOUNDARY = 'B';
constexpr auto PUZ_UNKNOWN = -1;

constexpr Position offset[] = {
    {-1, 0},        // n
    {-1, 1},        // ne
    {0, 1},        // e
    {1, 1},        // se
    {1, 0},        // s
    {1, -1},        // sw
    {0, -1},        // w
    {-1, -1},    // nw
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

enum class puz_game_type
{
    NORMAL,
    EQUAL,
    FOUR_ME,
    NO_SQUARE,
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    puz_game_type m_game_type;
    map<Position, puz_hint> m_pos2hint;
    map<puz_hint, vector<string>> m_hint2perms;
    int m_space_count;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
{
    string game_type = level.attribute("GameType").as_string("Tapa");
    m_game_type =
        game_type == "Equal Tapa" ? puz_game_type::EQUAL :
        game_type == "Four-Me-Tapa" ? puz_game_type::FOUR_ME :
        game_type == "No Square Tapa" ? puz_game_type::NO_SQUARE :
        puz_game_type::NORMAL;

    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            auto s = str.substr(c * 4 - 4, 4);
            if (s == "    ")
                m_start.push_back(PUZ_SPACE);
            else {
                m_start.push_back(PUZ_HINT);
                auto& hint = m_pos2hint[{r, c}];
                for (int i = 0; i < 4; ++i) {
                    char ch = s[i];
                    if (ch != PUZ_SPACE)
                        hint.push_back(ch == PUZ_QM ? PUZ_UNKNOWN : ch - '0');
                }
                m_hint2perms[hint];
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    m_space_count = boost::count(m_start, PUZ_SPACE);

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
    for (int n : {0, PUZ_UNKNOWN}) {
        auto it = m_hint2perms.find({n});
        if (it != m_hint2perms.end())
            it->second.emplace_back(8, PUZ_EMPTY);
    }
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
: string(g.m_start), m_game(&g)
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

    return puzzles::Tapa::compute_hint(filled);
}

bool puz_state::is_valid_move() const
{
    auto is_continuous = [&]{
        int i = find(PUZ_FILLED);
        if (i == -1)
            return true;
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves(
            {*this, {i / sidelen(), i % sidelen()}}, smoves);
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

    auto is_valid_tapa = [&]{ return is_valid_square({PUZ_FILLED}); };

    auto is_no_square_tapa = [&]{ return is_valid_square({PUZ_HINT, PUZ_EMPTY}); };

    auto is_equal_tapa = [&]{
        int n1 = boost::count(*this, PUZ_FILLED), n2 = boost::count(*this, PUZ_EMPTY);
        int n = m_game->m_space_count / 2;
        return !is_goal_state() ? n1 <= n && n2 <= n : n1 == n && n2 == n;
    };

    auto is_four_me_tapa = [&]{
        for (int i = 1; i < sidelen() - 4; ++i)
            for (int j = 1; j < sidelen() - 1; ++j)
                if (is_same_color({{i, j}, {i + 1, j}, {i + 2, j}, {i + 3, j}}, {PUZ_FILLED}) ||
                    is_same_color({{j, i}, {j, i + 1}, {j, i + 2}, {j, i + 3}}, {PUZ_FILLED}))
                    return false;
        return true;
    };

    return is_continuous() && is_valid_tapa() && (
        m_game->m_game_type == puz_game_type::NORMAL ||
        m_game->m_game_type == puz_game_type::EQUAL && is_equal_tapa() ||
        m_game->m_game_type == puz_game_type::FOUR_ME && is_four_me_tapa() ||
        m_game->m_game_type == puz_game_type::NO_SQUARE && is_no_square_tapa()
    );
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

void solve_puz_Tapa()
{
    using namespace puzzles::Tapa;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Tapa.xml", "Puzzles/Tapa.txt", solution_format::GOAL_STATE_ONLY);
}
