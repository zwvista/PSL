#include "stdafx.h"
#include "astar_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"
#include "HiddenPath.h"

namespace puzzles::HiddenPath{

struct puz_generator
{
    int m_sidelen;
    vector<int> m_nums, m_dirs;
    vector<int> m_cells;
    bool is_valid(const Position& p) const {
        return p.first >= 0 && p.first < m_sidelen && p.second >= 0 && p.second < m_sidelen;
    }
    int index(const Position& p) const { return p.first * m_sidelen + p.second; }
    int dirs(const Position& p) const { return m_dirs[index(p)]; }
    int cells(const Position& p) const { return m_cells[index(p)]; }
    int& cells(const Position& p) { return m_cells[index(p)]; }

    puz_generator(int n);
    void gen_bridge();
    string to_string();
};

puz_generator::puz_generator(int n)
    : m_sidelen(n + 2)
{
//     m_cells.append(m_sidelen, PUZ_BOUNDARY);
//     for (int r = 1; r < m_sidelen - 1; ++r) {
//         m_cells.push_back(PUZ_BOUNDARY);
//         m_cells.append(m_sidelen - 2, PUZ_SPACE);
//         m_cells.push_back(PUZ_BOUNDARY);
//     }
//     m_cells.append(m_sidelen, PUZ_BOUNDARY);
//     // add the first island
//     int i = rand() % (n * n);
//     Position p(i / n + 1, i % n + 1);
//     add_island(p);
}

void puz_generator::gen_bridge()
{
//     // randomly select an island
//     auto p = m_islands[rand() % m_islands.size()];
//     auto& nums = m_pos2nums.at(p);
// 
//     for (int i = 0; i < 4; ++i) {
//         bool is_horz = i % 2 == 1;
//         auto& os = offset[i];
//         int& num = nums[i];
//         if (num != -1) break;
//         vector<Position> rng;
//         {
//             auto p2 = p + os;
//             for (; cells(p2) == PUZ_SPACE; p2 += os)
//                 // Islands should not be adjacent to each other.
//                 if (boost::algorithm::none_of(offset, [&](const Position& os2) {
//                     return cells(p2 + os2) == PUZ_ISLAND;
//                 }))
//                     rng.push_back(p2);
//             if (cells(p2) == PUZ_ISLAND)
//                 rng.push_back(p2);
//         }
//         // randomly select the number of bridges to add
//         num = rng.empty() ? 0 : (rand() % 5 + 1) / 2;
//         if (num == 0) continue;
//         char ch = is_horz ? num == 1 ? PUZ_HORZ_1 : PUZ_HORZ_2 :
//             num == 1 ? PUZ_VERT_1 : PUZ_VERT_2;
//         auto p3 = rng[rand() % rng.size()];
//         // add the other island if not exists
//         if (cells(p3) == PUZ_SPACE)
//             add_island(p3);
//         // set the number of bridges for the other island
//         m_pos2nums.at(p3)[(i + 2) % 4] = num;
//         for (auto p2 = p + os; p2 != p3; p2 += os)
//             cells(p2) = ch;
//     }
//     // remove the island since no more bridges can be added
//     boost::remove_erase(m_islands, p);
}

string puz_generator::to_string()
{
    //auto f = [&](const Position& p) {
    //    auto& nums = m_pos2nums.at(p);
    //    return boost::accumulate(nums, 0, [](int acc, int n) {
    //        return acc + (n > 0 ? n : 0);
    //    });
    //};

    stringstream ss;
    //ss << endl;
     //for (int r = 1; r < m_sidelen - 1; r++) {
     //    for (int c = 1; c < m_sidelen - 1; c++) {
     //        Position p(r, c);
     //        ss << (cells(p) != PUZ_ISLAND ? ' ' : char(f(p) + '0'));
     //    }s
     //    ss << '`' << endl;
     //}
    return ss.str();
}

}

bool is_valid_HiddenPath(const string& s)
{
    xml_document doc;
    auto levels = doc.append_child("puzzle").append_child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = "test";
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("../Test.xml");
    solve_puz_HiddenPathTest();
    ifstream in("../Test.txt");
    string x;
    getline(in, x);
    getline(in, x);
    return x != "Solution 1:";
}

void save_new_HiddenPath(const string& id, const string& s)
{
    xml_document doc;
    doc.load_file("Puzzles/HiddenPath/HiddenPathGen.xml");
    auto levels = doc.child("puzzle").child("levels");
    auto level = levels.append_child("level");
    level.append_attribute("id") = id.c_str();
    level.append_child(node_cdata).set_value(s.c_str());
    doc.save_file("Puzzles/HiddenPath/HiddenPathGen.xml");
}

void gen_puz_HiddenPath()
{
    using namespace puzzles::HiddenPath;

    for (int i = 6; i <= 8; ++i)
        for (int j = 0; j < 8; ++j) {
            string s;
            do {
                for (;;) {
                    puz_generator g(i);
                    //do
                    //    g.gen_bridge();
                    //while (!g.m_islands.empty());
                    //int k = i * i / 3 + (rand() % 5 - 2);
                    //if (g.m_pos2nums.size() >= k) {
                    //    s = g.to_string();
                    //    print("{}", s);
                    //    break;
                    //}
                }
            } while(!is_valid_HiddenPath(s));
            stringstream ss;
            print(ss, "{}-{}", i, j + 1);
            save_new_HiddenPath(ss.str(), s);
        }
}
