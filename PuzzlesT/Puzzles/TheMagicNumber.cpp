#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 4/Puzzle Set 3/The Magic Number

    Summary
    No more, no less, you don't have to guess

    Description
    1. Fill the board with 3 different symbols.
    2. On side-6 boards there will be 2 of each on any row or column.
    3. On side-9 boards there will be 3 of each on any row or column.
    4. On side-12 boards there will be 4 of each on any row or column.
    5. When a tile has a shaded background, the symbols around it must
       be different.
*/

namespace puzzles::TheMagicNumber{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_BOUNDARY = '+';
constexpr auto PUZ_SYMBOL1 = '1';
constexpr auto PUZ_SYMBOL2 = '2';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_num_symbols;
    string m_start;
    vector<vector<Position>> m_area2range;
    vector<string> m_perms_rc;
    map<char, vector<string>> m_ch2perms_around;
    map<Position, char> m_pos2char;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
, m_num_symbols(strs.size() / 3)
, m_area2range(m_sidelen * 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
            if (char ch = str[c - 1]; !islower(ch))
                m_start.push_back(ch);
            else {
                m_start.push_back(toupper(ch));
                m_pos2char.emplace(p, ch);
                auto& rng = m_area2range.emplace_back();
                rng.push_back(p);
                for (auto& os : offset)
                    rng.push_back(p + os);
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    string perm;
    for (char ch = 'A'; ch <= 'C'; ++ch)
        perm += string(m_num_symbols, ch);
    do
        m_perms_rc.push_back(perm);
    while (boost::next_permutation(perm));

    for (char ch = 'A'; ch <= 'C'; ++ch) {
        string ch_str(1, ch);
        auto& perms = m_ch2perms_around[ch];
        vector<char> v{'A', 'B', 'C'};
        boost::remove_erase(v, ch);
        for (int i = 0; i <= 4; ++i) {
            string perm2 = string(4 - i, v[0]) + string(i, v[1]);
            do
                perms.push_back(ch_str + perm2);
            while (boost::next_permutation(perm2));
        }
    }
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
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count(m_cells, PUZ_SPACE) + m_matches.size();
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    vector<int> perm_ids(g.m_perms_rc.size());
    boost::iota(perm_ids, 0);
    for (int i = 1; i < sidelen() - 1; ++i)
        m_matches[i] = m_matches[sidelen() + i] = perm_ids;

    vector<int> perm_ids2(g.m_ch2perms_around.at('A').size());
    boost::iota(perm_ids2, 0);
    for (int i = sidelen() * 2; i < g.m_area2range.size(); ++i)
        m_matches[i] = perm_ids2;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [i, perm_ids] : m_matches) {
        string chars;
        for (auto& p2 : m_game->m_area2range[i])
            chars.push_back(cells(p2));

        auto& perms = i < sidelen() * 2 ? m_game->m_perms_rc :
            m_game->m_ch2perms_around.at(chars[0]);
        boost::remove_erase_if(perm_ids, [&](int id) {
            auto& perm = perms[id];
            return !boost::equal(chars, perm, [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == PUZ_BOUNDARY || ch1 == ch2;
            });
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(i, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& rng = m_game->m_area2range[i];
    auto& perm = (i < sidelen() * 2 ? m_game->m_perms_rc :
        m_game->m_ch2perms_around.at(cells(rng[0])))[j];

    for (int k = 0; k < perm.size(); ++k) {
        char& ch = cells(rng[k]);
        if (ch == PUZ_SPACE)
            ch = perm[k], ++m_distance;
    }

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
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
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(i, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (auto it = m_game->m_pos2char.find(p); it != m_game->m_pos2char.end())
                out << it->second;
            else
                out << ch;
            out << ' ';
        }
        println(out);
    }
    return out;
}

}

void solve_puz_TheMagicNumber()
{
    using namespace puzzles::TheMagicNumber;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/TheMagicNumber.xml", "Puzzles/TheMagicNumber.txt", solution_format::GOAL_STATE_ONLY);
}
