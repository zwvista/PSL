#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games 3/Puzzle Set 3/Rabbits

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

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_RABBIT       'R'
#define PUZ_TREE         'T'

const Position offset[] = {
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
    map<Position, vector<pair<int, int>>> m_pos2ids;

    puz_game(const vector<string>& strs, const xml_node& level);
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size())
, m_area2range(m_sidelen * 2)
{
    for (int r = 0; r < m_sidelen; ++r) {
        auto& str = strs[r];
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

    for (auto& [p, n] : m_pos2num) {
        auto& ids = m_pos2ids[p];
        for (int i = 0; i < m_perms.size(); ++i)
            for (int j = 0; j < m_perms.size(); ++j)
                if (m_perms[i].m_nums[p.second] + m_perms[j].m_nums[p.first] == n)
                    ids.emplace_back(i, j);
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int r, int c, int i, int j);
    void make_move2(int r, int c, int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    map<Position, vector<pair<int, int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start), m_matches(g.m_pos2ids)
{
    for (int i = 0; i < sidelen(); ++i) {
        for (int j = 0; j < g.m_perms.size(); ++j) {
            m_matches[{i, -1}].emplace_back(j, -1);
            m_matches[{-1, i}].emplace_back(-1, j);
        }
    }
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& kv : m_matches) {
        auto& p = kv.first;
        auto& perm_id_pairs = kv.second;

        string chars1, chars2;
        if (p.first != -1)
            for (auto& p2 : m_game->m_area2range.at(p.first))
                chars1.push_back(cells(p2));
        if (p.second != -1)
            for (auto& p2 : m_game->m_area2range.at(sidelen() + p.second))
                chars2.push_back(cells(p2));

        boost::remove_erase_if(perm_id_pairs, [&](const pair<int, int>& id_pair) {
            int id1 = id_pair.first, id2 = id_pair.second;
            return id1 != -1 && !boost::equal(chars1, m_game->m_perms[id1].m_perm, [&](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }) || id2 != -1 && !boost::equal(chars2, m_game->m_perms[id2].m_perm, [&](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_id_pairs.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p.first, p.second, perm_id_pairs[0].first, perm_id_pairs[0].second), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int r, int c, int i, int j)
{
    if (r != -1) {
        auto& rng = m_game->m_area2range.at(r);
        auto& perm = m_game->m_perms[i].m_perm;
        for (int k = 0; k < rng.size(); ++k)
            cells(rng[k]) = perm[k];
    }
    if (c != -1) {
        auto& rng = m_game->m_area2range.at(sidelen() + c);
        auto& perm = m_game->m_perms[j].m_perm;
        for (int k = 0; k < rng.size(); ++k)
            cells(rng[k]) = perm[k];
    }
    for (auto& [p, perm_id_pairs] : m_matches) {
        if (r != -1 && r == p.first)
            boost::remove_erase_if(perm_id_pairs, [&](const pair<int, int>& id_pair) {
                return id_pair.first != i;
            });
        if (c != -1 && c == p.second)
            boost::remove_erase_if(perm_id_pairs, [&](const pair<int, int>& id_pair) {
                return id_pair.second != j;
            });
    }
    m_distance++;
    m_matches.erase({ r, c });
}

bool puz_state::make_move(int r, int c, int i, int j)
{
    m_distance = 0;
    make_move2(r, c, i, j);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<pair<int, int>>>& kv1,
        const pair<const Position, vector<pair<int, int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (auto& id_pair : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(kv.first.first, kv.first.second, id_pair.first, id_pair.second))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            if (m_game->m_pos2num.count(p) == 1)
                out << m_game->m_pos2num.at(p);
            else
                out << cells(p);
        }
        out << endl;
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
