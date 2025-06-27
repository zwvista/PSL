#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 9/Lighthouses

    Summary
    Lighten Up at Sea

    Description
    1. You are at sea and you need to find the lighthouses and light the boats.
    2. Each boat has a number on it that tells you how many lighthouses are lighting it.
    3. A lighthouse lights all the tiles horizontally and vertically and doesn't
       stop at boats or other lighthouses.
    4. Finally, no boat touches another boat or lighthouse, not even diagonally.
       No lighthouse touches another lighthouse as well.
*/

namespace puzzles::Lighthouses{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_BOAT = 'T';
constexpr auto PUZ_LIGHTHOUSE = 'L';
constexpr auto PUZ_BOUNDARY = '`';

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

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
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
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_cells.push_back(ch);
            else {
                m_cells.push_back(PUZ_BOAT);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_cells.push_back(PUZ_BOUNDARY);
    }
    m_cells.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_area
{
    // all the tiles that are in the same row or column as the boat
    vector<Position> m_rng;
    // elem: characters that represent the kind of each tile
    vector<string> m_perms;
    bool operator<(const puz_area& x) const {
        return std::tie(m_rng, m_perms) < std::tie(x.m_rng, x.m_perms);
    }
};

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(const Position& p, const vector<Position>& rng, const string& perm);
    void make_move2(const Position& p, const vector<Position>& rng, const string& perm);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, puz_area> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_cells), m_game(&g)
{
    // 4. No boat touches another boat or lighthouse, not even diagonally.
    for (auto& [p, n] : g.m_pos2num)
        for (auto& os : offset) {
            char& ch = cells(p + os);
            if (ch == PUZ_SPACE)
                ch = PUZ_EMPTY;
        }

    for (auto& [p, n] : g.m_pos2num)
        m_matches[p];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, area] : m_matches) {
        auto& rng_space = area.m_rng;
        rng_space.clear();
        auto& perms = area.m_perms;
        perms.clear();

        vector<Position> rng_light;
        for (int i = 0; i < 4; ++i) {
            auto& os = offset[i * 2];
            for (auto p2 = p + os; ; p2 += os) {
                char ch = cells(p2);
                if (ch == PUZ_SPACE)
                    rng_space.push_back(p2);
                else if (ch == PUZ_LIGHTHOUSE)
                    rng_light.push_back(p2);
                else if (ch == PUZ_BOUNDARY)
                    break;
            }
        }

        // Out of all the tiles in the same row or column as the boat,
        // ns are undetermined and nl are lighthouses
        int ns = rng_space.size(), nl = rng_light.size();
        int n = m_game->m_pos2num.at(p), nl2 = n - nl;

        if (nl2 >= 0 && nl2 <= ns) {
            auto perm = string(ns - nl2, PUZ_EMPTY) + string(nl2, PUZ_LIGHTHOUSE);
            vector<Position> rng_light2(nl2);
            do {
                for (int i = 0, j = 0; i < perm.length(); ++i)
                    if (perm[i] == PUZ_LIGHTHOUSE)
                        rng_light2[j++] = rng_space[i];
                if ([&]{
                    // No lighthouse touches another lighthouse
                    for (const auto& p1 : rng_light2)
                        for (const auto& p2 : rng_light2)
                            if (boost::algorithm::any_of_equal(offset, p1 - p2))
                                return false;
                    return true;
                }())
                    perms.push_back(perm);
            } while(boost::next_permutation(perm));
        }

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, rng_space, perms.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, const vector<Position>& rng, const string& perm)
{
    for (int i = 0; i < rng.size(); ++i) {
        auto& p2 = rng[i];
        if ((cells(p2) = perm[i]) == PUZ_LIGHTHOUSE)
            for (auto& os : offset) {
                char& ch = cells(p2 + os);
                if (ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
            }
    }

    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, const vector<Position>& rng, const string& perm)
{
    m_distance = 0;
    make_move2(p, rng, perm);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [p, area] = *boost::min_element(m_matches, [](
        const pair<const Position, puz_area>& kv1,
        const pair<const Position, puz_area>& kv2) {
        return kv1.second.m_perms.size() < kv2.second.m_perms.size();
    });

    for (auto& perm : area.m_perms) {
        children.push_back(*this);
        if (!children.back().make_move(p, area.m_rng, perm))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch == PUZ_BOAT)
                out << format("{:2}", m_game->m_pos2num.at(p));
            else
                out << ' ' << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Lighthouses()
{
    using namespace puzzles::Lighthouses;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Lighthouses.xml", "Puzzles/Lighthouses.txt", solution_format::GOAL_STATE_ONLY);
}
