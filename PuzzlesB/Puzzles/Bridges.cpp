#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 7/Bridges

    Summary
    Enough Sudoku, let's build!

    Description
    1. The board represents a Sea with some islands on it.
    2. You must connect all the islands with Bridges, making sure every
       island is connected to each other with a Bridges path.
    3. The number on each island tells you how many Bridges are touching
       that island.
    4. Bridges can only run horizontally or vertically and can't cross
       each other.
    5. Lastly, you can connect two islands with either one or two Bridges
       (or none, of course)
*/

namespace puzzles{ namespace Bridges{

#define PUZ_SPACE            ' '
#define PUZ_NUMBER            'N'
#define PUZ_HORZ_1            '-'
#define PUZ_VERT_1            '|'
#define PUZ_HORZ_2            '='
#define PUZ_VERT_2            'H'
#define PUZ_BOUNDARY        'B'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2num;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(int r = 1; r < m_sidelen - 1; ++r){
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for(int c = 1; c < m_sidelen - 1; ++c){
            char ch = str[c - 1];
            if(ch == PUZ_SPACE)
                m_start.push_back(ch);
            else{
                m_start.push_back(PUZ_NUMBER);
                m_pos2num[{r, c}] = ch - '0';
            }
        }
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, const vector<int>& perm);
    bool make_move2(const Position& p, const vector<int>& perm);
    int find_matches(bool init);
    bool is_connected() const;

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
    // key: the position of the number
    // value.elem: the numbers of the bridges connected to the number
    //             in all the four directions
    map<Position, vector<vector<int>>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
{
    for(auto& kv : g.m_pos2num)
        m_matches[kv.first];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        const auto& p = kv.first;
        auto& perms = kv.second;
        perms.clear();

        int sum = m_game->m_pos2num.at(p);
        vector<vector<int>> dir_nums(4);
        for(int i = 0; i < 4; ++i){
            bool is_horz = i % 2 == 1;
            auto& os = offset[i];
            auto& nums = dir_nums[i];
            // none for the other cases
            nums = {0};
            for(auto p2 = p + os; ; p2 += os){
                char ch = cells(p2);
                if(ch == PUZ_NUMBER){
                    if(m_matches.count(p2) != 0)
                        // one, two bridges or none
                        nums = {0, 1, 2};
                }
                else if(ch == PUZ_HORZ_1 || ch == PUZ_HORZ_2){
                    if(is_horz)
                        // already connected
                        nums = {ch == PUZ_HORZ_1 ? 1 : 2};
                }
                else if(ch == PUZ_VERT_1 || ch == PUZ_VERT_2){
                    if(!is_horz)
                        // already connected
                        nums = {ch == PUZ_VERT_1 ? 1 : 2};
                }
                if(ch != PUZ_SPACE)
                    break;
            }
        }

        for(int n0 : dir_nums[0])
            for(int n1 : dir_nums[1])
                for(int n2 : dir_nums[2])
                    for(int n3 : dir_nums[3])
                        if(n0 + n1 + n2 + n3 == sum)
                            perms.push_back({n0, n1, n2, n3});

        if(!init)
            switch(perms.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(p, perms.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const puz_state& s);

    int sidelen() const { return m_state->sidelen(); }
    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const puz_state* m_state;
};

puz_state2::puz_state2(const puz_state& s)
: m_state(&s)
{
    int i = s.find(PUZ_NUMBER);
    make_move({i / sidelen(), i % sidelen()});
}

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for(int i = 0; i < 4; ++i){
        auto& os = offset[i];
        bool is_horz = i % 2 == 1;
        auto p2 = *this + os;
        char ch = m_state->cells(p2);
        if(is_horz && (ch == PUZ_HORZ_1 || ch == PUZ_HORZ_2) ||
            !is_horz && (ch == PUZ_VERT_1 || ch == PUZ_VERT_2)){
            while(m_state->cells(p2) != PUZ_NUMBER)
                p2 += os;
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_connected() const
{
    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(*this, smoves);
    return smoves.size() == boost::count(*this, PUZ_NUMBER);
}

bool puz_state::make_move2(const Position& p, const vector<int>& perm)
{
    for(int i = 0; i < 4; ++i){
        bool is_horz = i % 2 == 1;
        auto& os = offset[i];
        int n = perm[i];
        if(n == 0)
            continue;
        for(auto p2 = p + os; ; p2 += os){
            char& ch = cells(p2);
            if(ch != PUZ_SPACE)
                break;
            ch = is_horz ? n == 1 ? PUZ_HORZ_1 : PUZ_HORZ_2
                : n == 1 ? PUZ_VERT_1 : PUZ_VERT_2;
        }
    }

    ++m_distance;
    m_matches.erase(p);

    return !is_goal_state() || is_connected();
}

bool puz_state::make_move(const Position& p, const vector<int>& perm)
{
    m_distance = 0;
    if(!make_move2(p, perm))
        return false;
    int m;
    while((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const Position, vector<vector<int>>>& kv1,
        const pair<const Position, vector<vector<int>>>& kv2){
        return kv1.second.size() < kv2.second.size();
    });

    for(auto& perm : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, perm))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_NUMBER)
                out << format("%2d") % m_game->m_pos2num.at(p);
            else
                out << ' ' << ch;
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Bridges()
{
    using namespace puzzles::Bridges;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\Bridges.xml", "Puzzles\\Bridges.txt", solution_format::GOAL_STATE_ONLY);
}
