#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
#include "Bridges.h"

namespace puzzles{ namespace Bridges{

struct puz_generator
{
    int m_sidelen;
    map<Position, vector<int>> m_pos2nums;
    vector<Position> m_islands;
    string m_start;
    char cells(const Position& p) const { return m_start.at(p.first * m_sidelen + p.second); }
    char& cells(const Position& p) { return m_start[p.first * m_sidelen + p.second]; }

    puz_generator(int n);
    void add_island(const Position& p) {
        cells(p) = PUZ_ISLAND;
        m_islands.push_back(p);
        m_pos2nums[p] = {-1, -1, -1, -1};
    }
    void gen_bridge();
    string to_string();
};

puz_generator::puz_generator(int n)
    : m_sidelen(n + 2)
{
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    for (int r = 1; r < m_sidelen - 1; ++r) {
        m_start.push_back(PUZ_BOUNDARY);
        m_start.append(m_sidelen - 2, PUZ_SPACE);
        m_start.push_back(PUZ_BOUNDARY);
    }
    m_start.append(m_sidelen, PUZ_BOUNDARY);
    int i = rand() % (n * n);
    Position p(i / n + 1, i % n + 1);
    add_island(p);
}

void puz_generator::gen_bridge()
{
    auto p = m_islands[rand() % m_islands.size()];
    auto& nums = m_pos2nums.at(p);

    for (int i = 0; i < 4; ++i) {
        bool is_horz = i % 2 == 1;
        auto& os = offset[i];
        int& num = nums[i];
        if (num != -1) break;
        vector<Position> rng;
        for (auto p2 = p + os; cells(p2) == PUZ_SPACE; p2 += os)
            // Islands should not be adjacent to each other.
            if (boost::algorithm::none_of(offset, [&](const Position& os2) {
                return cells(p2 + os2) == PUZ_ISLAND;
            }))
                rng.push_back(p2);
        num = rng.empty() ? 0 : (rand() % 5 + 1) / 2;
        if (num == 0) continue;
        char ch = is_horz ? num == 1 ? PUZ_HORZ_1 : PUZ_HORZ_2 :
            num == 1 ? PUZ_VERT_1 : PUZ_VERT_2;
        auto p3 = rng[rand() % rng.size()];
        add_island(p3);
        m_pos2nums.at(p3)[(i + 2) % 4] = num;
        for (auto p2 = p + os; p2 != p3; p2 += os)
            cells(p2) = ch;
    }
    boost::remove_erase(m_islands, p);
}

string puz_generator::to_string()
{
    auto f = [&](const Position& p) {
        auto& nums = m_pos2nums.at(p);
        return boost::accumulate(nums, 0, [](int acc, int n) {
            return acc + (n > 0 ? n : 0);
        });
    };

    stringstream ss;
    ss << endl;
    for (int r = 1; r < m_sidelen - 1; r++) {
        for (int c = 1; c < m_sidelen - 1; c++) {
            Position p(r, c);
            ss << (cells(p) != PUZ_ISLAND ? ' ' : char(f(p) + '0'));
        }
        ss << '`' << endl;
    }
    return ss.str();
}

}}

bool is_valid_Bridges(const string& s)
{
    xml_document doc;
    auto levels = doc.append_child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = "test";
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("Puzzles/BridgesTest.xml");
    solve_puz_BridgesTest();
    ifstream in("Puzzles/BridgesTest.txt");
    string x;
    getline(in, x);
    getline(in, x);
    return x != "Solution 1:";
}

void gen_puz_Bridges()
{
    using namespace puzzles::Bridges;

    string s;
    do {
        for (;;) {
            puz_generator g(6);
            do
                g.gen_bridge();
            while (!g.m_islands.empty());
            if (g.m_pos2nums.size() >= 12) {
                s = g.to_string();
                cout << s;
                break;
            }
        }
    } while(!is_valid_Bridges(s));
}
