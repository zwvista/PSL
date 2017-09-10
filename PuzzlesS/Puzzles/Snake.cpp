#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 16/Snake

    Summary
    Still lives inside your pocket-sized computer

    Description
    1. Complete the Snake, head to tail, inside the board.
    2. The two tiles given at the start are the head and the tail of the snake
       (it is irrelevant which is which).
    3. Numbers on the border tell you how many tiles the snake occupies in that
       row or column.
    4. The snake can't touch itself, not even diagonally.
*/

namespace puzzles{ namespace Snake{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_SNAKE        'S'

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
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    // 1st dimension : the index of the area(rows and columns)
    // 2nd dimension : all the positions that the area is composed of
    vector<vector<Position>> m_area2range;
    map<int, vector<string>> m_num2perms;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_piece_counts_rows(m_sidelen)
    , m_piece_counts_cols(m_sidelen)
    , m_area2range(m_sidelen * 2)
    , m_start(m_sidelen * m_sidelen, PUZ_SPACE)
{
    for(int r = 0; r <= m_sidelen; ++r){
        auto& str = strs[r];
        for(int c = 0; c <= m_sidelen; c++){
            char ch = str[c];
            if(ch != PUZ_SPACE)
                (c == m_sidelen ? m_piece_counts_rows[r] : m_piece_counts_cols[c])
                = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
        }
    }
    cells({0, 0}) = cells({m_sidelen - 1, m_sidelen - 1}) = PUZ_SNAKE;
    for(int r = 0; r < m_sidelen; ++r)
        for(int c = 0; c < m_sidelen; ++c){
            Position p(r, c);
            m_area2range[r].push_back(p);
            m_area2range[m_sidelen + c].push_back(p);
        }

    for(int i = 1; i <= m_sidelen; ++i){
        auto& perms = m_num2perms[i];
        auto perm = string(m_sidelen - i, PUZ_EMPTY) + string(i, PUZ_SNAKE);
        do{
            perms.push_back(perm);
        } while(boost::next_permutation(perm));
    }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int i, int j);
    bool make_move2(int i, int j);
    int find_matches(bool init);

    // solve_puzzle interface
    bool is_goal_state() const {return get_heuristic() == 0;}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return 
        boost::accumulate(m_piece_counts_rows, 0);
    }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    map<int, vector<int>> m_matches;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
    , m_piece_counts_rows(g.m_piece_counts_rows)
    , m_piece_counts_cols(g.m_piece_counts_cols)
{
    auto f = [&](int n){
        vector<int> perm_ids(g.m_num2perms.at(n).size());
        boost::iota(perm_ids, 0);
        return perm_ids;
    };

    for(int i = 0; i < sidelen(); ++i){
        m_matches[i] = f(g.m_piece_counts_rows[i]);
        m_matches[sidelen() + i] = f(g.m_piece_counts_cols[i]);
    }
    
    --m_piece_counts_rows[0], --m_piece_counts_cols[0];
    --m_piece_counts_rows[sidelen() - 1], --m_piece_counts_cols[sidelen() - 1];

    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    for(auto& kv : m_matches){
        int area_id = kv.first;
        auto& perm_ids = kv.second;

        string chars;
        for(auto& p : m_game->m_area2range[kv.first])
            chars.push_back(cells(p));

        int n = area_id < sidelen() ? m_game->m_piece_counts_rows[area_id] :
            m_game->m_piece_counts_cols[area_id - sidelen()];
        auto& perms = m_game->m_num2perms.at(n);

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
                return make_move2(area_id, perm_ids.front()), 1;
            }
    }
    return 2;
}

bool puz_state::make_move2(int i, int j)
{
    int n = i < sidelen() ? m_game->m_piece_counts_rows[i] :
        m_game->m_piece_counts_cols[i - sidelen()];
    auto& range = m_game->m_area2range[i];
    auto& perm = m_game->m_num2perms.at(n)[j];

    for(int k = 0; k < perm.size(); ++k){
        auto& p = range[k];
        char& ch = cells(p);
        if(ch == PUZ_SPACE && (ch = perm[k]) == PUZ_SNAKE)
            ++m_distance, --m_piece_counts_rows[p.first], --m_piece_counts_cols[p.second];
    }
    m_matches.erase(i);
    return true;
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
    for(int r = 0; r <= sidelen(); ++r){
        for(int c = 0; c <= sidelen(); ++c)
            if(r == sidelen() && c == sidelen())
                break;
            else if(c == sidelen())
                out << format("%-2d") % m_game->m_piece_counts_rows[r];
            else if(r == sidelen())
                out << format("%-2d") % m_game->m_piece_counts_cols[c];
            else
                out << cells({r, c}) << ' ';
        out << endl;
    }
    return out;
}

}}

void solve_puz_Snake()
{
    using namespace puzzles::Snake;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Snake.xml", "Puzzles/Snake.txt", solution_format::GOAL_STATE_ONLY);
}
