#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: 100 Logic Games 3/Puzzle Set 3/Cultured Branches

    Summary
    Well-read trees

    Description
    1. Each Letter represents a tree. A tree has branches coming out of it,
       in any of the four directions around it.
    2. Each Letter stands for a number and no two Letters stand for the same number.
    3. The number tells you the total length of the Branches coming out of
       that Tree.
    4. In the example all 'A' means '2' and all 'B' means '4'.
    5. Every Tree having the same number must have a different number of Branches
       (1 to 4 in the possible directions around it).
    6. In the example the top Letter 'B' has 3 branches (left, right, down)
       while the bottom one has 2 (up and left).
*/

namespace puzzles::CulturedBranches{

constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_NUMBER = 'N';
constexpr auto PUZ_BRANCH = 'B';
constexpr auto PUZ_WALL = 'W';

constexpr Position offset[] = {
    {-1, 0},        // n
    {0, 1},        // e
    {1, 0},        // s
    {0, -1},        // w
};

constexpr string_view str_branch = "|-|-^>v<";

using puz_perms = vector<vector<int>>;

struct puz_game
{
    string m_id;
    int m_sidelen;
    map<Position, char> m_pos2char;
    map<char, vector<Position>> m_char2rng;
    string m_start;
    int m_max_sum;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
: m_id(level.attribute("id").value())
, m_sidelen(strs.size() + 2)
, m_max_sum((m_sidelen - 3) * 2)
{
    m_start.append(m_sidelen, PUZ_WALL);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        string_view str = strs[r - 1];
        m_start.push_back(PUZ_WALL);
        for (int c = 1; c < m_sidelen - 1; ++c) {
            char ch = str[c - 1];
            if (ch == PUZ_SPACE)
                m_start.push_back(ch);
            else {
                Position p(r, c);
                m_start.push_back(PUZ_NUMBER);
                m_pos2char[p] = ch;
                m_char2rng[ch].push_back(p);
            }
        }
        m_start.push_back(PUZ_WALL);
    }
    m_start.append(m_sidelen, PUZ_WALL);
}

struct puz_state
{
    puz_state(const puz_game& g);
    int sidelen() const {return m_game->m_sidelen;}
    char cells(const Position& p) const { return m_cells[p.first * sidelen() + p.second]; }
    char& cells(const Position& p) { return m_cells[p.first * sidelen() + p.second]; }
    bool operator<(const puz_state& x) const { return m_matches < x.m_matches; }
    bool make_move(char ch, const vector<int>& perm);
    void make_move2(char ch, const vector<int>& perm);
    void make_move3(char ch, const vector<int>& perm, int i, bool stopped);
    int find_matches(bool init);

