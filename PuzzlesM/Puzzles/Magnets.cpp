#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 2/Magnets

    Summary
    Place Magnets on the board, respecting the orientation of poles

    Description
    1. Each Magnet has a positive(+) and a negative(-) pole.
    2. Every rectangle can either contain a Magnet or be empty.
    3. The number on the board tells you how many positive and negative poles
       you can see from there in a straight line.
    4. When placing a Magnet, you have to respect the rule that the same pole
       (+ and + / - and -) can't be adjacent horizontally or vertically.
    5. In some levels, a few numbers on the border can be hidden.
*/

namespace puzzles{ namespace Magnets{

#define PUZ_HORZ        'H'
#define PUZ_VERT        'V'
#define PUZ_SPACE        ' '
#define PUZ_POSITIVE    '+'
#define PUZ_NEGATIVE    '-'
#define PUZ_EMPTY        '.'
#define PUZ_UNKNOWN        9999

const Position offset[] = {
    {0, 1},
    {1, 0},
    {0, -1},
    {-1, 0},
};

typedef vector<char> puz_chars;
const puz_chars all_chars = {PUZ_POSITIVE, PUZ_NEGATIVE, PUZ_EMPTY};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_rect_count;
    vector<vector<Position>> m_area_info;
    map<Position, int> m_pos2rect;
    vector<array<int, 2>> m_num_poles_rows, m_num_poles_cols;
    string m_start;

    puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level);
    char cells(const Position& p) const { return m_start[p.first * (m_sidelen + 2) + p.second]; }
};

puz_game::puz_game(const ptree& attrs, const vector<string>& strs, const ptree& level)
    : m_id(attrs.get<string>("id"))
    , m_sidelen(strs.size() - 2)
    , m_rect_count(m_sidelen * m_sidelen / 2)
    , m_area_info(m_rect_count + m_sidelen + m_sidelen)
    , m_num_poles_rows(m_sidelen)
    , m_num_poles_cols(m_sidelen)
{
    m_start = boost::accumulate(strs, string());
    for(int r = 0, n = 0; r < m_sidelen + 2; ++r){
        auto& str = strs[r];
        for(int c = 0; c < m_sidelen + 2; ++c){
            Position p(r, c);
            switch(char ch = str[c]){
            case PUZ_POSITIVE:
            case PUZ_NEGATIVE:
            case PUZ_EMPTY:
                break;
            case PUZ_HORZ:
            case PUZ_VERT:
                for(int d = 0; d < 2; ++d){
                    Position p = 
                        ch == PUZ_HORZ ? Position(r, c + d)
                        : Position(r + d, c);
                    m_pos2rect[p] = n;
                    for(int i : {n, m_rect_count + p.first, m_rect_count + m_sidelen + p.second})
                        m_area_info[i].push_back(p);
                }
                ++n;
                break;
            default:
                if((c >= m_sidelen) != (r >= m_sidelen))
                    (c >= m_sidelen ? m_num_poles_rows[r][c - m_sidelen] :
                    m_num_poles_cols[c][r - m_sidelen]) =
                    ch == PUZ_SPACE ? PUZ_UNKNOWN : ch - '0';
                break;
            }
        }
    }
}

// first: the index of the area
// second.key : all chars used to fill a position
// second.first : the number of remaining times that the key char can be used in the area
// second.second : the number of times that the key char has been used in the area
struct puz_area : pair<int, map<char, pair<int, int>>>
{
    puz_area() {}
    puz_area(int index, int num_positive, int num_negative, int num_all){
        first = index;
        second[PUZ_POSITIVE] = {num_positive, 0};
        second[PUZ_NEGATIVE] = {num_negative, 0};
        second[PUZ_EMPTY] = {
            num_positive == PUZ_UNKNOWN ||
            num_negative == PUZ_UNKNOWN ? PUZ_UNKNOWN :
            num_all - num_positive - num_negative, 0};
    }
    bool fill_cell(const Position& p, char ch){
        auto& kv = second.at(ch);
        if(kv.first != PUZ_UNKNOWN)
            --kv.first;
        ++kv.second;
        return kv.first >= 0;
    }
    int cant_use(char ch) const { return second.at(ch).first == 0; }
    bool is_ready() const {
        auto f = [](int n){ return n == 0 || n == PUZ_UNKNOWN; };
        auto g = [&](char ch) { return f(second.at(ch).first); };
        return g(PUZ_POSITIVE) && g(PUZ_NEGATIVE);
    }
};

