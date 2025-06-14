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
                auto& rng = m_area2range.emplace_back();
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
        auto& perms = m_ch2perms_around[ch];
        for (int i = 0; i <= 4; ++i) {
            string perm2;
            for (char ch2 = 'A'; ch2 <= 'C'; ++ch2)
                if (ch2 != ch)
                    perm2 += perm2.empty() ? string(4 - i, ch2) : string(i, ch2);
            do
                perms.push_back(perm2);
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
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
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
    map<int, vector<int>> m_matches;
    set<int> m_perm_id_rows, m_perm_id_cols;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    vector<int> perm_ids(g.m_perms_rc.size());
    boost::iota(perm_ids, 0);

    for (int i = 0; i < sidelen(); ++i)
        m_matches[i] = m_matches[sidelen() + i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for (auto& [p, perm_ids] : m_matches) {
        string chars;
        for (auto& p2 : m_game->m_area2range[p])
            chars.push_back(cells(p2));

        auto& ids = p < sidelen() ? m_perm_id_rows : m_perm_id_cols;
        boost::remove_erase_if(perm_ids, [&](int id) {
            return !boost::equal(chars, m_game->m_perms_rc.at(id), [](char ch1, char ch2) {
                return ch1 == PUZ_SPACE || ch1 == ch2;
            }) || ids.contains(id);
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& area = m_game->m_area2range[i];
    auto& perm = m_game->m_perms_rc[j];

    for (int k = 0; k < perm.size(); ++k)
        cells(area[k]) = perm[k];

    auto& ids = i < sidelen() ? m_perm_id_rows : m_perm_id_cols;
    ids.insert(j);

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
    auto& [p, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2) {
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
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            char ch = cells({r, c});
            out << ch << ' ';
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