    //solve_puzzle interface
    bool is_goal_state() const { return get_heuristic() == 0; }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_matches.size(); }
    unsigned int get_distance(const puz_state& child) const { return child.m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    string m_cells;
    // key: the letter
    // value.key: sum of the lengths of the branches that stem from
    //             the letter in all four directions
    // value.value.elem: respective lengths of the branches that stem from
    //             the letter in all four directions
    map<char, puz_perms> m_matches;
    set<int> m_used_sums;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_cells(g.m_start)
{
    for (auto& [ch, rng] : g.m_char2rng)
        m_matches[ch];
    
    find_matches(true);
}

int puz_state::find_matches(bool init)
{
    bool matches_changed = init;
    set<Position> spaces;
    for (auto& [ch, perms] : m_matches) {
        auto perms_old = perms;
        perms.clear();

        for (int sum = 1; sum <= m_game->m_max_sum; ++sum) {
            if (m_used_sums.contains(sum))
                continue;
            vector<vector<int>> perms;
            auto& rng = m_game->m_char2rng.at(ch);

            auto f = [&](const Position& p) {
                vector<set<int>> dir_nums2(4);
                for (int i = 0; i < 4; ++i) {
                    auto& os = offset[i];
                    int n = 0;
                    auto& nums = dir_nums2[i];
                    for (auto p2 = p + os; n <= sum; p2 += os)
                        if (char ch = cells(p2); ch == PUZ_SPACE)
                            // we can stop here
                            nums.insert(n++);
                        else if (ch == str_branch[i] || ch == str_branch[i + 4])
                            // we cannot stop here
                            ++n;
                        else {
                            // we have to stop here
                            nums.insert(n);
                            break;
                        }
                }
                return dir_nums2;
            };

            auto dir_nums = f(rng[0]);
            for (int i = 1; i < rng.size(); ++i) {
                auto dir_nums2 = f(rng[i]);
                for (int j = 0; j < 4; ++j) {
                    set<int> s;
                    boost::set_intersection(dir_nums[j], dir_nums2[j], inserter(s, s.end()));
                    dir_nums[j] = s;
                }
            }

            // Compute the total length of the branches connected to the letter
            // Record the combination if the sum is equal to the given number
            for (int n0 : dir_nums[0])
                for (int n1 : dir_nums[1])
                    for (int n2 : dir_nums[2])
                        for (int n3 : dir_nums[3])
                            if (n0 + n1 + n2 + n3 == sum) {
                                vector v = {n0, n1, n2, n3, sum};
                                perms.push_back(v);
                                for (auto& p : rng)
                                    for (int i = 0; i < 4; ++i) {
                                        auto& os = offset[i];
                                        auto p2 = p;
                                        for (int j = 1; j <= v[i]; ++j)
                                            if (cells(p2 += os) == PUZ_SPACE)
                                                spaces.insert(p2);
                                    }
                            }
        }

        if (!init)
            switch(perms.size()) {
            case 0:
                return 0;
            case 1:
                return make_move2(ch, perms.front()), 1;
            default:
                matches_changed = matches_changed || perms != perms_old;
                break;
            }
    }
    // pruning
    // All the branches added up should cover all the remaining spaces
    if (boost::count(m_cells, PUZ_SPACE) != spaces.size())
        return 0;

    if (!matches_changed)
        return 2;

    for (auto& [ch, perms] : m_matches)
        for (int i = 0; i < 4; ++i) {
            auto f = [=](const vector<int>& v1, const vector<int>& v2) {
                return v1[i] < v2[i];
            };
            const auto& perm = *boost::min_element(perms, f);
            int n = boost::max_element(perms, f)->at(i);
            make_move3(ch, perm, i, perm[i] == n);
        }
    return 1;
}

void puz_state::make_move3(char ch, const vector<int>& perm, int i, bool stopped)
{
    for (auto& p : m_game->m_char2rng.at(ch)) {
        auto& os = offset[i];
        int n = perm[i];
        auto p2 = p + os;
        for (int j = 0; j < n - 1; ++j) {
            cells(p2) = str_branch[i];
            p2 += os;
        }
        if (stopped && n > 0)
            // branch head
            cells(p2) = str_branch[i + 4];
    }
}

void puz_state::make_move2(char ch, const vector<int>& perm)
{
    for (int i = 0; i < 4; ++i)
        make_move3(ch, perm, i, true);

    ++m_distance;
    m_matches.erase(ch);
}

bool puz_state::make_move(char ch, const vector<int>& perm)
{
    m_distance = 0;
    make_move2(ch, perm);
    int m;
    while ((m = find_matches(false)) == 1);
    return m == 2;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    auto& [ch, perms] = *boost::min_element(m_matches, [](
        const pair<const char, vector<vector<int>>>& kv1,
        const pair<const char, vector<vector<int>>>& kv2) {
        return kv1.second.size() < kv2.second.size();
    });

    for (auto& perm : perms) {
        children.push_back(*this);
        if (!children.back().make_move(ch, perm))
            children.pop_back();
    }
}

ostream& puz_state::dump(ostream& out) const
{
    for (int r = 1; r < sidelen() - 1; ++r) {
        for (int c = 1; c < sidelen() - 1; ++c) {
            Position p(r, c);
            if (char ch = cells(p); ch == PUZ_NUMBER)
                out << format("{:2}", m_game->m_pos2char.at(p));
            else
                out << ' ' << ch;
        }
        println(out);
    }
    return out;
}

}

void solve_puz_CulturedBranches()
{
    using namespace puzzles::CulturedBranches;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/CulturedBranches.xml", "Puzzles/CulturedBranches.txt", solution_format::GOAL_STATE_ONLY);
}
