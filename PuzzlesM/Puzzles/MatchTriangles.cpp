#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Matchsticks Puzzles
*/

namespace puzzles{ namespace MatchTriangles{

#define PUZ_REMOVE        0
#define PUZ_ADD           1
#define PUZ_MOVE          2

const Position offset[] = {
    {-1, 0},    // n
    {0, 1},     // e
    {1, 0},     // s
    {0, -1},    // w
};

typedef pair<Position, Position> puz_match;

struct puz_game
{
    string m_id;
    Position m_size;
    set<Position> m_dots;
    set<puz_match> m_matches;
    int m_action, m_move_count, m_triangle_count;
    set<puz_match> m_possible_matches;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
    bool is_valid_dot(const Position& p) const { return m_dots.count(p) != 0; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() / 2 + 1, strs[0].size() / 2 + 1)
    , m_move_count(level.attribute("matches").as_int())
    , m_triangle_count(level.attribute("triangles").as_int())
{
    string action = level.attribute("action").value();
    m_action = action == "add" ? PUZ_ADD : action == "move" ? PUZ_MOVE : PUZ_REMOVE;
    for(int r = 0;; ++r){
        auto& str_h = strs[r * 2];
        for(int c = 0;; ++c){
            Position p(r, c);
            if(str_h[c * 2] == '.')
                m_dots.insert(p);
            if(c == cols() - 1) break;
            if(str_h[c * 2 + 1] == '-' && is_valid_dot(p))
                m_matches.emplace(p, Position(r, c + 2));
        }
        if(r == rows() - 1) break;
        auto& str_v = strs[r * 2 + 1];
        for(int c = 0; c < cols(); ++c)
            switch(str_v[c * 2 + 1]){
            case 'A':
                m_matches.emplace(Position(r, c), Position(r + 1, c + 1));
                break;
            case '/':
                m_matches.emplace(Position(r, c + 1), Position(r + 1, c));
                break;
            }
    }
    if(m_action != PUZ_REMOVE)
        for(auto& p : m_dots){
            Position p2(p.first, p.second + 2);
            Position p3(p.first + 1, p.second - 1), p4(p.first + 1, p.second + 1);
            if(is_valid_dot(p2)) m_possible_matches.emplace(p, p2);
            if(is_valid_dot(p3)) m_possible_matches.emplace(p, p3);
            if(is_valid_dot(p4)) m_possible_matches.emplace(p, p4);
        }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool is_valid_dot(const Position& p) const { return m_game->is_valid_dot(p); }
    bool is_match(const puz_match& m) const { return m_matches.count(m) != 0; }
    void check_triangles();
    void make_move(function<void()> f);
    void make_move_remove(const puz_match& m) { make_move([&]{m_matches.erase(m);}); }
    void make_move_add(const puz_match& m) { make_move([&]{m_matches.insert(m);}); }
    void make_move_move(const puz_match& m1, const puz_match& m2) {
        make_move([&]{m_matches.erase(m1); m_matches.insert(m2);});
    }

    //solve_puzzle interface
    bool is_goal_state() const {
        return m_is_valid_state && get_heuristic() == 0;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return m_move_count * 10 + abs(m_triangle_count - m_game->m_triangle_count);
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    set<puz_match> m_matches;
    int m_move_count;
    int m_triangle_count = 0;
    bool m_is_valid_state = false;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_matches(g.m_matches), m_move_count(g.m_move_count)
{
    check_triangles();
}

void puz_state::check_triangles()
{
    m_triangle_count = 0;
    set<puz_match> matches_in_triangle;
    for(int r = 0; r < rows() - 1; ++r)
        for(int c = 0; c < cols() - 1; ++c){
            for(int n = 1;; ++n){
                for(int i = 0; i <= n; ++i){
                    if(!is_valid_dot({r, c + i * 2})) goto next_dot1;
                    if(!is_valid_dot({r + i, c + i})) goto next_dot1;
                    if(!is_valid_dot({r + i, c + n * 2 - i})) goto next_dot1;
                }
                vector<puz_match> matches;
                auto f = [&](int r1, int c1, int r2, int c2) {
                    puz_match m({r1, c1}, {r2, c2});
                    if(!is_match(m)) return false;
                    matches.push_back(m);
                    return true;
                };
                for(int i = 0; i < n; ++i){
                    if(!f(r, c + i * 2, r, c + i * 2 + 2)) goto next_triangle1;
                    if(!f(r + i, c + i, r + i + 1, c + i + 1)) goto next_triangle1;
                    if(!f(r + i, c + n * 2 - i, r + i + 1, c + n * 2 - i - 1)) goto next_triangle1;
                }
                matches_in_triangle.insert(matches.begin(), matches.end());
                ++m_triangle_count;
            next_triangle1:;
            }
        next_dot1:;
            for(int n = 1;; ++n){
                for(int i = 0; i <= n; ++i){
                    if(!is_valid_dot({r + i, c - i})) goto next_dot2;
                    if(!is_valid_dot({r + i, c + i})) goto next_dot2;
                    if(!is_valid_dot({r + n, c - n + i * 2})) goto next_dot2;
                }
                vector<puz_match> matches;
                auto f = [&](int r1, int c1, int r2, int c2) {
                    puz_match m({r1, c1}, {r2, c2});
                    if(!is_match(m)) return false;
                    matches.push_back(m);
                    return true;
                };
                for(int i = 0; i < n; ++i){
                    if(!f(r + i, c - i, r + i + 1, c - i - 1)) goto next_triangle2;
                    if(!f(r + i, c + i, r + i + 1, c + i + 1)) goto next_triangle2;
                    if(!f(r + n, c - n + i * 2, r + n, c - n + i * 2 + 2)) goto next_triangle2;
                }
                matches_in_triangle.insert(matches.begin(), matches.end());
                ++m_triangle_count;
            next_triangle2:;
            }
        next_dot2:;
        }
    m_is_valid_state = m_matches == matches_in_triangle;
}

void puz_state::make_move(function<void()> f)
{
    int d = get_heuristic();
    f();
    check_triangles();
    --m_move_count;
    m_distance = get_heuristic() - d;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if(m_move_count == 0) return;
    set<puz_match> matches;
    switch(m_game->m_action){
    case PUZ_REMOVE:
        for(auto& m : m_matches){
            children.push_back(*this);
            children.back().make_move_remove(m);
        }
        break;
    case PUZ_ADD:
        boost::set_difference(m_game->m_possible_matches, m_matches, inserter(matches, matches.end()));
        for(auto& m : matches){
            children.push_back(*this);
            children.back().make_move_add(m);
        }
        break;
    case PUZ_MOVE:
        boost::set_difference(m_game->m_possible_matches, m_matches, inserter(matches, matches.end()));
        for(auto& m1 : m_matches)
            for(auto& m2 : matches){
                children.push_back(*this);
                children.back().make_move_move(m1, m2);
            }
        break;
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0;; ++r){
        for(int c = 0; c < cols(); ++c){
            Position p(r, c);
            if(is_valid_dot(p)){
                out << '.';
                if(is_match({p, Position(r, c + 2)})){
                    out << "---";
                    ++c;
                }
                else
                    out << ' ';
            }
            else
                out << "  ";
        }
        out << endl;
        if(r == rows() - 1) break;
        for(int c = 0; c < cols(); ++c){
            out << ' ';
            out << (is_match({Position(r, c + 1), Position(r + 1, c)}) ? '/' :
                is_match({Position(r, c), Position(r + 1, c + 1)}) ? '\\' : ' ');
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_MatchTriangles()
{
    using namespace puzzles::MatchTriangles;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MatchTriangles.xml", "Puzzles/MatchTriangles.txt", solution_format::GOAL_STATE_ONLY);
}