#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games/Puzzle Set 7/Gardener

    Summary
    Hitori Flower Planting

    Description
    1. The Board represents a Garden, divided in many rectangular Flowerbeds.
    2. The owner of the Garden wants you to plant Flowers according to these
       rules.
    3. A number tells you how many Flowers you must plant in that Flowerbed.
       A Flowerbed without number can have any quantity of Flowers.
    4. Flowers can't be horizontally or vertically touching.
    5. All the remaining Garden space where there are no Flowers must be
       interconnected (horizontally or vertically), as he wants to be able
       to reach every part of the Garden without treading over Flowers.
    6. Lastly, there must be enough balance in the Garden, so a straight
       line (horizontally or vertically) of non-planted tiles can't span
       for more than two Flowerbeds.
    7. In other words, a straight path of empty space can't pass through
       three or more Flowerbeds.
*/

namespace puzzles::Gardener{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';
constexpr auto PUZ_FLOWER = 'F';
constexpr auto PUZ_BOUNDARY = '"';

constexpr auto PUZ_FLOWER_COUNT_UNKNOWN = -1;

bool is_empty(char ch) { return ch == PUZ_SPACE || ch == PUZ_EMPTY; }

constexpr Position offset[] = {
    {-1, 0},       // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},       // w
};

constexpr Position offset2[] = {
    {0, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, 0},        // w
};

struct puz_fb_info
{
    vector<Position> m_range;
    int m_flower_count = PUZ_FLOWER_COUNT_UNKNOWN;
    vector<string> m_perms;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, int> m_pos2fb;
    vector<puz_fb_info> m_fb_info;
    map<Position, int> m_pos2num;
    set<Position> m_horz_walls, m_vert_walls;

    puz_game(const vector<string>& strs, const xml_node& level);
};

struct puz_state2 : Position
{
    puz_state2(const set<Position>& horz_walls, const set<Position>& vert_walls, const Position& p_start)
        : m_horz_walls(&horz_walls), m_vert_walls(&vert_walls) {
        make_move(p_start);
    }

    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state2>& children) const;

    const set<Position> *m_horz_walls, *m_vert_walls;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (int i = 0; i < 4; ++i) {
        auto p = *this + offset[i];
        auto p_wall = *this + offset2[i];
        auto& walls = i % 2 == 0 ? *m_horz_walls : *m_vert_walls;
        if (!walls.contains(p_wall)) {
            children.push_back(*this);
            children.back().make_move(p);
        }
    }
}

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() / 2 + 2)
{
    set<Position> rng;
    for (int r = 1;; ++r) {
        // horizontal walls
        auto& str_h = strs[r * 2 - 2];
        for (int c = 1; c < m_sidelen - 1; ++c)
            if (str_h[c * 2 - 1] == '-')
                m_horz_walls.insert({r, c});
        if (r == m_sidelen - 1) break;
        auto& str_v = strs[r * 2 - 1];
        for (int c = 1;; ++c) {
            Position p(r, c);
            // vertical walls
            if (str_v[c * 2 - 2] == '|')
                m_vert_walls.insert(p);
            if (c == m_sidelen - 1) break;
            char ch = str_v[c * 2 - 1];
            if (ch != ' ')
                m_pos2num[p] = ch - '0';
            rng.insert(p);
        }
    }

    for (int n = 0; !rng.empty(); ++n) {
        list<puz_state2> smoves;
        auto& p_start = *rng.begin();
        puz_move_generator<puz_state2>::gen_moves({m_horz_walls, m_vert_walls, p_start}, smoves);
        m_fb_info.resize(n + 1);
        if (auto it = m_pos2num.find(p_start); it != m_pos2num.end())
            m_fb_info[n].m_flower_count = it->second;
        for (auto& p : smoves) {
            m_fb_info[n].m_range.push_back(p);
            m_pos2fb[p] = n;
            rng.erase(p);
        }
        boost::sort(m_fb_info[n].m_range);
    }

    map<pair<int, int>, vector<string>> pair2perms;
    for (auto& info : m_fb_info) {
        int pos_cnt = info.m_range.size();
        int flower_cnt = info.m_flower_count;
        auto& perms = pair2perms[{pos_cnt, flower_cnt}];
        if (perms.empty())
            for (int i = 0; i <= pos_cnt; ++i) {
                if (flower_cnt != PUZ_FLOWER_COUNT_UNKNOWN && flower_cnt != i) continue;
                auto perm = string(pos_cnt - i, PUZ_EMPTY) + string(i, PUZ_FLOWER);
                do
                    perms.push_back(perm);
                while (boost::next_permutation(perm));
            }
        for (const auto& perm : perms) {
            vector<Position> ps_flower;
            for (int i = 0; i < perm.size(); ++i)
                if (perm[i] == PUZ_FLOWER)
                    ps_flower.push_back(info.m_range[i]);

            if ([&]{
                // no touching
                for (const auto& p1 : ps_flower)
                    for (const auto& p2 : ps_flower)
                        if (boost::algorithm::any_of_equal(offset, p1 - p2))
                            return false;
                return true;
            }())
                info.m_perms.push_back(perm);
        }
    }

    boost::sort(m_fb_info, [this](const puz_fb_info& info1, const puz_fb_info& info2) {
        return info1.m_perms.size() < info2.m_perms.size();
    });
}

