#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 6/Paint The Nurikabe

    Summary
    Paint areas, find Nurikabes

    Description
    1. By painting (filling) the areas you have to complete a Nurikabe.
       Specifically:
    2. A number indicates how many painted tiles are adjacent to it.
    3. The painted tiles form an orthogonally continuous area, like a
       Nurikabe.
    4. There can't be any 2*2 area of the same color(painted or empty).
*/

namespace puzzles{ namespace PaintTheNurikabe{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_MINE        'M'
#define PUZ_BOUNDARY    'B'
#define PUZ_UNKNOWN     5
    
const Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, int> m_pos2num;
    // all permutations
    vector<vector<string>> m_num2perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2 + 2)
    , m_num2perms(5)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(int r = 1; r < m_sidelen - 1; ++r){
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for(int c = 1; c < m_sidelen - 1; ++c){
            char ch = str[c - 1];
            m_start.push_back(ch == PUZ_SPACE);
            if(ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);

    for(int i = 0; i <= 4; ++i){
        auto perm = string(4 - i, PUZ_EMPTY) + string(i, PUZ_MINE);
        do
            m_num2perms[i].push_back(perm);
        while(boost::next_permutation(perm));
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
    bool make_move(const Position& p, int n);
    void make_move2(const Position& p, int n);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
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
    // key: the position of the number
    // value.elem: the index of the permutation
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for(auto& kv : g.m_pos2num){
        auto& perm_ids = m_matches[kv.first];
        perm_ids.resize(g.m_num2perms[kv.second].size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        auto& p = kv.first;
        auto& perm_ids = kv.second;

        string chars;
        for(auto& os : offset)
            chars.push_back(cells(p + os));

        auto& perms = m_game->m_num2perms[m_game->m_pos2num.at(p)];
        boost::remove_erase_if(perm_ids, [&](int id){
            return !boost::equal(chars, perms[id], [](char ch1, char ch2){
                return ch1 == PUZ_SPACE || ch1 == ch2 ||
                    ch1 == PUZ_BOUNDARY && ch2 == PUZ_EMPTY;
            });
        });

        if(!init)
            switch(perm_ids.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(p, perm_ids.front()), 1;
            }
    }
    return 2;
}

void puz_state::make_move2(const Position& p, int n)
{
    auto& perm = m_game->m_num2perms[m_game->m_pos2num.at(p)][n];

    for(int k = 0; k < perm.size(); ++k){
        char& ch = cells(p + offset[k]);
        if(ch == PUZ_SPACE)
            ch = perm[k];
    }

    ++m_distance;
    m_matches.erase(p);
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    make_move2(p, n);
    int m;
    while((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<int>>& kv1, 
        const pair<const Position, vector<int>>& kv2){
        return kv1.second.size() < kv2.second.size();
    });
    for(int n : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            auto it = m_game->m_pos2num.find(p);
            if(it == m_game->m_pos2num.end())
                out << format("%-2s") % cells(p);
            else
                out << format("%-2d") % it->second;
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_PaintTheNurikabe()
{
    using namespace puzzles::PaintTheNurikabe;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/PaintTheNurikabe.xml", "Puzzles/PaintTheNurikabe.txt", solution_format::GOAL_STATE_ONLY);
}