struct puz_group : vector<puz_area>
{
    bool is_ready() const {
        return boost::algorithm::all_of(*this, [](const puz_area& a){
            return a.is_ready();
        });
    }
};

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(const Position& p, char ch);
    bool make_move2(const Position& p, char ch);
    void check_area(const puz_area& a, char ch){
        if(a.cant_use(ch))
            for(auto& p : m_game->m_area_info[a.first])
                remove_pair(p, ch);
    }
    // remove the possibility of filling the position p with char ch
    void remove_pair(const Position& p, char ch){
        auto i = m_pos2chars.find(p);
        if(i != m_pos2chars.end())
            boost::remove_erase(i->second, ch);
    }

    //solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {return boost::count(*this, PUZ_SPACE);}
    unsigned int get_distance(const puz_state& child) const {return 2;}
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    puz_group m_grp_rows;
    puz_group m_grp_cols;
    map<Position, puz_chars> m_pos2chars;
};

puz_state::puz_state(const puz_game& g)
: string(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
, m_game(&g)
{
    for(int r = 0; r < g.m_sidelen; ++r)
        for(int c = 0; c < g.m_sidelen; ++c){
            Position p(r, c);
            if(g.cells(p) == PUZ_EMPTY)
                cells(p) = PUZ_EMPTY;
            else
                m_pos2chars[p] = all_chars;
        }

    for(int i = 0; i < g.m_sidelen; i++){
        auto& np = g.m_num_poles_rows[i];
        m_grp_rows.emplace_back(i + g.m_rect_count, np[0], np[1], g.m_sidelen);
        for(char ch : all_chars)
            check_area(m_grp_rows.back(), ch);
    }
    for(int i = 0; i < g.m_sidelen; i++){
        auto& np = g.m_num_poles_cols[i];
        m_grp_cols.emplace_back(i + g.m_rect_count + g.m_sidelen, np[0], np[1], g.m_sidelen);
        for(char ch : all_chars)
            check_area(m_grp_cols.back(), ch);
    }
}

bool puz_state::make_move(const Position& p, char ch)
{
    if(!make_move2(p, ch))
        return false;

    // The rectangle should be handled within one move
    auto rng = m_game->m_area_info.at(m_game->m_pos2rect.at(p));
    boost::remove_erase(rng, p);
    auto& p2 = rng[0];

    // We either fill the rectangle with a Magnet or mark it as empty
    char ch2 =
        ch == PUZ_EMPTY ? PUZ_EMPTY :
        ch == PUZ_POSITIVE ? PUZ_NEGATIVE :
        PUZ_POSITIVE;
    return make_move2(p2, ch2);
}

bool puz_state::make_move2(const Position& p, char ch)
{
    cells(p) = ch;
    m_pos2chars.erase(p);

    for(auto a : {&m_grp_rows[p.first], &m_grp_cols[p.second]}){
        if(!a->fill_cell(p, ch))
            return false;
        check_area(*a, ch);
    }

    // respect the rule of poles
    if(ch != PUZ_EMPTY)
        for(auto& os : offset){
            auto p2 = p + os;
            if(is_valid(p2))
                remove_pair(p2, ch);
        }

    return boost::algorithm::none_of(m_pos2chars, [](const pair<const Position, puz_chars>& kv){
        return kv.second.empty();
    }) && (!is_goal_state() || m_grp_rows.is_ready() && m_grp_cols.is_ready());
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& kv = *boost::min_element(m_pos2chars, [](
        const pair<const Position, puz_chars>& kv1,
        const pair<const Position, puz_chars>& kv2){
        return kv1.second.size() < kv2.second.size();
    });

    for(char ch : kv.second){
        children.push_back(*this);
        if(!children.back().make_move(kv.first, ch))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for(int r = 0; r < sidelen() + 2; ++r){
        for(int c = 0; c < sidelen() + 2; ++c){
            Position p(r, c);
            if(r < sidelen() && c < sidelen())
                out << cells(p);
            else if(r >= sidelen() && c >= sidelen())
                out << m_game->cells(p);
            else{
                bool is_row = c >= sidelen();
                out <<
                    (is_row ? m_grp_rows : m_grp_cols)
                    [is_row ? r : c].second.at
                    ((is_row ? c : r) == sidelen() ?
                    PUZ_POSITIVE : PUZ_NEGATIVE).second;
            }
            out << ' ';
        }
        out << endl;
    }
    return out;
}

}}

void solve_puz_Magnets()
{
    using namespace puzzles::Magnets;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>
        ("Puzzles\\Magnets.xml", "Puzzles\\Magnets.txt", solution_format::GOAL_STATE_ONLY);
}