struct puz_state : string
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i);
    void count_unbalanced();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_game->m_fb_info.size() - m_fb_index + m_unbalanced; 
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    int m_fb_index = 0;
    unsigned int m_distance = 0;
    unsigned int m_unbalanced = 0;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
    for (int i = 0; i < sidelen(); ++i)
        cells({i, 0}) = cells({i, sidelen() - 1}) =
        cells({0, i}) = cells({sidelen() - 1, i}) = PUZ_BOUNDARY;

    count_unbalanced();
}

void puz_state::count_unbalanced()
{
    auto f = [this](Position p, const Position& os) {
        char ch_last = PUZ_BOUNDARY;
        int fb_last = -1;
        for (int i = 1, n = 0; i < sidelen(); ++i) {
            char ch = cells(p);
            if (is_empty(ch)) {
                int fb = m_game->m_pos2fb.at(p);
                if (fb != fb_last)
                    ++n, fb_last = fb;
            } else {
                fb_last = -1;
                if (n > 2)
                    m_unbalanced += n - 2;
                n = 0;
            }
            ch_last = ch;
            p += os;
        }
    };

    m_unbalanced = 0;
    for (int i = 1; i < sidelen() - 1; ++i) {
        f({i, 1}, {0, 1});        // e
        f({1, i}, {1, 0});        // s
    }
}

struct puz_state3 : Position
{
    puz_state3(const puz_state& s);

    int sidelen() const { return m_state->sidelen(); }
    void make_move(const Position& p) { static_cast<Position&>(*this) = p; }
    void gen_children(list<puz_state3>& children) const;

    const puz_state* m_state;
};

puz_state3::puz_state3(const puz_state& s)
: m_state(&s)
{
    int i = boost::find_if(s, [](char ch) {
        return is_empty(ch);
    }) - s.begin();

    make_move({i / sidelen(), i % sidelen()});
}

void puz_state3::gen_children(list<puz_state3>& children) const
{
    for (auto& os : offset) {
        auto p2 = *this + os;
        if (is_empty(m_state->cells(p2))) {
            children.push_back(*this);
            children.back().make_move(p2);
        }
    }
}

bool puz_state::make_move(int i)
{
    m_distance = 0;
    auto& info = m_game->m_fb_info[m_fb_index];
    auto& range = info.m_range;
    auto& perm = info.m_perms[i];

    auto ub = m_unbalanced;
    for (int k = 0; k < perm.size(); ++k) {
        auto& p = range[k];
        if ((cells(p) = perm[k]) != PUZ_FLOWER) continue;

        // no touching
        for (auto& os : offset)
            if (cells(p + os) == PUZ_FLOWER)
                return false;
    }

    // interconnected spaces
    list<puz_state3> smoves;
    puz_move_generator<puz_state3>::gen_moves(*this, smoves);
    if (smoves.size() != boost::count_if(*this, [](char ch) {
        return is_empty(ch);
    }))
        return false;

    count_unbalanced();
    m_distance += ub - m_unbalanced + 1;
    return ++m_fb_index != m_game->m_fb_info.size() || m_unbalanced == 0;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int sz = m_game->m_fb_info[m_fb_index].m_perms.size();
    for (int i = 0; i < sz; ++i) {
        children.push_back(*this);
        if (!children.back().make_move(i))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1;; ++r) {
        // draw horizontal walls
        for (int c = 1; c < sidelen() - 1; ++c)
            out << (m_game->m_horz_walls.contains({r, c}) ? " -" : "  ");
        println(out);
        if (r == sidelen() - 1) break;
        for (int c = 1;; ++c) {
            Position p(r, c);
            // draw vertical walls
            out << (m_game->m_vert_walls.contains(p) ? '|' : ' ');
            if (c == sidelen() - 1) break;
            out << cells(p);
        }
        println(out);
    }
    return out;
}

}

void solve_puz_Gardener()
{
    using namespace puzzles::Gardener;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Gardener.xml", "Puzzles/Gardener.txt", solution_format::GOAL_STATE_ONLY);
}
