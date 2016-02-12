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
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move(const Position& p);
    bool is_continuous() const;

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_paths.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    string m_cells;
    vector<vector<Position>> m_paths;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g) ,m_cells(g.m_start)
{
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& a) : m_area(a) { make_move(*a.begin()); }

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

// 5. All the signposts and empty spaces must form an orthogonally continuous area.
bool puz_state::is_continuous() const
{
    set<Position> area;
    for(int r = 0; r < sidelen(); ++r)
        for(int c = 0; c < sidelen(); ++c){
            Position p(r, c);
            if(cells(p) != PUZ_LAKE)
                area.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(area, smoves);
    return smoves.size() == area.size();
}

bool puz_state::make_move(const Position& p)
{
    cells(p) = PUZ_LAKE;
    if(!is_continuous())
        return false;

    int sz = m_paths.size();
    boost::remove_erase_if(m_paths, [&](const vector<Position>& path){
        return boost::algorithm::any_of_equal(path, p);
    });

    // 4. Walls can't touch horizontally or vertically.
    for(auto& os : offset){
        auto p2 = p + os;
        char& ch = cells(p2);
        if(ch == PUZ_SPACE){
            ch = PUZ_EMPTY;
            for(auto& path : m_paths)
                boost::remove_erase(path, p2);
        }
    }
    boost::remove_erase_if(m_paths, [&](const vector<Position>& path){
        return path.empty();
    });
    m_distance = sz - m_paths.size();

    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& path = *boost::min_element(m_paths, [](
        const vector<Position>& path1, const vector<Position>& path2){
        return path1.size() < path2.size();
    });

    for(auto& p : path){
        children.push_back(*this);
        if(!children.back().make_move(p))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            char ch = cells({r, c});
            out << (ch == PUZ_SPACE ? PUZ_EMPTY : ch);
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
