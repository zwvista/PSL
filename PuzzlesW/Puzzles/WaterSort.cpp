#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Water pouring puzzle

    Summary
    Water pouring puzzle
      
    Description
*/

namespace puzzles::WaterSort{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_EMPTY = '.';

struct puz_tube : vector<string>
{
    int length() const {
        return boost::accumulate(*this, 0, [](int sum, const string& s) {
            return sum + s.length();
        });
    }
    string to_string() const {
        return boost::accumulate(*this, string());
    }
};

struct puz_game
{
    string m_id;
    Position m_size;
    vector<puz_tube> m_tubes;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const { return m_size.first; }
    int cols() const { return m_size.second; }
};

struct puz_move : pair<int, int> { using pair::pair; };

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size(), strs[0].length()))
    , m_tubes(cols())
{
    for (int r = 0; r < rows(); ++r) {
        string_view str = strs[r];
        for (int c = 0; c < cols(); ++c)
            if (char ch = str[c]; ch == PUZ_SPACE)
                ;
            else if (m_tubes[c].empty() || m_tubes[c].back().back() != ch)
                m_tubes[c].push_back(string(1, ch));
            else
                m_tubes[c].back().push_back(ch);
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int rows() const { return m_game->rows(); }
    int cols() const { return m_game->cols(); }
    bool operator<(const puz_state& x) const { return m_tubes < x.m_tubes; }
    void make_move(int i, int j);
    bool is_finished(const puz_tube& tube) const { return tube.size() == 1 && tube[0].size() == rows(); }

    //solve_puzzle interface
    bool is_goal_state() const {
        return boost::algorithm::all_of(m_tubes, [&](const puz_tube& tube) {
            return tube.empty() || is_finished(tube);
        });
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return is_goal_state() ? 0 : 1; }
    unsigned int get_distance(const puz_state& child) const { return 1; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_tube> m_tubes;
    optional<puz_move> m_move;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_tubes(g.m_tubes)
{
}

void puz_state::make_move(int i, int j)
{
    auto &tube1 = m_tubes[i], &tube2 = m_tubes[j];
    if (auto it = tube1.begin(); tube2.empty()) {
        tube2.push_back(*it);
        tube1.erase(it);
    } else if (int n = rows() - tube1.length(); it->size() <= n) {
        tube2.front().append(*it);
        tube1.erase(it);
    } else {
        tube2.front().append(it->substr(0, n));
        it->erase(0, n);
    }
    m_move = puz_move(i, j);
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < m_tubes.size(); i++) {
        auto& tube1 = m_tubes[i];
        if (tube1.empty() || is_finished(tube1)) continue; // cannot pour from an empty tube or a finished tube
        for (int j = 0; j < m_tubes.size(); j++) {
            auto& tube2 = m_tubes[j];
            if (i == j || tube2.length() == rows()) continue;
            if (!tube2.empty() && tube1.front().back() != tube2.front().back()) continue; // cannot pour if colors do not match
            children.push_back(*this);
            children.back().make_move(i, j);
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move) {
        auto& [i, j] = *m_move;
        out << format("move: {} -> {}\n", to_string(i + 1), to_string(j + 1));
    }
    vector<string> strs;
    for (int c = 0; c < cols(); c++) {
        auto str = m_tubes[c].to_string();
        strs.push_back(string(rows() - str.size(), PUZ_EMPTY) + str);
    }
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            out << strs[c][r] << PUZ_SPACE;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_WaterSort()
{
    using namespace puzzles::WaterSort;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/WaterSort.xml", "Puzzles/WaterSort.txt");
}
