// https://omiplay.com/ja/games/3d-logic/

#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "dfs_solver.h"
#include "bfs_move_gen.h"
#include "solve_puzzle.h"

namespace puzzles::_3dlogic{

#define PUZ_NOENTRY        '#'
#define PUZ_SPACE        ' '
#define PUZ_FOUND        '!'

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

typedef pair<int, Position> Position3d;
typedef pair<Position3d, Position3d> Position3dPair;

struct puz_game
{
    string m_id;
    int m_sidelen;
    string m_start;
    vector<Position3dPair> m_markers;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_sidelen(strs[0].length())
{
    m_start = boost::accumulate(strs, string());
    for (size_t i = 0; i < m_start.length(); ++i) {
        char ch = m_start[i];
        if (islower(ch)) {
            Position3d* pv;
            int n = ch - 'a';
            if (m_markers.size() == n) {
                m_markers.resize(n + 1);
                pv = &m_markers[n].first;
            } else
                pv = &m_markers[n].second;
            *pv = Position3d(i / m_sidelen / m_sidelen, Position(i / m_sidelen % m_sidelen, i % m_sidelen));
        }
    }
}

typedef pair<char, Position3d> puz_step;

ostream& operator<<(ostream& out, const puz_step& a)
{
    out << format("{}({}{}{})", a.first, a.second.first, a.second.second.first, a.second.second.second);
    return out;
}

struct puz_state_base
{
    int sidelen() const {return m_game->m_sidelen;}
    int marker_count() const {return (int)m_game->m_markers.size();}
    int p2i(const Position3d& p) const {return (p.first * sidelen() + p.second.first) * sidelen() + p.second.second;}
    bool is_valid(Position3d& p) const;
    unsigned int manhattan_distance3d(const Position3d& p31, const Position3d& p32) const;

    const puz_game* m_game = nullptr;
};

bool puz_state_base::is_valid(Position3d& p) const
{
    int face, r, c;
    // cannot compile with gcc
    // boost::tie(face, boost::tie(r, c)) = p;
    face = p.first, r = p.second.first, c = p.second.second;
    if (face == 0) {
        if (r == -1 || c == -1) return false;
        if (r == sidelen())
            face = 1, r = 0;
        else if (c == sidelen())
            face = 2, c = sidelen() - 1 - r, r = 0;
    } else if (face == 1) {
        if (r == sidelen() || c == -1) return false;
        if (r == -1)
            face = 0, r = sidelen() - 1;
        else if (c == sidelen())
            face = 2, c = 0;
    } else if (face == 2) {
        if (r == sidelen() || c == sidelen()) return false;
        if (r == -1)
            face = 0, r = sidelen() - 1 -  c, c = sidelen() - 1;
        else if (c == -1)
            face = 1, c = sidelen() - 1;
    }
    p = Position3d(face, Position(r, c));
    return true;
}

unsigned int puz_state_base::manhattan_distance3d(const Position3d& p31, const Position3d& p32) const
{
    if (p31 == p32) return 0;
    int face1 = p31.first, face2 = p32.first;
    Position p1 = p31.second, p2 = p32.second;
    if (face1 > face2) {
        std::swap(face1, face2);
        std::swap(p1, p2);
    }
    if (face1 == 0 && face2 == 1)
        p2.first += sidelen();
    else if (face1 == 0 && face2 == 2)
        p2 = Position(sidelen() - 1 - p2.second, sidelen() + p2.first);
    else if (face1 == 1 && face2 == 2)
        p2.second += sidelen();
    return manhattan_distance(p1, p2) - 1;
}

struct puz_state : puz_state_base
{
    puz_state() {}
    puz_state(const puz_game& g) : m_cells(g.m_start), m_links(g.m_markers)
    {
        m_game = &g;
    }
    char cells(const Position3d& p) const {return m_cells.at(p2i(p));}
    char& cells(const Position3d& p) {return m_cells[p2i(p)];}
    bool operator<(const puz_state& x) const {return m_cells < x.m_cells;}
    bool operator==(const puz_state& x) const { return m_cells == x.m_cells; }
    void make_move(int i, bool is_link1, const Position3d& p);

    // solve_puzzle interface
    bool is_goal_state() const {
        for (const Position3dPair& pr : m_links)
            if (pr.first != pr.second)
                return false;
        return true;
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    string m_cells;
    vector<Position3dPair> m_links;
    boost::optional<puz_step> m_move;
    int m_index = 0;
};

struct puz_state2 : puz_state_base
{
    puz_state2(const puz_state& x, vector<int>& connects, const Position3d& p)
        : m_cells(&x.m_cells), m_links(&x.m_links), m_connects(&connects) {
        m_game = x.m_game; make_move(p);
    }
    char cells(const Position3d& p) const {return m_cells->at(p2i(p));}
    bool operator<(const puz_state2& x) const {return m_curpos < x.m_curpos;}
    void make_move(const Position3d& p) {m_curpos = p;}

    void gen_children(list<puz_state2>& children) const;

