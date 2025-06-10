#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 1/Planets

    Summary
    Planets, Stars and Nebulas

    Description
    1. In Planets you are given an interesting Galaxy, where Suns only
       shine their light in horizontal and vertical lines.
    2. On the board you can see the Planets of this Galaxy. Each Planet
       is lit on some side (or not lit at all).
    3. You should place one Sun on each row and column, according to how
       the Planets are lit.
    4. You should also place one Nebula on each row and column.
    5. Nebulas block sunlight, so if there is a Nebula between a Sun and
       a Planet, the Planet won't be lit.
    6. Finally, Planets block sunlight too. So if there is a Planet
       between a Sun and another Planet, the further Planet won't be lit
       by that Sun.
*/

namespace puzzles::Planets{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_SUN = 'S';
constexpr auto PUZ_NEBULAR = 'N';
constexpr auto PUZ_PLANET = 'P';
constexpr auto PUZ_NORTH = 1;
constexpr auto PUZ_EAST = 2;
constexpr auto PUZ_SOUTH = 4;
constexpr auto PUZ_WEST = 8;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    map<Position, int> m_pos2lightness;
    // all permutations
    // ..NS
    // ..SN
    // ....
    // SN..
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
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
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
            char ch = str[c];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                m_start.push_back(PUZ_PLANET);
                int n = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
                m_pos2lightness[p] = n;
            }
        }
    }

    // 3. You should place one Sun on each row and column.
    // 4. You should also place one Nebula on each row and column.
    auto perm = string(m_sidelen - 2, PUZ_EMPTY) + PUZ_NEBULAR + PUZ_SUN;
    do {
        m_perms.push_back(perm);
    } while(boost::next_permutation(perm));
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
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    vector<int> perm_ids(g.m_perms.size());
    boost::iota(perm_ids, 0);

    for (int i = 0; i < sidelen(); ++i)
        m_matches[i] = m_matches[sidelen() + i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    auto& perms = m_game->m_perms;
    for (auto& [area_id, perm_ids] : m_matches) {
        bool is_horz = area_id < sidelen();

        auto& area = m_game->m_area2range[area_id];
        string chars;
        for (auto& p : area)
            chars.push_back(cells(p));

        boost::remove_erase_if(perm_ids, [&](int id) {
            auto perm = perms[id];
            vector<int> vi;
            for (int i = 0; i < sidelen(); i++) {
                char &ch1 = chars[i], &ch2 = perm[i];
                if (ch1 == PUZ_SPACE || ch1 == ch2) continue;
                if (ch1 == PUZ_PLANET && ch2 == PUZ_EMPTY) {
                    vi.push_back(i);
                    ch2 = ch1;
                } else
                    return true;
            }
            // 1. Suns only shine their light in horizontal and vertical lines.
            // 2. Each Planet is lit on some side(or not lit at all).
            // 5. Nebulas block sunlight, so if there is a Nebula between a Sun and
            //    a Planet, the Planet won't be lit.
            // 6. Planets block sunlight too. So if there is a Planet between a Sun
            //    and another Planet, the further Planet won't be lit by that Sun.
            for (int i : vi) {
                int n1 = m_game->m_pos2lightness.at(area[i]);
                bool lit = false;
                for (int j = i - 1; j >= 0; --j) {
                    char ch = perm[j];
                    if (ch == PUZ_SUN) {
                        lit = true;
                        break;
                    }
                    if (ch != PUZ_EMPTY) break;
                }
                int n2 = is_horz ? PUZ_WEST : PUZ_NORTH;
                if (lit && (n1 & n2) != n2 || !lit && (n1 & n2) != 0)
                    return true;
                lit = false;
                for (int j = i + 1; j < sidelen(); ++j) {
                    char ch = perm[j];
                    if (ch == PUZ_SUN) {
                        lit = true;
                        break;
                    }
                    if (ch != PUZ_EMPTY) break;
                }
                n2 = is_horz ? PUZ_EAST : PUZ_SOUTH;
                if (lit && (n1 & n2) != n2 || !lit && (n1 & n2) != 0)
                    return true;
            }
            return false;
        });

        if (!init)
            switch(perm_ids.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_area2range[i];
    auto& perm = m_game->m_perms[j];

    for (int k = 0; k < perm.size(); ++k) {
        char &ch = cells(range[k]);
        if (ch != PUZ_PLANET)
            ch = perm[k];
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
    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1, 
        const pair<const int, vector<int>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });
    for (int n : perm_ids) {
        children.push_back(*this);
        if (!children.back().make_move(area_id, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c) {
            Position p(r, c);
            char ch = cells(p);
            if (ch != PUZ_PLANET)
                out << ch << ' ';
            else {
                int n = m_game->m_pos2lightness.at(p);
                out << char(n < 10 ? n + '0' : n - 10 + 'A') << ' ';
            }
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Planets()
{
    using namespace puzzles::Planets;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Planets.xml", "Puzzles/Planets.txt", solution_format::GOAL_STATE_ONLY);
}
