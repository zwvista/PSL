#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 3/Rabbits

    Summary
    Rabbit 'n' Seek

    Description
    1. The board represents a lawn where Rabbits are playing Hide 'n' Seek,
       behind Trees.
    2. Each number tells you how many Rabbits can be seen from that tile,
       in an horizontal and vertical line.
    3. Tree hide Rabbits, numbers don't.
    4. Each row and column has exactly one Tree and one Rabbit.
*/

namespace puzzles::Rabbits{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_RABBIT = 'R';
constexpr auto PUZ_TREE = 'T';

constexpr Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
};

// rabbit and tree
struct puz_rt {
    string m_perm;
    vector<int> m_nums;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, int> m_pos2num;
    vector<puz_rt> m_perms;
    vector<vector<Position>> m_area2range;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        string_view str = strs[r];
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            char ch = str[c];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_EMPTY);
                m_pos2num[p] = ch - '0';
            }
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }
    }
    string perm(m_sidelen, PUZ_EMPTY);
    perm[m_sidelen - 2] = PUZ_RABBIT;
    perm[m_sidelen - 1] = PUZ_TREE;
    do {
        puz_rt o;
        o.m_perm = perm;
        for (int i = 0; i < m_sidelen; ++i) {
            char ch = perm[i];
            int n = 0;
            if (ch != PUZ_EMPTY)
                n = 99;
            else {
                for (int j = i - 1; j >= 0; --j)
                    if (char ch = perm[j]; ch != PUZ_EMPTY) {
                        n += ch == PUZ_RABBIT ? 1 : 0;
                        break;
                    }
                for (int j = i + 1; j < m_sidelen; ++j)
                    if (char ch = perm[j]; ch != PUZ_EMPTY) {
                        n += ch == PUZ_RABBIT ? 1 : 0;
                        break;
                    }
            }
            o.m_nums.push_back(n);
        }
        m_perms.push_back(o);
    } while (boost::next_permutation(perm));
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
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);
    bool remove_matches(int id1, int id2, int n) {
        auto& v = m_matches[id1];
        boost::remove_erase_if(v, [&](int id) {
            return m_game->m_perms[id].m_nums[id2] != n;
        });
        return !v.empty();
    }

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size() + m_pos2num.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    map<int, int> m_rc2index;
    map<Position, int> m_pos2num;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start), m_pos2num(g.m_pos2num)
{
    vector<int> perm_ids(m_game->m_perms.size());
    boost::iota(perm_ids, 0);
    for (int i = 0; i < sidelen() * 2; ++i)
        m_matches[i] = perm_ids;
    for (auto& [p, n] : m_pos2num)
        if (n == 0 || n == 2) {
            auto& [r, c] = p;
            remove_matches(r, c, n / 2);
            remove_matches(sidelen() + c, r, n / 2);
        }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [i, perm_ids] : m_matches) {
        string chars;
        for (auto& p2 : m_game->m_area2range.at(i))
            chars.push_back(cells(p2));

        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, m_game->m_perms[id].m_perm, [&](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(i, perm_ids[0]) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& rng = m_game->m_area2range.at(i);
    auto& perm = m_game->m_perms[j].m_perm;
    for (int k = 0; k < rng.size(); ++k)
        cells(rng[k]) = perm[k];
    m_distance++;
    m_matches.erase(i);
    m_rc2index[i] = j;

    for (auto it = m_pos2num.begin(); it != m_pos2num.end();) {
        auto& [p, n] = *it;
        auto& [r, c] = p;
        bool hasR = m_matches.contains(r), hasC = m_matches.contains(sidelen() + c);
        if (!hasR && !hasC) {
            m_distance++;
            it = m_pos2num.erase(it);
        }
        else {
            if (n == 1 && (
                hasR && !hasC && !remove_matches(r, c, 1 - m_game->m_perms[m_rc2index.at(sidelen() + c)].m_nums[r]) ||
                !hasR && hasC && !remove_matches(sidelen() + c, r, 1 - m_game->m_perms[m_rc2index.at(r)].m_nums[c])))
                return false;
            it++;
        }
    }
    return true;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if (!make_move2(i, j))
        return false;
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [i, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int j : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(i, j))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (m_game->m_pos2num.contains(p))
                out << m_game->m_pos2num.at(p);
            else
                out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Rabbits()
{
    using namespace puzzles::Rabbits;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Rabbits.xml", "Puzzles/Rabbits.txt", solution_format::GOAL_STATE_ONLY);
}
