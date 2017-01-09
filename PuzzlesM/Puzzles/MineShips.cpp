#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 8/Mine Ships

    Summary
    Warning! Naval Mines in the water!

    Description
    1. There are actually no mines in the water, but this is a mix between
       Minesweeper and Battle Ships.
    2. You must find the same set of ships like 'Battle Ships'
       (1*4, 2*3, 3*2, 4*1).
    3. However this time the hints are given in the same form as 'Minesweeper',
       where a number tells you how many pieces of ship are around it.
    4. Usual Battle Ships rules apply!
*/

namespace puzzles{ namespace MineShips{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_NUMBER        'N'

struct puz_ship_info {
    // symbols that represent the ship
    string m_pieces[2];
    // the area occupied by the ship
    string m_area[3];
};

const puz_ship_info ship_info[] = {
    {{"o", "o"}, {"111", "1 1", "111"}},
    {{"<>", "^v"}, {"1221", "1  1", "1221"}},
    {{"<+>", "^+v"}, {"12321", "1   1", "12321"}},
    {{"<++>", "^++v"}, {"123321", "1    1", "123321"}},
    {{"<+++>", "^+++v"}, {"1233321", "1     1", "1233321"}},
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    bool m_has_supertanker;
    string m_start;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, int> m_pos2num;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size())
    , m_has_supertanker(attrs.get<int>("SuperTanker", 0) == 1)
{
    if(m_has_supertanker)
        m_ship2num[5] = 1;

    for(int r = 0; r < m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c < m_sidelen; ++c){
            char ch = str[c];
            m_start.push_back(ch == PUZ_SPACE ? PUZ_SPACE : PUZ_NUMBER);
            if(ch != PUZ_SPACE)
                m_pos2num[{r, c}] = ch - '0';
        }
    }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, int n, bool vert);
    void find_matches();

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv){
            return acc + kv.second;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<int, int> m_ship2num;
    map<Position, int> m_pos2num;
    map<int, vector<pair<Position, bool>>> m_matches;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_ship2num(g.m_ship2num), m_pos2num(g.m_pos2num)
{
    find_matches();
}

void puz_state::find_matches()
{
    m_matches.clear();
    for(const auto& kv : m_ship2num){
        int i = kv.first;
        for(int j = 0; j < 2; ++j){
            bool vert = j == 1;
            if(i == 1 && vert)
                continue;

            auto& info = ship_info[i - 1];
            int len = info.m_pieces[0].length();
            for(int r = -1; r < sidelen() - (!vert ? 1 : len); ++r)
                for(int c = -1; c < sidelen() - (vert ? 1 : len); ++c)
                    if([&]{
                        for(int r2 = 0; r2 < 3; ++r2)
                            for(int c2 = 0; c2 < len + 2; ++c2){
                                char ch2 = info.m_area[r2][c2];
                                Position p2(r + (!vert ? r2 : c2), c + (!vert ? c2 : r2));
                                if(!is_valid(p2))
                                    if(ch2 == ' ')
                                        return false;
                                    else
                                        continue;
                                char ch = cells(p2);
                                if(ch2 == ' ')
                                    if(ch == PUZ_SPACE)
                                        continue;
                                    else
                                        return false;
                                if(ch != PUZ_NUMBER)
                                    continue;
                                int n1 = m_pos2num.at(p2), n2 = ch2 - '0';
                                if(n1 < n2)
                                    return false;
                            }
                        return true;
                    }())
                        m_matches[i].push_back({{r, c}, vert});
        }
    }
}

bool puz_state::make_move(const Position& p, int n, bool vert)
{
    auto& info = ship_info[n - 1];
    int len = info.m_pieces[0].length();
    for(int r2 = 0; r2 < 3; ++r2)
        for(int c2 = 0; c2 < len + 2; ++c2){
            auto p2 = p + Position(!vert ? r2 : c2, !vert ? c2 : r2);
            if(!is_valid(p2)) continue;
            char& ch = cells(p2);
            char ch2 = info.m_area[r2][c2];
            if(ch2 == ' ')
                ch = info.m_pieces[!vert ? 0 : 1][c2 - 1];
            else if(ch == PUZ_NUMBER)
                m_pos2num.at(p2) -= (ch2 - '0');
            else if(ch == PUZ_SPACE)
                ch = PUZ_EMPTY;
        }

    if(--m_ship2num[n] == 0)
        m_ship2num.erase(n);
    find_matches();

    if(!is_goal_state())
        return boost::algorithm::all_of(m_ship2num, [&](const pair<const int, int>& kv){
            return m_matches[kv.first].size() >= kv.second;
        });
    else
        return boost::algorithm::all_of(m_pos2num, [&](const pair<const Position, int>& kv){
            return kv.second == 0;
        });
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_matches, [](
        const pair<const int, vector<pair<Position, bool>>>& kv1,
        const pair<const int, vector<pair<Position, bool>>>& kv2){
        return kv1.second.size() < kv2.second.size();
    });
    for(auto& kv2 : m_matches.at(kv.first)){
        children.push_back(*this);
        if(!children.back().make_move(kv2.first, kv.first, kv2.second))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_NUMBER)
                out << format("%-2d") % m_game->m_pos2num.at(p);
            else{
                if(ch == PUZ_SPACE)
                    ch = PUZ_EMPTY;
                out << format("%-2s") % ch;
            }
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_MineShips()
{
    using namespace puzzles::MineShips;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MineShips.xml", "Puzzles/MineShips.txt", solution_format::GOAL_STATE_ONLY);
}
