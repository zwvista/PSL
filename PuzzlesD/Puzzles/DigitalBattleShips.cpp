#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Logic Games/Puzzle Set 13/Digital Battle Ships

    Summary
    Please divert your course 12+1+2 to avoid collision

    Description
    1. Play like Solo Battle Ships, with a difference.
    2. Each number on the outer board tells you the SUM of the ship or
       ship pieces you're seeing in that row or column.
    3. A ship or ship piece is worth the number it occupies on the board.
    4. Standard rules apply: a ship or piece of ship can't touch another,
       not even diagonally.
    5. In each puzzle there are
       1 Aircraft Carrier (4 squares)
       2 Destroyers (3 squares)
       3 Submarines (2 squares)
       4 Patrol boats (1 square)

    Variant
    5. Some puzzle can also have a:
       1 Supertanker (5 squares)
*/

namespace puzzles::DigitalBattleships{

#define PUZ_SPACE        ' '
#define PUZ_EMPTY        '.'
#define PUZ_TOP            '^'
#define PUZ_BOTTOM        'v'
#define PUZ_LEFT        '<'
#define PUZ_RIGHT        '>'
#define PUZ_MIDDLE        '+'
#define PUZ_BOAT        'o'
#define PUZ_PIECE        'x'

#define PUZ_UNKNOWN        -1

struct puz_ship_info {
    // symbols that represent the ship
    string m_pieces[2];
    // the area occupied by the ship
    string m_area[3];
};

const puz_ship_info ship_info[] = {
    {{"o", "o"}, {"...", ". .", "..."}},
    {{"<>", "^v"}, {"....", ".  .", "...."}},
    {{"<+>", "^+v"}, {".....", ".   .", "....."}},
    {{"<++>", "^++v"}, {"......", ".    .", "......"}},
    {{"<+++>", "^+++v"}, {".......", ".     .", "......."}},
};

// a row or column
struct puz_area_info
{
    int m_sum;
    vector<Position> m_rng;
    // elem: all permutations
    // elem.elem: all ships in this area
    // elem.elem.first: the starting position of a ship
    // elem.elem.second: the length of a ship
    vector<vector<pair<int, int>>> m_perms;
};

struct puz_game    
{
    string m_id;
    int m_sidelen;
    bool m_has_supertanker;
    map<int, int> m_ship2num{{1, 4},{2, 3},{3, 2},{4, 1}};
    map<Position, int> m_pos2num;
    vector<puz_area_info> m_area2info;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs.size() - 1)
    , m_has_supertanker(level.attribute("SuperTanker").as_int() == 1)
    , m_area2info(m_sidelen * 2)
{
    if (m_has_supertanker)
        m_ship2num[5] = 1;

    for (int r = 0; r <= m_sidelen; ++r) {
        auto& str = strs[r];
        for (int c = 0; c <= m_sidelen; ++c) {
            Position p(r, c);
            if (r == m_sidelen && c == m_sidelen)
                break;
            auto s = str.substr(c * 2, 2);
            int n = s == "  " ? PUZ_UNKNOWN : stoi(s);
            if (r == m_sidelen || c == m_sidelen)
                m_area2info[c == m_sidelen ? r : c + m_sidelen].m_sum = n;
            else {
                m_pos2num[p] = n;
                m_area2info[r].m_rng.push_back(p);
                m_area2info[c + m_sidelen].m_rng.push_back(p);
            }
        }
    }

    for (auto& ai : m_area2info) {
        if (ai.m_sum == 0 || ai.m_sum == PUZ_UNKNOWN) continue;
        int sz = ai.m_rng.size();
        int cnt = m_has_supertanker ? 5 : 4;
        // elem.first: partial sum
        // elem.second: all ships in this area
        // elem.second.elem.first: the starting position of the ship
        // elem.second.elem.second: the length of the ship
        deque<pair<int, vector<pair<int, int>>>> queue;
        for (int i = 0; i < sz; ++i)
            for (int j = 1; j <= cnt; ++j)
                if (i + j - 1 < sz) {
                    vector<pair<int, int>> v{{i, j}};
                    queue.emplace_back(0, v);
                }
        while (!queue.empty()) {
            auto& kv = queue.front();
            auto& v = kv.second;
            auto& kv2 = v.back();
            int sum = kv.first;
            for (int i = 0; i < kv2.second; ++i)
                sum += m_pos2num.at(ai.m_rng[kv2.first + i]);
            if (sum == ai.m_sum)
                ai.m_perms.push_back(v);
            else if (sum < ai.m_sum)
                for (int i = kv2.first + kv2.second + 1; i < sz; ++i)
                    for (int j = 1; j <= cnt; ++j)
                        if (i + j - 1 < sz) {
                            v.emplace_back(i, j);
                            queue.emplace_back(sum, v);
                            v.pop_back();
                        }
            queue.pop_front();
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    int sidelen() const { return m_game->m_sidelen; }
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < sidelen() && p.second >= 0 && p.second < sidelen();
    }
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { 
        return tie(m_cells, m_area_matches) <
            tie(x.m_cells, x.m_area_matches);
    }
    bool make_move_ship(const Position& p, int n, bool vert);
    bool make_move_area(int i, int j);
    bool find_matches();
    bool exist_matches(int i, function<bool(int)> f) {
        auto it = m_area_matches.find(i);
        return it == m_area_matches.end() ||
            boost::algorithm::any_of(it->second.second, f);
    }
    bool exist_matches(int i, int j) {
        auto& ai = m_game->m_area2info[i];
        return exist_matches(i, [&](int n) {
            return boost::algorithm::none_of(ai.m_perms[n], [=](const pair<const int, int>& kv) {
                return j >= kv.first && j < kv.first + kv.second;
            });
        });
    }
    bool exist_matches(int i, int j, int k) {
        auto& ai = m_game->m_area2info[i];
        return exist_matches(i, [&](int n) {
            return boost::algorithm::any_of(ai.m_perms[n], [=](const pair<const int, int>& kv) {
                return j == kv.first && k == kv.second;
            });
        });
    }
    void remove_matches(int i, function<bool(int)> f) {
        auto it = m_area_matches.find(i);
        if (it == m_area_matches.end()) return;
        boost::remove_erase_if(it->second.second, f);
    }
    void remove_matches(int i, int j) {
        auto& ai = m_game->m_area2info[i];
        remove_matches(i, [&](int n) {
            return boost::algorithm::any_of(ai.m_perms[n], [=](const pair<const int, int>& kv) {
                return j >= kv.first && j < kv.first + kv.second;
            });
        });
    }
    void remove_matches(int i, int j, int k) {
        auto& ai = m_game->m_area2info[i];
        remove_matches(i, [&](int n) {
            return boost::algorithm::none_of(ai.m_perms[n], [=](const pair<const int, int>& kv) {
                return j == kv.first && k == kv.second;
            });
        });
    }
    void remove_matches2(int i, int j) {
        auto& ai = m_game->m_area2info[i];
        remove_matches(i, [&](int n) {
            return boost::algorithm::none_of(ai.m_perms[n], [=](const pair<const int, int>& kv) {
                return j >= kv.first && j < kv.first + kv.second;
            });
        });
    }

    // solve_puzzle interface
    bool is_goal_state() const {
        return m_ship_matches.empty() && m_area_matches.empty();
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return boost::count_if(m_cells, [](char ch) {
            return ch == PUZ_SPACE || ch == PUZ_PIECE;
        });
    }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game = nullptr;
    map<int, int> m_ship2num;
    map<int, vector<pair<Position, bool>>> m_ship_matches;
    map<int, pair<int, vector<int>>> m_area_matches;
    string m_cells;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_cells(g.m_sidelen * g.m_sidelen, PUZ_SPACE)
    , m_ship2num(g.m_ship2num)
{
    for (int i = 0; i < g.m_area2info.size(); ++i) {
        auto& ai = g.m_area2info[i];
        if (ai.m_sum == PUZ_UNKNOWN) continue;
        auto& kv = m_area_matches[i];
        kv.first = ai.m_sum;
        auto& v = kv.second;
        v.resize(ai.m_perms.size());
        boost::iota(v, 0);
    }

    find_matches();
}

bool puz_state::find_matches()
{
    for (auto it = m_area_matches.begin(); it != m_area_matches.end();)
        // if the sum has become zero
        if (it->second.first == 0) {
            auto& ai = m_game->m_area2info[it->first];
            for (auto& p : ai.m_rng) {
                char& ch = cells(p);
                // there should not exist any more ship pieces
                if (ch == PUZ_SPACE) {
                    ch = PUZ_EMPTY;
                    ++m_distance;
                    bool vert = it->first >= sidelen();
                    // remove all matches in the other direction
                    // that includes this position
                    remove_matches(vert ? p.first : p.second + sidelen(),
                        vert ? p.second : p.first);
                }
            }
            it = m_area_matches.erase(it);
        } else
            ++it;

    m_ship_matches.clear();
    for (const auto& kv : m_ship2num) {
        int i = kv.first;
        m_ship_matches[i];
        for (int j = 0; j < 2; ++j) {
            bool vert = j == 1;
            if (i == 1 && vert)
                continue;

            auto& si = ship_info[i - 1];
            int len = si.m_pieces[j].length();
            for (int r = 0; r < sidelen() - (!vert ? 0 : len - 1); ++r) {
                for (int c = 0; c < sidelen() - (vert ? 0 : len - 1); ++c) {
                    Position p(r, c);
                    if ([&]{
                        if (!exist_matches(!vert ? p.first : p.second + sidelen(),
                            !vert ? p.second : p.first, len))
                            return false;
                        for (int r2 = 0; r2 < 3; ++r2)
                            for (int c2 = 0; c2 < len + 2; ++c2) {
                                auto p2 = p + Position(-1, -1) + Position(!vert ? r2 : c2, !vert ? c2 : r2);
                                if (!is_valid(p2)) continue;
                                char ch = cells(p2);
                                char ch2 = si.m_area[r2][c2];
                                if (ch2 == ' ' && (ch != PUZ_SPACE && ch != PUZ_PIECE ||
                                    !exist_matches(vert ? p2.first : p2.second + sidelen(),
                                    vert ? p2.second : p2.first, 1)) ||
                                    ch2 == '.' && (ch != PUZ_SPACE && ch != PUZ_EMPTY ||
                                    !exist_matches(vert ? p2.first : p2.second + sidelen(),
                                    vert ? p2.second : p2.first)))
                                    return false;
                            }
                        return true;
                    }())
                        m_ship_matches[i].emplace_back(p, vert);
                }
            }
        }
    }
    return boost::algorithm::all_of(m_area_matches,
        [&](const pair<const int, pair<int, vector<int>>>& kv) {
        return !kv.second.second.empty();
    }) && boost::accumulate(m_ship2num, 0, [](int acc, const pair<const int, int>& kv) {
        return acc + kv.first * kv.second;
    }) >= boost::count(m_cells, PUZ_PIECE);
}

bool puz_state::make_move_ship(const Position& p, int n, bool vert)
{
    m_distance = 0;
    auto& si = ship_info[n - 1];
    int len = si.m_pieces[0].length();
    remove_matches(!vert ? p.first : p.second + sidelen(),
        !vert ? p.second : p.first, len);
    for (int r2 = 0; r2 < 3; ++r2)
        for (int c2 = 0; c2 < len + 2; ++c2) {
            auto p2 = p + Position(-1, -1) + Position(!vert ? r2 : c2, !vert ? c2 : r2);
            if (!is_valid(p2)) continue;
            char& ch = cells(p2);
            char ch2 = si.m_area[r2][c2];
            if (ch2 == ' ') {
                ch = si.m_pieces[!vert ? 0 : 1][c2 - 1];
                ++m_distance;
                int n2 = m_game->m_pos2num.at(p2);
                remove_matches(vert ? p2.first : p2.second + sidelen(),
                    vert ? p2.second : p2.first, 1);

                auto f = [&](int id) {
                    auto it = m_area_matches.find(id);
                    if (it != m_area_matches.end())
                        it->second.first -= n2;
                };
                f(p2.first);
                f(p2.second + sidelen());
            } else if (ch == PUZ_SPACE) {
                ch = PUZ_EMPTY;
                ++m_distance;
                remove_matches(vert ? p2.first : p2.second + sidelen(),
                    vert ? p2.second : p2.first);
            }
        }

    if (--m_ship2num[n] == 0)
        m_ship2num.erase(n);
    return find_matches();
}

bool puz_state::make_move_area(int i, int j)
{
    m_distance = 0;
    bool vert = i >= sidelen();
    vector<char> v(sidelen(), PUZ_EMPTY);
    int id = m_area_matches.at(i).second[j];
    auto& ai = m_game->m_area2info[i];
    auto& perm = ai.m_perms[id];
    for (auto& kv : perm)
        for (int k = 0; k < kv.second; ++k)
            v[k + kv.first] = PUZ_PIECE;
    for (int k = 0; k < sidelen(); ++k) {
        auto& p = ai.m_rng[k];
        char& ch = cells(p);
        char ch2 = v[k];
        if (ch2 == PUZ_EMPTY && ch != PUZ_SPACE && ch != PUZ_EMPTY ||
            ch2 == PUZ_PIECE && ch == PUZ_EMPTY)
            return false;
        if (ch != PUZ_SPACE) continue;
        ch = ch2;
        if (ch2 == PUZ_EMPTY) {
            ++m_distance;
            remove_matches(vert ? p.first : p.second + sidelen(),
                vert ? p.second : p.first);
        } else
            remove_matches2(vert ? p.first : p.second + sidelen(),
                vert ? p.second : p.first);
    }
    m_area_matches.erase(i);
    return find_matches();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    const pair<const int, vector<pair<Position, bool>>>* kv_ship_ptr = nullptr;
    if (!m_ship_matches.empty())
        kv_ship_ptr = &*boost::min_element(m_ship_matches, [](
            const pair<const int, vector<pair<Position, bool>>>& kv1,
            const pair<const int, vector<pair<Position, bool>>>& kv2) {
            return kv1.second.size() < kv2.second.size();
        });

    const pair<const int, pair<int, vector<int>>>* kv_area_ptr = nullptr;
    if (!m_area_matches.empty())
        kv_area_ptr = &*boost::min_element(m_area_matches, [](
            const pair<const int, pair<int, vector<int>>>& kv1,
            const pair<const int, pair<int, vector<int>>>& kv2) {
            return kv1.second.second.size() < kv2.second.second.size();
        });

    if (kv_ship_ptr != nullptr && (kv_area_ptr == nullptr ||
        kv_ship_ptr->second.size() < kv_area_ptr->second.second.size()))
        for (auto& kv2 : kv_ship_ptr->second) {
            children.push_back(*this);
            if (!children.back().make_move_ship(kv2.first, kv_ship_ptr->first, kv2.second))
                children.pop_back();
        } else
        for (int i = 0; i < kv_area_ptr->second.second.size(); ++i) {
            children.push_back(*this);
            if (!children.back().make_move_area(kv_area_ptr->first, i))
                children.pop_back();
        }
}

ostream& puz_state::dump(ostream& out) const
{
    auto f = [&](int i) {
        auto& ai = m_game->m_area2info[i];
        int sum = 0;
        for (auto& p : ai.m_rng) {
            char ch = cells(p);
            if (ch != PUZ_EMPTY && ch != PUZ_SPACE)
                sum += m_game->m_pos2num.at(p);
        }
        return sum;
    };
    for (int r = 0; r <= sidelen(); ++r) {
        for (int c = 0; c <= sidelen(); ++c)
            if (r == sidelen() && c == sidelen())
                break;
            else if (c == sidelen() || r == sidelen())
                out << format("{:2}",
                f(c == sidelen() ? r : c + sidelen()))
                << ' ';
            else {
                Position p(r, c);
                char ch = cells(p);
                if (ch == PUZ_EMPTY || ch == PUZ_SPACE)
                    out << PUZ_EMPTY << ' ';
                else
                    out << ch << m_game->m_pos2num.at(p);
                out << ' ';
            }
        out << endl;
    }
    return out;
}

}

void solve_puz_DigitalBattleships()
{
    using namespace puzzles::DigitalBattleships;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/DigitalBattleships.xml", "Puzzles/DigitalBattleships.txt", solution_format::GOAL_STATE_ONLY);
}
