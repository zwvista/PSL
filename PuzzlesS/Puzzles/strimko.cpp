#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles{ namespace strimko{

#define PUZ_SPACE        ' '

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    map<Position, char> m_pos2area;
    vector<vector<Position>> m_areas;
    vector<string> m_perms;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() / 2)
{
    m_start = accumulate(strs.begin(), strs.begin() + m_sidelen, string());
    m_areas.resize(m_sidelen * 3);
    for(int r = 0; r < m_sidelen; ++r){
        auto& str = strs[r + m_sidelen];
        for(int c = 0; c < m_sidelen; ++c){
            Position p(r, c);
            int n = str[c] - 'a';
            m_pos2area[p] = n;
            for(int i : {n, m_sidelen + p.first, m_sidelen * 2 + p.second})
                m_areas[i].push_back(p);
        }
    }

    string perm(m_sidelen, PUZ_SPACE);
    boost::iota(perm, '1');
    do 
        m_perms.push_back(perm);
    while(boost::next_permutation(perm));
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(int i, int j);
    void make_move2(int i, int j);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const { out << endl; }
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    string m_cells;
    map<int, vector<int>> m_matches;
    int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_cells(g.m_start), m_game(&g)
{
    for(int i = 0; i < sidelen() * 3; ++i){
        auto& perm_ids = m_matches[i];
        perm_ids.resize(g.m_perms.size());
        boost::iota(perm_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        int area_id = kv.first;
        auto& perm_ids = kv.second;

        string nums;
        for(auto& p : m_game->m_areas[area_id])
            nums.push_back(cells(p));

        auto& perms = m_game->m_perms;
        boost::remove_erase_if(perm_ids, [&](int id){
            return !boost::equal(nums, perms[id], [](char ch1, char ch2){
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if(!init)
            switch(perm_ids.size()){
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
    auto& area = m_game->m_areas[i];
    auto& perm = m_game->m_perms[j];

    for(int k = 0; k < perm.size(); ++k)
        cells(area[k]) = perm[k];

    ++m_distance;
    m_matches.erase(i);
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    make_move2(i, j);
    int m;
    while((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<int>>& kv1,
        const pair<const int, vector<int>>& kv2){
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
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c)
            out << cells({r, c});
        out << endl;
    }
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c)
            out << char(m_game->m_pos2area.at({r, c}) + 'a');
        out << endl;
    }
    return out;
}

}}

void solve_puz_strimko()
{
    using namespace puzzles::strimko;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/strimko.xml", "Puzzles/strimko.txt", solution_format::GOAL_STATE_ONLY);
}