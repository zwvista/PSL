#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Noughts & Crosses

    Summary
    Spot the Number

    Description
    1. Place all numbers from 1 to N on each row and column - just once,
       without repeating.
    2. In other words, all numbers must appear just once on each row and column.
    3. A circle marks where a number must go.
    4. A cross marks where no number can go.
    5. All other cells can contain a number or be empty.
*/

namespace puzzles{ namespace NoughtsAndCrosses{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_CIRCLE       'O'
#define PUZ_CROSS        'X'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_numbers : set<char>
{
    puz_numbers() {}
    puz_numbers(int num) {
        insert(PUZ_EMPTY);
        for (int i = 0; i < num; ++i)
            insert(i + '1');
    }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_num;
    puz_numbers m_numbers;
    vector<vector<Position>> m_area_pos;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_num(level.attribute("num").as_int())
    , m_area_pos(m_sidelen * 2)
    , m_numbers(m_num)
{
    m_start = boost::accumulate(strs, string());
    for (int r = 0; r < m_sidelen; ++r)
        for (int c = 0; c < m_sidelen; ++c) {
            Position p(r, c);
            m_area_pos[p.first].push_back(p);
            m_area_pos[m_sidelen + p.second].push_back(p);
        }
}

// first: the index of the area
// second : all char numbers used to fill a position
typedef pair<int, puz_numbers> puz_area;

struct puz_group : vector<puz_area>
{
    puz_group() {}
    puz_group(int index, int sz, const puz_numbers& numbers) {
        for (int i = 0; i < sz; i++)
            emplace_back(index++, numbers);
    }
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, char ch);
    void remove_pair(const Position& p, char ch) {
        auto i = m_pos2nums.find(p);
        if (i != m_pos2nums.end())
            i->second.erase(ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::range::count(*this, PUZ_SPACE);}
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
    map<Position, puz_numbers> m_pos2nums;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_sidelen * g.m_sidelen, PUZ_SPACE), m_game(&g)
    , m_grp_rows(0, g.m_sidelen, g.m_numbers)
    , m_grp_cols(g.m_sidelen, g.m_sidelen, g.m_numbers)
{
    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c)
            m_pos2nums[{r, c}] = g.m_numbers;
    for (int r = 0; r < g.m_sidelen; ++r)
        for (int c = 0; c < g.m_sidelen; ++c) {
            Position p(r, c);
            switch(char ch = g.cells(p)) {
            case PUZ_SPACE:
                break;
            case PUZ_CIRCLE:
                remove_pair(p, PUZ_EMPTY);
                break;
            case PUZ_CROSS:
                make_move(p, PUZ_EMPTY);
                break;
            default:
                make_move(p, ch);
                break;
            }
        }
}

bool puz_state::make_move(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2nums.erase(p);

    auto areas = {
        &m_grp_rows[p.first],
        &m_grp_cols[p.second]
    };
    for (puz_area* a : areas)
        for (auto& p2 : m_game->m_area_pos[a->first])
            remove_pair(p2, ch);

    return boost::algorithm::none_of(m_pos2nums, [](const pair<const Position, puz_numbers>& kv) {
        return kv.second.empty();
    });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_pos2nums, [](
        const pair<const Position, puz_numbers>& kv1,
        const pair<const Position, puz_numbers>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    auto& p = kv.first;
    for (char ch : kv.second) {
        children.push_back(*this);
        if (!children.back().make_move(p, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 0; r < sidelen(); ++r) {
        for (int c = 0; c < sidelen(); ++c)
            out << cells({r, c}) << " ";
        out << endl;
    }
    return out;
}

}}

void solve_puz_NoughtsAndCrosses()
{
    using namespace puzzles::NoughtsAndCrosses;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles/NoughtsAndCrosses.xml", "Puzzles/NoughtsAndCrosses.txt", solution_format::GOAL_STATE_ONLY);
}
