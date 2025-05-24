#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 5/Clouds

    Summary
    Weather Radar Report

    Description
    1. You must find Clouds in the sky.
    2. The hints on the borders tell you how many tiles are covered by Clouds
       in that row or column.
    3. Clouds only appear in rectangular or square areas. Furthermore, their
       width and height is always at least two tiles wide.
    4. Clouds can't touch between themselves, not even diagonally. 
*/

namespace puzzles::Clouds{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_CLOUD        'C'
#define PUZ_BOUNDARY    'B'

const Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

struct puz_cloud
{
    set<Position> m_body;
    set<Position> m_empty;
    Position m_size;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    vector<puz_cloud> m_clouds;
    set<Position> m_pieces;
    string m_start;

    puz_game(const vector<string>& strs, const xml_node& level);
    char cells(const Position& p) const { return m_start[p.first * m_sidelen + p.second]; }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() + 1)
    , m_piece_counts_rows(m_sidelen)
    , m_piece_counts_cols(m_sidelen)
    , m_start(m_sidelen * m_sidelen, PUZ_SPACE)
{
    for (int i = 0; i < m_sidelen; ++i)
        cells({i, 0}) = cells({i, m_sidelen - 1}) =
        cells({0, i}) = cells({m_sidelen - 1, i}) = PUZ_BOUNDARY;

    for (int r = 1; r < m_sidelen; ++r) {
        auto& str = strs[r - 1];
        for (int c = 1; c < m_sidelen; c++) {
            char ch = str[c - 1];
            if (ch == PUZ_CLOUD)
                m_pieces.emplace(r, c);
            else if (ch != PUZ_SPACE)
                (c == m_sidelen - 1 ? m_piece_counts_rows[r] : m_piece_counts_cols[c])
                = isdigit(ch) ? ch - '0' : ch - 'A' + 10;
        }
    }

    int max_rows = *boost::max_element(m_piece_counts_cols);
    int max_cols = *boost::max_element(m_piece_counts_rows);
    for (int h = 2; h <= max_rows; ++h)
        for (int r = 1; r < m_sidelen - h; ++r)
            for (int w = 2; w <= max_cols; ++w)
                for (int c = 1; c < m_sidelen - w; ++c) {
                    puz_cloud o;
                    auto f = [&](const Position& p) {
                        char ch = cells(p);
                        if (ch == PUZ_SPACE)
                            o.m_empty.insert(p);
                    };
                    for (int dr = 0; dr < h; ++dr)
                        if (m_piece_counts_rows[r + dr] < w)
                            goto next_cloud;
                    for (int dc = 0; dc < w; ++dc)
                        if (m_piece_counts_cols[c + dc] < h)
                            goto next_cloud;
                    o.m_size = {h, w};
                    for (int dr = 0; dr < h; ++dr)
                        for (int dc = 0; dc < w; ++dc)
                            o.m_body.emplace(r + dr, c + dc);
                    for (int dr = -1; dr <= h; ++dr)
                        f({r + dr, c - 1}), f({r + dr, c + w});
                    for (int dc = -1; dc <= w; ++dc)
                        f({r - 1, c + dc}), f({r + h, c + dc});
                    m_clouds.push_back(o);

                next_cloud:;
                }
}

struct puz_state : string
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    char cells(const Position& p) const { return (*this)[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return (*this)[p.first * sidelen() + p.second]; }
    bool make_move(int n);
    void check_area();
    bool find_matches();

    // solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
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

    const puz_game* m_game = nullptr;
    vector<int> m_piece_counts_rows, m_piece_counts_cols;
    set<Position> m_pieces;
    vector<int> m_matches, m_matches2;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : string(g.m_start), m_game(&g)
    , m_piece_counts_rows(g.m_piece_counts_rows)
    , m_piece_counts_cols(g.m_piece_counts_cols)
    , m_pieces(g.m_pieces)
{
    m_matches.resize(g.m_clouds.size());
    boost::iota(m_matches, 0);

    check_area();
    find_matches();
}

void puz_state::check_area()
{
    auto f = [](char& ch) {
        if (ch == PUZ_SPACE)
            ch = PUZ_EMPTY;
    };

    for (int i = 1; i < sidelen() - 1; ++i) {
        if (m_piece_counts_rows[i] == 0)
            for (int j = 1; j < sidelen() - 1; ++j)
                f(cells({i, j}));
        if (m_piece_counts_cols[i] == 0)
            for (int j = 1; j < sidelen() - 1; ++j)
                f(cells({j, i}));
    }
}

bool puz_state::find_matches()
{
    boost::remove_erase_if(m_matches, [&](int id) {
        auto& o = m_game->m_clouds[id];
        auto& p0 = *o.m_body.begin();
        for (int dc = 0; dc < o.m_size.second; ++dc)
            if (m_piece_counts_cols[p0.second + dc] < o.m_size.first)
                return true;
        for (int dr = 0; dr < o.m_size.first; ++dr)
            if (m_piece_counts_rows[p0.first + dr] < o.m_size.second)
                return true;
        return boost::algorithm::any_of(o.m_body, [&](const Position& p) {
            return cells(p) == PUZ_EMPTY;
        }) || boost::algorithm::any_of(o.m_empty, [&](const Position& p) {
            return cells(p) == PUZ_CLOUD;
        });
    });

    m_matches2.clear();
    if (!m_pieces.empty())
        for (int id : m_matches) {
            auto& o = m_game->m_clouds[id];
            if (boost::algorithm::any_of(m_pieces, [&](const Position& p) {
                return o.m_body.contains(p);
            }))
                m_matches2.push_back(id);
        } else {
        // pruning
        set<Position> rng;
        map<int, set<int>> rc_indexes;
        for (int id : m_matches) {
            auto& o = m_game->m_clouds[id];
            for (auto& p : o.m_body) {
                rng.insert(p);
                // if it contains the position (r,c)
                // the i-th cloud will be inserted into
                // Row r group and Column c group
                rc_indexes[p.first].insert(id);
                rc_indexes[sidelen() + p.second].insert(id);
            }
        }

        // the total number of the tiles in the clouds
        // in a row or column should be greater or equal to
        // the count in that row or column
        for (int i = 1; i < sidelen() - 1; ++i)
            if (boost::count_if(rng, [i](const Position& p) {
                return p.second == i;
            }) < m_piece_counts_cols[i] ||
                boost::count_if(rng, [i](const Position& p) {
                return p.first == i;
            }) < m_piece_counts_rows[i])
                return false;

        if (!m_matches.empty()) {
            // find the group that has the fewest number of clouds
            auto& kv = *boost::min_element(rc_indexes, [](
                const pair<const int, set<int>>& kv1,
                const pair<const int, set<int>>& kv2) {
                return kv1.second.size() < kv2.second.size();
            });
            for (int id : kv.second)
                m_matches2.push_back(id);
        }
    }
    return true;
}

bool puz_state::make_move(int n)
{
    auto& o = m_game->m_clouds[n];
    for (auto& p : o.m_body) {
        cells(p) = PUZ_CLOUD;
        --m_piece_counts_rows[p.first];
        --m_piece_counts_cols[p.second];
        m_pieces.erase(p);
    }
    for (auto& p : o.m_empty)
        cells(p) = PUZ_EMPTY;

    m_distance = o.m_body.size();
    boost::remove_erase(m_matches, n);

    check_area();
    return find_matches();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int n : m_matches2) {
        children.push_back(*this);
        if (!children.back().make_move(n))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen(); ++r) {
        for (int c = 1; c < sidelen(); ++c)
            if (r == sidelen() - 1 && c == sidelen() - 1)
                break;
            else if (c == sidelen() - 1)
                out << boost::format("%-2d") % m_game->m_piece_counts_rows[r];
            else if (r == sidelen() - 1)
                out << boost::format("%-2d") % m_game->m_piece_counts_cols[c];
            else
                out << cells({r, c}) << ' ';
        out << endl;
    }
    return out;
}

}

void solve_puz_Clouds()
{
    using namespace puzzles::Clouds;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Clouds.xml", "Puzzles/Clouds.txt", solution_format::GOAL_STATE_ONLY);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/Clouds2.xml", "Puzzles/Clouds2.txt", solution_format::GOAL_STATE_ONLY);
}
