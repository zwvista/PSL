#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 15/Fussy Waiter

    Summary
    Won't give you what you asked for

    Description
    1. This restaurant has a peculiar waiter. Priding himself on a math
       degree, he is very fussy about how you order.
    2. Respecting university nutrition balance, he only accepts unique
       pairings of food and drinks.
    3. Thus, a type of food can be ordered along with the same drink only 
       on a single table.
    4. Moreover, touting sudoku nutrition, he also maintains that each row
       and column of tables must have each food and drinks represented
       exactly once.
    5. He is indeed, very fussy.
*/

namespace puzzles{ namespace FussyWaiter{

#define PUZ_SPACE        ' '

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<vector<Position>> m_area2range;
    string m_start;
    vector<string> m_perms_food, m_perms_drink;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size())
    , m_area2range(m_sidelen * 4)
{
    m_start = boost::accumulate(strs, string());

    for(int r = 0; r < m_sidelen; ++r)
        for(int c = 0; c < m_sidelen; ++c){
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[c + m_sidelen].push_back(p);
            m_area2range[r + 2 * m_sidelen].push_back(p);
            m_area2range[c + 3 * m_sidelen].push_back(p);
        }

    string perm(m_sidelen, ' ');
    boost::iota(perm, 'a');
    do
        m_perms_food.push_back(perm);
    while(boost::next_permutation(perm));
    boost::iota(perm, 'A');
    do
        m_perms_drink.push_back(perm);
    while(boost::next_permutation(perm));
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char food(const Position& p) const { return (*this)[p.first * sidelen() * 2 + p.second * 2]; }
    char& food(const Position& p) { return (*this)[p.first * sidelen() * 2 + p.second * 2]; }
    char drinks(const Position& p) const { return (*this)[p.first * sidelen() * 2 + p.second * 2 + 1]; }
    char& drinks(const Position& p) { return (*this)[p.first * sidelen() * 2 + p.second * 2 + 1]; }
    string paring(const Position& p) const { return substr(p.first * sidelen() * 2 + p.second * 2, 2); }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
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
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    vector<int> perm_ids(g.m_perms_food.size());
    boost::iota(perm_ids, 0);
    for(int i = 0; i < g.m_sidelen * 4; ++i)
        m_matches[i] = perm_ids;

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        int area_id = kv.first;
        auto& perm_ids = kv.second;
        bool is_food = area_id < sidelen() * 2;
        auto& perms = is_food ? m_game->m_perms_food : m_game->m_perms_drink;

        string chars;
        for(auto& p : m_game->m_area2range[area_id])
            chars.push_back(is_food ? food(p) : drinks(p));

        boost::remove_erase_if(perm_ids, [&](int id){
            return !boost::equal(chars, perms[id], [](char ch1, char ch2){
                return ch1 == PUZ_SPACE || ch1 == ch2;
            });
        });

        if(!init)
            switch(perm_ids.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(area_id, perm_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    auto& range = m_game->m_area2range[i];
    bool is_food = i < sidelen() * 2;
    auto& perms = is_food ? m_game->m_perms_food : m_game->m_perms_drink;
    auto& perm = perms[j];

    for(int k = 0; k < perm.size(); ++k){
        auto p = range[k];
        (is_food ? food(p) : drinks(p)) = perm[k];
    }

    set<string> parings;
    for(int r = 0; r < sidelen(); ++r)
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            auto fd = paring(p);
            if(fd[0] != PUZ_SPACE && fd[1] != PUZ_SPACE){
                if(parings.count(fd) != 0)
                    return false;
                parings.insert(fd);
            }
        }

    ++m_distance;
    m_matches.erase(i);
    return true;
}

bool puz_state::make_move(int i, int j)
{
    m_distance = 0;
    if(!make_move2(i, j))
        return false;
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
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            out << food(p) << drinks(p) << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_FussyWaiter()
{
    using namespace puzzles::FussyWaiter;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/FussyWaiter.xml", "Puzzles/FussyWaiter.txt", solution_format::GOAL_STATE_ONLY);
}