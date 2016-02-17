#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
/*
    iOS Game: Logic Games/Puzzle Set 15/Park Lakes

    Summary
    Find the Lakes

    Description
    1. The board represents a park, where there are some hidden lakes, all square
       in shape.
    2. You have to find the lakes with the aid of hints, knowing that:
    3. A number tells you the total size of the any lakes orthogonally touching it,
       while a question mark tells you that there is at least one lake orthogonally
       touching it.
    4. Lakes aren't on tiles with numbers or question marks.
    5. All the land tiles are connected horizontally or vertically.
*/

namespace puzzles{ namespace ParkLakes{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_NUM          'N'
#define PUZ_LAKE         'L'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

    // top-left and bottom-right
typedef pair<Position, Position> puz_lake;

struct puz_lake_info
{
    int m_sum;
    vector<vector<puz_lake>> m_lake_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, puz_lake_info> m_pos2lakeinfo;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size())
{
    map<int, vector<vector<int>>> num2perms;
    for(int r = 0; r < m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c < m_sidelen; ++c){
            auto s = str.substr(c * 2, 2);
            if(s == "  ")
                m_start.push_back(PUZ_SPACE);
            else{
                m_start.push_back(PUZ_NUM);
                num2perms[m_pos2lakeinfo[{r, c}].m_sum = s == " ?" ? -1 : atoi(s.c_str())];
            }
        }
    }
    for(auto& kv : num2perms){
        int num = kv.first;
        auto& perms = kv.second;
        for(int n0 = 0; n0 < m_sidelen; ++n0)
            for(int n1 = 0; n1 < m_sidelen; ++n1)
                for(int n2 = 0; n2 < m_sidelen; ++n2)
                    for(int n3 = 0; n3 < m_sidelen; ++n3){
                        int sum = n0 * n0 + n1 * n1 + n2 * n2 + n3 * n3;
                        if(num == -1 && sum != 0 || num == sum)
                            perms.push_back({n0, n1, n2, n3});
                    }
    }
    for(auto& kv : m_pos2lakeinfo){
        auto& p0 = kv.first;
        int r0 = p0.first, c0 = p0.second;
        auto& info = kv.second;
        auto& perms = num2perms[info.m_sum];
        map<pair<int, int>, vector<puz_lake>> dirnum2lakes;
        for(auto& perm : perms){
            for(int i = 0; i < 4; ++i){
                int n = perm[i];
                if(n == 0 || dirnum2lakes.count({i, n}) != 0) continue;
                auto& lakes = dirnum2lakes[{i, n}];
                Position lake_sz(n - 1, n - 1);
                int r1 = i == 0 ? r0 - n : i == 2 ? r0 + 1 : r0 - n + 1;
                int r2 = i == 0 ? r0 - n : i == 2 ? r0 + 1 : r0;
                int c1 = i == 3 ? c0 - n : i == 1 ? c0 + 1 : c0 - n + 1;
                int c2 = i == 3 ? c0 - n : i == 1 ? c0 + 1 : c0;
                for(int r = r1; r <= r2; ++r)
                    for(int c = c1; c <= c2; ++c){
                        Position tl(r, c), br = tl + lake_sz;
                        if(tl.first >= 0 && tl.second >= 0 &&
                            br.first < m_sidelen && br.second < m_sidelen &&
                            // 4. Lakes aren't on tiles with numbers or question marks.
                            boost::algorithm::none_of(m_pos2lakeinfo, [&](
                                const pair<const Position, puz_lake_info>& kv){
                            auto& p = kv.first;
                            return
                                p.first >= tl.first && p.second >= tl.second &&
                                p.first <= br.first && p.second <= br.second;
                        }))
                            lakes.emplace_back(tl, br);
                    }
            }
        }
        for(auto& perm : perms){
            vector<int> indexes;
            vector<vector<puz_lake>> lake_perms;
            for(int i = 0; i < 4; ++i){
                int n = perm[i];
                if(n == 0) continue;
                auto lakes = dirnum2lakes[{i, n}];
                if(lakes.empty()) goto next_perm;
                lake_perms.push_back(lakes);
            }
            int cnt = lake_perms.size();
            indexes.resize(cnt);
            for(int i = 0; i < cnt;){
                vector<puz_lake> lakes;
                for(int j = 0; j < cnt; ++j)
                    lakes.push_back(lake_perms[j][indexes[j]]);

                info.m_lake_perms.push_back(lakes);
                for(i = 0; i < cnt && ++indexes[i] == lake_perms[i].size(); ++i)
                    indexes[i] = 0;
            }
        next_perm:;
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p, int n);
    bool make_move2(const Position& p, int n);
    int find_matches(bool init);
    bool is_continuous() const;

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
    map<Position, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g) ,m_cells(g.m_start)
{
    for(auto& kv : g.m_pos2lakeinfo){
        auto& lake_ids = m_matches[kv.first];
        lake_ids.resize(kv.second.m_lake_perms.size());
        boost::iota(lake_ids, 0);
    }

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        auto& p = kv.first;
        auto& lake_ids = kv.second;

        auto& lake_perms = m_game->m_pos2lakeinfo.at(p).m_lake_perms;
        boost::remove_erase_if(lake_ids, [&](int id){
            for(auto& lake : lake_perms[id])
                for(int r = lake.first.first; r <= lake.second.first; ++r)
                    for(int c = lake.first.second; c <= lake.second.second; ++c)
                        if(this->cells({r, c}) == PUZ_EMPTY)
                            return true;
            return false;
        });

        if(!init)
            switch(lake_ids.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(p, lake_ids.front()) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a, const Position& p_start)
        : m_area(a) { make_move(p_start); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>& m_area;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for(auto& os : offset){
        auto p = *this + os;
        if(m_area.count(p) != 0){
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

// 5. All the land tiles are connected horizontally or vertically.
bool puz_state::is_continuous() const
{
    set<Position> rng1;
    for(int r = 0; r < sidelen(); ++r)
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            if(cells(p) != PUZ_LAKE)
                rng1.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves({rng1, m_game->m_pos2lakeinfo.begin()->first}, smoves);
    set<Position> rng2(smoves.begin(), smoves.end()), rng3;
    boost::set_difference(rng1, rng2, inserter(rng3, rng3.end()));
    return boost::algorithm::all_of(rng3, [this](const Position& p){
        return cells(p) != PUZ_NUM;
    });
}

bool puz_state::make_move2(const Position& p, int n)
{
    auto& lakes = m_game->m_pos2lakeinfo.at(p).m_lake_perms[n];
    for(auto& lake : lakes)
        for(int r = lake.first.first; r <= lake.second.first; ++r)
            for(int c = lake.first.second; c <= lake.second.second; ++c)
                cells({r, c}) = PUZ_LAKE;
    for(auto& os : offset){
        auto p2 = p + os;
        if(!is_valid(p2)) continue;
        char& ch = cells(p2);
        if(ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
    }
    if(!is_continuous())
        return false;

    ++m_distance;
    m_matches.erase(p);
    return true;
}

bool puz_state::make_move(const Position& p, int n)
{
    m_distance = 0;
    if(!make_move2(p, n))
        return false;
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
    for(int r = 0; r < sidelen(); ++r){
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_NUM){
                int n = m_game->m_pos2lakeinfo.at(p).m_sum;
                if(n == -1)
                    out << " ?";
                else
                    out << format("%2d") % n;
            }
            else
                out << ' ' << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_ParkLakes()
{
    using namespace puzzles::ParkLakes;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\ParkLakes.xml", "Puzzles\\ParkLakes.txt", solution_format::GOAL_STATE_ONLY);
}
