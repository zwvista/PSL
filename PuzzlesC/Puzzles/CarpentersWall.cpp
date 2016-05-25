#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 12/Carpenter's Wall

    Summary
    Angled Walls

    Description
    1. In this game you have to create a valid Nurikabe following a different
       type of hints.
    2. In the end, the empty spaces left by the Nurikabe will form many Carpenter's
       Squares (L shaped tools) of different size.
    3. The circled numbers on the board indicate the corner of the L.
    4. When a number is inside the circle, that indicates the total number of
       squares occupied by the L.
    5. The arrow always sits at the end of an arm and points to the corner of
       an L.
    6. Not all the Carpenter's Squares might be indicated: some could be hidden
       and no hint given.
*/

namespace puzzles{ namespace CarpentersWall{

#define PUZ_SPACE       ' '
#define PUZ_WALL        'W'
#define PUZ_BOUNDARY    '+'
#define PUZ_CORNER      'o'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},         // e
    {1, 0},         // s
    {0, -1},        // w
};

const vector<vector<int>> tool_dirs2 = {
    {0, 1}, {0, 3}, {1, 2}, {2, 3}
};

const string tool_dirs = "^>v<";

enum class tool_hint_type {
    CORNER,
    NUMBER,
    ARM_END
};

struct puz_tool
{
    Position m_hint_pos;
    char m_hint = PUZ_CORNER;
    puz_tool() {}
    puz_tool(const Position& p, char ch) : m_hint_pos(p), m_hint(ch) {}
    tool_hint_type hint_type() const {
        return m_hint == PUZ_CORNER ? tool_hint_type::CORNER :
            dir() != -1 ? tool_hint_type::ARM_END : tool_hint_type::NUMBER;
    }
    int len() const { return isdigit(m_hint) ? m_hint - '0' : m_hint - 'A' + 10; }
    int dir() const { return tool_dirs.find(m_hint); }
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2ch;
    map<char, puz_tool> m_ch2tool;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
: m_id(attrs.get<string>("id"))
, m_sidelen(strs.size() + 2)
{
    char ch_t = 'a';
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for(int r = 1; r < m_sidelen - 1; ++r){
        auto& str = strs[r - 1];
        m_start.push_back(PUZ_BOUNDARY);
        for(int c = 1; c < m_sidelen - 1; ++c){
            char ch = str[c - 1];
            if(ch == PUZ_SPACE)
                m_start.push_back(PUZ_SPACE);
            else{
                Position p(r, c);
                m_pos2ch[p] = ch;
                m_ch2tool[ch_t] = {p, ch};
                m_start.push_back(ch_t++);
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
    bool make_move(char ch, int n);
    bool make_move2(char ch, int n);
    bool make_move_hidden(char ch, int n);
    int adjust_area(bool init);
    bool is_continuous() const;
    const puz_tool& get_tool(char ch) const {
        auto it = m_game->m_ch2tool.find(ch);
        return it == m_game->m_ch2tool.end() ? m_next_tool : it->second;
    }
    void add_tool(const Position& p) {
        m_next_tool.m_hint_pos = p;
        m_matches[m_next_ch];
        adjust_area(true);
    }

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0 && m_2by2walls.empty();}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<char, vector<vector<Position>>> m_matches;
    vector<set<Position>> m_2by2walls;
    puz_tool m_next_tool;
    char m_next_ch;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_start), m_game(&g)
, m_next_ch('a' + g.m_ch2tool.size())
{
    for(int r = 1; r < g.m_sidelen - 2; ++r)
        for(int c = 1; c < g.m_sidelen - 2; ++c){
            set<Position> rng{{r, c}, {r, c + 1}, {r + 1, c}, {r + 1, c + 1}};
            if(boost::algorithm::all_of(rng, [&](const Position& p){
                return cells(p) == PUZ_SPACE;
            }))
                m_2by2walls.push_back(rng);
        }

    for(auto& kv : g.m_ch2tool)
        m_matches[kv.first];

    adjust_area(true);
}

int puz_state::adjust_area(bool init)
{
    for(auto& kv : m_matches){
        char ch = kv.first;
        auto& ranges = kv.second;
        ranges.clear();
        auto& t = get_tool(ch);
        auto& p = t.m_hint_pos;

        auto f1 = [&](const Position& p2){
            if(cells(p2) != PUZ_SPACE)
                return 0;
            for(int i = 0; i < 4; ++i){
                char ch2 = this->cells(p2 + offset[i]);
                if(ch2 != PUZ_BOUNDARY && ch2 != PUZ_SPACE &&
                    ch2 != PUZ_WALL && ch2 != ch)
                    return ~i;
            }
            return 1;
        };

        auto f2 = [&](const vector<Position>& a0, const vector<Position>& a1, int i, int j){
            vector<Position> rng;
            for(int k = 0; k < i; ++k)
                rng.push_back(a0[k]);
            for(int k = 0; k < j; ++k)
                rng.push_back(a1[k]);
            rng.push_back(p);
            boost::sort(rng);
            ranges.push_back(rng);
        };

        auto g1 = [&](int len){
            vector<vector<Position>> arms(4);
            vector<vector<int>> indexes(4);
            Position p2;
            int n;
            for(int i = 0; i < 4; ++i){
                auto& os = offset[i];
                int j = 1;
                for(p2 = p + os; (n = f1(p2)) == 1; p2 += os){
                    arms[i].push_back(p2);
                    indexes[i].push_back(j++);
                }
                if(~n != i) continue;
                auto p3 = p2 + os;
                auto& t = get_tool(this->cells(p3));
                if(t.hint_type() == tool_hint_type::ARM_END &&
                    (t.dir() + 2) % 4 == i){
                    arms[i].push_back(p2);
                    arms[i].push_back(p3);
                    indexes[i].push_back(++j);
                }
            }
            for(auto& dirs : tool_dirs2){
                auto &a0 = arms[dirs[0]], &a1 = arms[dirs[1]];
                auto &ids0 = indexes[dirs[0]], &ids1 = indexes[dirs[1]];
                if(a0.empty() || a1.empty()) continue;
                for(int i : ids0)
                    for(int j : ids1)
                        if(len == -1 || i + j + 1 == len)
                            f2(a0, a1, i, j);
            }
        };

        auto g2 = [&](int d){
            vector<Position> a0, a1;
            auto& os = offset[d];
            int d21 = (d + 3) % 4, d22 = (d + 1) % 4;
            Position p2;
            int n, len = -1;
            for(p2 = p + os; ; p2 += os){
                n = f1(p2);
                if(n != 1 && ~n != d) break;
                if(n == 1)
                    a0.push_back(p2);
                else{
                    auto p3 = p2 + os;
                    auto& t = get_tool(this->cells(p3));
                    auto ht = t.hint_type();
                    if(ht == tool_hint_type::ARM_END) break;
                    if(ht == tool_hint_type::NUMBER)
                        len = t.len();
                    a0.push_back(p2);
                    a0.push_back(p2 = p3);
                }
                char ch_corner = cells(p2);
                int i = a0.size();
                for(int d2 : {d21, d22}){
                    auto& os2 = offset[d2];
                    a1.clear();
                    Position p3;
                    cells(p2) = ch;
                    for(p3 = p2 + os2; (n = f1(p3)) == 1; p3 += os2)
                        a1.push_back(p3);
                    cells(p2) = ch_corner;
                    vector<int> indexes;
                    int j;
                    for(j = 1; j <= a1.size(); ++j)
                        indexes.push_back(j);
                    if(~n == d2){
                        auto p4 = p3 + offset[d2];
                        char ch2 = this->cells(p4);
                        auto& t = get_tool(ch2);
                        if(t.hint_type() == tool_hint_type::ARM_END &&
                            (t.dir() + 2) % 4 == d2){
                            a1.push_back(p3);
                            a1.push_back(p4);
                            indexes.push_back(++j);
                        }
                    }
                    if(a1.empty()) continue;
                    for(int j : indexes)
                        if(len == -1 || i + j + 1 == len)
                            f2(a0, a1, i, j);
                }
                if(ch_corner != PUZ_SPACE && ch_corner != ch) break;
            }
            if(n == 0) return;
            n = ~n;
            if(n == d21 || n == d22){
                auto p3 = p2 + offset[n];
                auto& t = get_tool(this->cells(p3));
                if(t.hint_type() == tool_hint_type::ARM_END &&
                    (t.dir() + 2) % 4 == n){
                    a0.push_back(p2);
                    a1 = {p3};
                    f2(a0, a1, a0.size(), 1);
                }
            }
        };

        switch(t.hint_type()){
        case tool_hint_type::CORNER:
            g1(-1);
            break;
        case tool_hint_type::NUMBER:
            g1(t.len());
            break;
        case tool_hint_type::ARM_END:
            g2(t.dir());
            break;
        }

        if(!init)
            switch(ranges.size()){
            case 0:
                return 0;
            case 1:
                return make_move2(ch, 0) ? 1 : 0;
            }
    }
    return 2;
}

struct puz_state2 : Position
{
    puz_state2(const set<Position>& rng) : m_rng(&rng) { make_move(*rng.begin()); }

    void make_move(const Position& p){ static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position>* m_rng;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for(auto& os : offset){
        auto p2 = *this + os;
        if(m_rng->count(p2) != 0){
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::is_continuous() const
{
    set<Position> a;
    for(int r = 1; r < sidelen() - 1; ++r)
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_SPACE || ch == PUZ_WALL)
                a.insert(p);
        }

    list<puz_state2> smoves;
    puz_move_generator<puz_state2>::gen_moves(a, smoves);
    return smoves.size() == a.size();
}

bool puz_state::make_move2(char ch, int n)
{
    auto& t = get_tool(ch);
    auto& rng = m_matches.at(ch)[n];

    set<char> chars;
    for(auto& p : rng)
        chars.insert(cells(p));
    chars.erase(PUZ_SPACE);
    if(chars.empty())
        chars.insert(ch);

    for(auto& p : rng){
        cells(p) = ch;
        for(auto& os : offset){
            char& ch2 = cells(p + os);
            if(ch2 == PUZ_SPACE)
                ch2 = PUZ_WALL;
        }
        boost::remove_erase_if(m_2by2walls, [&](const set<Position>& rng2){
            return rng2.count(p) != 0;
        });
    }
    for(char ch2 : chars){
        m_matches.erase(ch2);
        ++m_distance;
    }

    return is_continuous();
}

bool puz_state::make_move(char ch, int n)
{
    m_distance = 0;
    if(!make_move2(ch, n))
        return false;
    int m;
    while((m = adjust_area(false)) == 1);
    return m == 2;
}

bool puz_state::make_move_hidden(char ch, int n)
{
    if(!make_move2(ch, n))
        return false;
    m_next_ch++;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if(!m_matches.empty()){
        auto& kv = *boost::min_element(m_matches, [](
            const pair<const char, vector<vector<Position>>>& kv1,
            const pair<const char, vector<vector<Position>>>& kv2){
            return kv1.second.size() < kv2.second.size();
        });
        for(int i = 0; i < kv.second.size(); ++i){
            children.push_back(*this);
            if(!children.back().make_move(kv.first, i))
                children.pop_back();
        }
    }
    else{
        set<Position> rng;
        for(int r = 1; r < sidelen() - 1; ++r)
            for(int c = 1; c < sidelen() - 1; ++c){
                Position p(r, c);
                char ch = cells(p);
                if(ch != PUZ_SPACE) continue;
                auto s = *this;
                s.add_tool(p);
                auto& kv = *s.m_matches.begin();
                for(auto& rng2 : kv.second)
                    rng.insert(rng2.begin(), rng2.end());
                for(int i = 0; i < kv.second.size(); ++i){
                    children.push_back(s);
                    if(!children.back().make_move_hidden(kv.first, i))
                        children.pop_back();
                }
            }

        // pruning
        if(boost::algorithm::any_of(m_2by2walls, [&](const set<Position>& rng2){
            return boost::algorithm::none_of(rng2, [&](const Position& p){
                return rng.count(p) != 0;
            });
        }))
            children.clear();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 1; r < sidelen() - 1; ++r){
        for(int c = 1; c < sidelen() - 1; ++c){
            Position p(r, c);
            char ch = cells(p);
            if(ch == PUZ_SPACE || ch == PUZ_WALL)
                out << PUZ_WALL << ' ';
            else{
                auto it = m_game->m_pos2ch.find(p);
                if(it == m_game->m_pos2ch.end())
                    out << ". ";
                else{
                    char ch = it->second;
                    if(isupper(ch))
                        out << ch - 'A' + 10;
                    else
                        out << ch << ' ';
                }
            }
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_CarpentersWall()
{
    using namespace puzzles::CarpentersWall;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles\\CarpentersWall.xml", "Puzzles\\CarpentersWall.txt", solution_format::GOAL_STATE_ONLY);
}