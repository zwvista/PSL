#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 6/Slanted Maze

    Summary
    Maze of Slants!

    Description
    1. Fill the board with diagonal lines (Slants), following the hints at
       the intersections.
    2. Every number tells you how many Slants (diagonal lines) touch that
       point. So, for example, a 4 designates an X pattern around it.
    3. The Mazes or paths the Slants will form will usually branch off many
       times, but can also end abruptly. Also all the Slants don't need to
       be all connected.
    4. However, you must ensure that they don't form a closed loop anywhere.
       This also means very big loops, not just 2*2.
*/

namespace puzzles::SlantedMaze{

constexpr auto PUZ_UNKNOWN = 5;
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_SLASH = '/';
constexpr auto PUZ_BACKSLASH = '\\';
constexpr auto PUZ_BOUNDARY = 'B';
constexpr auto PUZ_TOUCHED = 1;
constexpr auto PUZ_UNTOUCHED = 0;

constexpr Position offset[] = {
    {0, 0},    // nw
    {0, 1},    // ne
    {1, 1},    // se
    {1, 0},    // sw
};

constexpr Position offset2[] = {
    {-1, -1},    // nw
    {-1, 1},        // ne
    {1, 1},        // se
    {1, -1},        // sw
};

const string slants = "\\/\\/";

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    vector<vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 1)
, m_num2perms(PUZ_UNKNOWN + 1)
{
    for (int r = 0; r < m_sidelen - 1; ++r) {
        auto& str = strs[r];
        for (int c = 0; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            char ch = str[c];
            m_pos2num[p] = ch == ' ' ? PUZ_UNKNOWN : ch - '0';
        }
    }

    auto perm = slants;
    auto& perms_unknown = m_num2perms[PUZ_UNKNOWN];
    for (int i = 0; i <= 4; ++i) {
        auto& perms = m_num2perms[i];
        vector<int> indexes(4, PUZ_UNTOUCHED);
        fill(indexes.begin() + 4 - i, indexes.end(), PUZ_TOUCHED);
        do {
            for (int j = 0; j < 4; ++j) {
                char ch = slants[j];
                perm[j] = indexes[j] == PUZ_TOUCHED ? ch :
                        ch == PUZ_SLASH ? PUZ_BACKSLASH :
                        PUZ_SLASH;
            }
            perms.push_back(perm);
            perms_unknown.push_back(perm);
        } while(boost::next_permutation(indexes));
    }
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool check_loop() const;

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
{
    for (int i = 0; i < sidelen(); ++i)
        cells({i, 0}) = cells({i, sidelen() - 1}) =
        cells({0, i}) = cells({sidelen() - 1, i}) =
        PUZ_BOUNDARY;

    for (auto& [p, n] : g.m_pos2num) {
        auto& perm_ids = m_matches[p];
        perm_ids.resize(g.m_num2perms[n].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (int i = 0; i < 4; ++i) {
            char ch = cells(p + offset[i]);
            if (ch == PUZ_BOUNDARY)
                ch = slants[i] == PUZ_SLASH ? PUZ_BACKSLASH : PUZ_SLASH;
            chars.push_back(ch);
        }

        auto& perms = m_game->m_num2perms[m_game->m_pos2num.at(p)];
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, perms[id], [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];
    for (int k = 0; k < perm.size(); ++k) {
        char& ch = cells(p + offset[k]);
        if (ch == PUZ_SPACE)
            ch = perm[k];
    }

    ++m_distance;
    m_matches.erase(p);

    return check_loop();
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s, set<Position>& dots, set<Position>& path, bool& has_loop)
        : m_state(&s), m_dots(&dots), m_path(&path), m_has_loop(&has_loop) {
        make_move(-1, *dots.begin());
    }

    int sidelen() const { return m_state->sidelen(); }
    void make_move(int n, const Position& p);
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
    set<Position> *m_dots, *m_path;
    bool* m_has_loop;
    int m_last_dir;
};

void puz_state2::make_move(int n, const Position& p)
{
    m_last_dir = n;
    static_cast<Position&>(*this) = p;
    if (!m_path->contains(p))
        m_path->insert(p);
    else
        *m_has_loop = true;
    m_dots->erase(p);
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i)
        if ((i + 2) % 4 != m_last_dir &&
            m_state->cells(*this + offset[i]) == slants[i]) {
            children.push_back(*this);
            children.back().make_move(i, *this + offset2[i]);
        }
}

bool puz_state::check_loop() const
{
    set<Position> dots;
    for (int r = 0; r < sidelen() - 1; ++r)
        for (int c = 0; c < sidelen() - 1; ++c)
            dots.emplace(r, c);

    while (!dots.empty()) {
        bool has_loop = false;
        set<Position> path;
        list<puz_state2> smoves;
        puz_move_generator<puz_state2>::gen_moves({*this, dots, path, has_loop}, smoves);
        if (has_loop)
            return false;
    }
    return true;
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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1,
        const pair<const Position, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(p, n))
            children.pop_back();
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

void solve_puz_SlantedMaze()
{
    using namespace puzzles::SlantedMaze;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/SlantedMaze.xml", "Puzzles/SlantedMaze.txt", solution_format::GOAL_STATE_ONLY);
}
