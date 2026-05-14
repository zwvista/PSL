#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 2/Puzzle Set 6/Inbetween Sumscrapers

    Summary
    Sumscrapers on the ground

    Description
    1. Find two Skyscrapers and fill the remaining cells with numbers.
    2. Each row and column contains two skyscrapers.
    3. The remaining cells contain numbers increasing from 1 to N-2 (N being
       the board size).
    4. Numbers appear once in every row and column.
    5. Hints on the border give you the sums of the numbers between the skyscrapers.
*/

namespace puzzles::InbetweenSumscrapers2{

constexpr auto PUZ_SKYSCRAPER = 0;
constexpr auto PUZ_SKYSCRAPER_S = " X";
constexpr auto PUZ_SPACE_S = "  ";

using puz_numbers = set<int>;

struct puz_move
{
    // index of the skyscrapers, 0 to N - 1
    int m_index1, m_index2;
    puz_numbers m_inbetween, m_outside;
};

struct puz_game
{
    string m_id;
    int m_sidelen;
    int m_max_num;
    puz_numbers m_numbers;
    vector<vector<Position>> m_area_pos;
    map<int, int> m_area2num;
    map<int, vector<puz_move>> m_num2moves;
    
    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() - 1)
, m_max_num(m_sidelen - 2)
{
    for (int i = 0; i < m_sidelen * 2; ++i)
        if (auto s = i < m_sidelen ? strs[i].substr(m_sidelen * 2, 2) : strs[m_sidelen].substr((i - m_sidelen) * 2, 2); s != "  ")
            m_area2num[i] = stoi(string(s));

    for (int i = 0; i <= m_max_num; ++i)
        m_numbers.insert(i);

    auto numbers = m_numbers;
    numbers.erase(PUZ_SKYSCRAPER);
    for (auto& [_1, num] : m_area2num) {
        auto& moves = m_num2moves[num];
        if (!moves.empty()) continue;
        auto f = [&](const puz_numbers& inbetween, const puz_numbers& outside) {
            if (boost::accumulate(inbetween, 0) != num)
                return;
            int sz = inbetween.size();
            for (int i = 0; i < m_sidelen - sz - 1; ++i)
                moves.emplace_back(i, i + sz + 1, inbetween, outside);
        };
        if (num == 0) {
            puz_numbers inbetween, outside = numbers;
            f(inbetween, outside);
        } else {
            for (int m = 1; m <= m_max_num; ++m) {
                vector<int> selector(m_max_num, 0);
                fill(selector.begin(), selector.begin() + m, 1);
                do {
                    puz_numbers inbetween, outside = numbers;
                    for (int i = 1; i <= m_max_num; i++)
                        if (selector[i - 1])
                            inbetween.insert(i), outside.erase(i);
                    f(inbetween, outside);
                } while (prev_permutation(selector.begin(), selector.end()));
            }
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    puz_numbers cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    puz_numbers& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_cells < x.m_cells; }
    bool make_move_hint(int area_id, const puz_move& move);
    bool check_numbers();

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return sidelen() * sidelen() - boost::count_if(m_cells, [&](const puz_numbers& numbers) {
            return numbers.size() == 1;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    vector<puz_numbers> m_cells;
    map<int, int> m_area2num;
    set<Position> m_finished;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, g.m_numbers)
, m_area2num(g.m_area2num)
{
}

bool puz_state::make_move_hint(int area_id, const puz_move& move)
{
    auto& [index1, index2, inbetween, outside] = move;
    bool is_row = area_id < sidelen();
    int i = is_row ? area_id : area_id - sidelen();
    for (int j = 0; j < sidelen(); ++j) {
        auto& numbers = cells({is_row ? i : j, is_row ? j : i});
        auto& numbers2 = j < index1 || j > index2 ? outside :
            j > index1 || j < index2 ? inbetween : set{PUZ_SKYSCRAPER};
        puz_numbers numbers3;
        boost::set_intersection(numbers, numbers2, inserter(numbers3, numbers3.end()));
        if ((numbers = numbers3).empty())
            return false;
    }
    return true;
}

bool puz_state::check_numbers()
{
    for (;;) {
        set<Position> newly_finished;
        for (int r = 0; r < sidelen(); ++r)
            for (int c = 0; c < sidelen(); ++c)
                if (Position p(r, c); !m_finished.contains(p))
                    if (auto& numbers = cells(p); numbers.size() == 1)
                        newly_finished.insert(p);
        if (newly_finished.empty())
            return true;
        for (auto& p : newly_finished) {
            
        }
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    if (!m_area2num.empty()) {
        auto& [area_id, num] = *boost::min_element(m_area2num, [&](
            const pair<const int, int>& kv1,
            const pair<const int, int>& kv2) {
            auto f = [&](const pair<const int, int>& kv) {
                return m_game->m_num2moves.at(kv.second).size();
            };
            return f(kv1) < f(kv2);
        });
        auto& moves = m_game->m_num2moves.at(num);
        for (auto& move : moves)
            if (!children.emplace_back(*this).make_move_hint(area_id, move))
                children.pop_back();
    }
//    auto& [area_id, perm_ids] = *boost::min_element(m_matches, [](
//        const pair<const int, vector<int>>& kv1, 
//        const pair<const int, vector<int>>& kv2) {
//        return kv1.second.size() < kv2.second.size();
//    });
//    for (int n : perm_ids)
//        if (!children.emplace_back(*this).make_move(area_id, n))
//            children.pop_back();
}

ostream& puz_state::dump(ostream& out) const
{
//    for (int r = 0; r < sidelen() + 1; ++r) {
//        for (int c = 0; c < sidelen() + 1; ++c)
//            if (int n = cells({r, c}); n == PUZ_SPACE)
//                out << PUZ_SPACE_S;
//            else if (n == PUZ_SKYSCRAPER)
//                out << PUZ_SKYSCRAPER_S;
//            else
//                out << format("{:2}", n);
//        println(out);
//    }
    return out;
}

}

void solve_puz_InbetweenSumscrapers2()
{
    using namespace puzzles::InbetweenSumscrapers2;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/InbetweenSumscrapers.xml", "Puzzles/InbetweenSumscrapers2.txt", solution_format::GOAL_STATE_ONLY);
}