    const string* m_cells;
    const vector<Position3dPair>* m_links;
    vector<int>* m_connects;
    Position3d m_curpos;
};

void puz_state2::gen_children(list<puz_state2>& children) const
{
    for (const Position& os : offset) {
        Position3d p(m_curpos.first, m_curpos.second + os);
        if (!is_valid(p)) continue;
        char ch = cells(p);
        if (ch == PUZ_SPACE) {
            children.push_back(*this);
            children.back().make_move(p);
        } else if (islower(ch)) {
            int i = ch - 'a';
            const Position3d& link1 = (*m_links)[i].first;
            const Position3d& link2 = (*m_links)[i].second;
            if (link1 != link2) {
                if (p == link1) (*m_connects)[i] |= 1;
                if (p == link2) (*m_connects)[i] |= 2;
            }
        }
    }
}

void puz_state::make_move(int i, bool is_link1, const Position3d& p)
{
    char ch = i + 'a';
    Position3dPair& pr = m_links[i];
    Position3d& link1 = is_link1 ? pr.first : pr.second;
    const Position3d& link2 = is_link1 ? pr.second : pr.first;
    int cnt = marker_count();
    m_index = ((is_link1 ? 0 : cnt) + i + 1) % (cnt * 2);
    cells(link1 = p) = ch;
    m_move = puz_step(ch, p);

    for (const Position& os : offset) {
        Position3d p2(p.first, p.second + os);
        if (is_valid(p2) && p2 == link2) {
            link1 = link2;
            break;
        }
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    vector<int> connects_indexes(sidelen() * sidelen() * 3, -1);
    vector<vector<int> > connects_all;

    for (;;) {
        bool found = false;
        for (int face = 0; face < 3; ++face)
            for (int r = 0; r < sidelen(); ++r)
                for (int c = 0; c < sidelen(); ++c) {
                    Position3d p(face, Position(r, c));
                    if (connects_indexes[p2i(p)] == -1 && cells(p) == PUZ_SPACE) {
                        found = true;
                        list<puz_state2> smoves;
                        vector<int> connects(marker_count());
                        puz_move_generator<puz_state2>::gen_moves(
                            puz_state2(*this, connects, p), smoves);
                        connects_all.push_back(connects);
                        int n = (int)connects_all.size() - 1;
                        for (const puz_state2& s : smoves) {
                            const Position3d& p2 = s.m_curpos;
                            connects_indexes[p2i(p2)] = n;
                        }
                        goto for_break;
                    }
                }
for_break:
        if (!found) break;
    }


    for (int index = 0, cnt = marker_count(); index < cnt * 2; ++index) {
        int i = index % cnt;
        bool is_link1 = index / cnt == 0;
        const Position3dPair& pr = m_links[i];
        const Position3d& link1 = is_link1 ? pr.first : pr.second;
        const Position3d& link2 = is_link1 ? pr.second : pr.first;
        if (link1 == link2) continue;
        bool found = false;
        for (const Position& os : offset) {
            Position3d p(link1.first, link1.second + os);
            if (is_valid(p) && cells(p) == PUZ_SPACE &&
                connects_all[connects_indexes[p2i(p)]][i] == 3) {
                found = true;
                break;
            }
        }
        if (!found) return;
    }

    for (int index = m_index, cnt = marker_count(); ; index = (index + 1) % (cnt * 2)) {
        int i = index % cnt;
        bool is_link1 = index / cnt == 0;
        const Position3dPair& pr = m_links[i];
        const Position3d& link1 = is_link1 ? pr.first : pr.second;
        const Position3d& link2 = is_link1 ? pr.second : pr.first;
        if (link1 == link2) continue;
        for (const Position& os : offset) {
            Position3d p(link1.first, link1.second + os);
            if (is_valid(p) && cells(p) == PUZ_SPACE &&
                connects_all[connects_indexes[p2i(p)]][i] == 3) {
                children.push_back(*this);
                children.back().make_move(i, is_link1, p);
            }
        }
        break;
    }

    //for(int i = 0, cnt = marker_count(); i < cnt; ++i) {
    //    const Position3dPair& pr = m_links[i];
    //    const Position3d& link1 = pr.first;
    //    const Position3d& link2 = pr.second;
    //    if (link1 == link2) continue;
    //    for (const Position& os : offset) {
    //        Position3d p(link1.first, link1.second + os);
    //        if (is_valid(p) && cells(p) == PUZ_SPACE &&
    //            connects_all[connects_indexes[p2i(p)]][i] == 3) {
    //                children.push_back(*this);
    //                children.back().make_move(i, true, p);
    //        }
    //    }
    //    break;
    //}
}

unsigned int puz_state::get_heuristic() const
{
    unsigned int md = 0;
    for (const Position3dPair& pr : m_links)
        md += manhattan_distance3d(pr.first, pr.second);
    return md;
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << *m_move << endl;
    println(out);
    return out;
}

}

void solve_puz_3dlogic()
{
    using namespace puzzles::_3dlogic;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>(
        "Puzzles/3dlogic.xml", "Puzzles/3dlogic.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/3dlogic.xml", "Puzzles/3dlogic_ida.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_dfs<puz_state, false>>(
        "Puzzles/3dlogic.xml", "Puzzles/3dlogic_dfs.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
