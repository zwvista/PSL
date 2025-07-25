#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

/*
    https://www.quickflashgames.com/games/twistoff/
    1. A BLOB, MOVES AROUND WHEN THE SCREEN IS TURNED. GET IT TO THE FINISH!
    2. A ROCK, DOESN'T MOVE AND HAS NO OTHER EFFECT THAN JUST SITTING THERE.
    3. A BREAKABLE ROCK GETS DESTROYED AFTER ONE HIT.
    4. A BLUB (SAME AS THE BLOB) BUT DOESN'T HAVE TO GET TO THE FINISH.
    5. THE FINISH, GET ALL THE BLOBS TO FINISH THE LEVEL.
    6. TELEPORT BLOCK, PLACE TWO OR MORE OF THESE ON THE SCREEN.
       IF YOU PLACE MORE THAN 2 ON THE SCREEN THEY WILL EACH TELEPORT TO THE NEXT ONE.
       (EXAMPLE 3: ONE TELEPORTS TO 2, 2 TO 3, AND 3 TO 1)
    7. BOMB BLOCK, EXPLODES AFTER A SET TIME (CAN BE SET BY THE PERSON WHO MAKES THE LEVEL)
    8. DEATH BLOCK, RESETS THE GAME WHEN THE PLAYER BLOCK HITS IT.
    9. LOCKS WITH THEIR KEYS.
       IF THE BLUE KEY IS HIT, THE BLUE LOCK BECOMES PASSABLE.
       IF AFTER THAT THE YELLOW KEY IS HIT, THE BLUE BLOCK AND THE KEY BECOME VISIBLE AGAIN.
       AND THE YELLOW ONE BECOMES INVISIBLE.
    10.DOORS, ONLY STOPS THE BLOBS OR THE BLUBS.
       THE PINK ONE PASSES THE BLOBS AND THE GREEN ONE THE BLUBS.
*/

namespace puzzles::turnz{

constexpr auto PUZ_ROCK = '#';
constexpr auto PUZ_FINISH = '.';
constexpr auto PUZ_BLOB = '$';
constexpr auto PUZ_BLUB = '!';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_KEY_Y = 'y';
constexpr auto PUZ_LOCK_Y = 'Y';
constexpr auto PUZ_BREAK2 = '2';
constexpr auto PUZ_BREAK1 = '1';
constexpr auto PUZ_DEATH = 'x';

constexpr Position offset[] = {
    {1, 0},
    {0, 1},
    {-1, 0},
    {0, -1},
};

struct puz_game
{
    string m_id;
    Position m_size;
    string m_cells;
    set<Position> m_finishes;
    set<Position> m_blobs;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(strs.size() + 2, strs[0].length() + 2)
{
    m_cells = string(rows() * cols(), PUZ_SPACE);
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_ROCK);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_ROCK);

    for (int r = 1, n = cols(); r < rows() - 1; ++r) {
        string_view str = strs[r - 1];
        m_cells[n++] = PUZ_ROCK;
        for (int c = 1; c < cols() - 1; ++c) {
            char ch = str[c - 1];
            Position p(r, c);
            switch(ch) {
            case PUZ_BLOB:
                m_blobs.insert(p);
                break;
            case PUZ_FINISH:
                m_finishes.insert(p);
                ch = PUZ_SPACE;
                break;
            }
            m_cells[n++] = ch;
        }
        m_cells[n++] = PUZ_ROCK;
    }
}

struct puz_state
{
    puz_state(const puz_game& g)
        : m_cells(g.m_cells), m_game(&g), m_blobs(g.m_blobs)
        , m_grav(0), m_move(0) {}
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
    char& cells(const Position& p) {return m_cells[p.first * cols() + p.second];}
    bool operator<(const puz_state& x) const {
        return m_cells < x.m_cells || m_cells == x.m_cells && m_grav < x.m_grav;
    }
    bool operator==(const puz_state& x) const {
        return m_cells == x.m_cells && m_grav == x.m_grav;
    }
    bool make_move(int i);

    //solve_puzzle interface
    bool is_goal_state() const {return m_blobs.empty();}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(m_move) out << m_move;}
    ostream& dump(ostream& out) const;

    const puz_game* m_game;
    set<Position> m_blobs;
    string m_cells;
    int m_grav;
    char m_move;
};

bool puz_state::make_move(int i)
{
    static string_view dirs = ")(";
    m_move = dirs[i];
    m_grav = (m_grav + (i == 0 ? 1 : 3)) % 4;
    Position os = offset[m_grav];
    int rs = m_grav % 2 == 0 ? rows() : cols();
    int cs = m_grav % 2 == 0 ? cols() : rows();
    for (bool keep_moving = true; keep_moving;) {
        keep_moving = false;
        for (int r = rs - 2; r >= 1; --r) {
            for (int c = 1; c < cs - 1; ++c) {
                Position p =
                    m_grav == 1 ? Position(cs - 1 - c, r) :
                    m_grav == 2 ? Position(rs - 1 - r, cs - 1 - c) :
                    m_grav == 3 ? Position(c, rs - 1 - r) :
                    Position(r, c);
                char ch = cells(p);
                if (ch != PUZ_BLOB && ch != PUZ_BLUB)
                    continue;
                Position p2 = p;
                char ch2;
                for (;;) {
                    if (ch == PUZ_BLOB && m_game->m_finishes.contains(p2)) {
                        ch2 = PUZ_FINISH;
                        break;
                    }
                    ch2 = cells(p2 + os);
                    if (ch2 == PUZ_SPACE)
                        p2 += os;
                    else if (ch2 == PUZ_KEY_Y) {
                        boost::replace(m_cells, PUZ_LOCK_Y, PUZ_SPACE);
                        cells(p2 += os) = PUZ_SPACE;
                        keep_moving = true;
                    } else if (ch2 == PUZ_BREAK2) {
                        cells(p2 + os) = PUZ_BREAK1;
                        break;
                    } else if (ch2 == PUZ_BREAK1)
                        cells(p2 += os) = PUZ_SPACE;
                    else if (ch2 == PUZ_DEATH)
                        return false;
                    else
                        break;
                }
                if (p2 != p) {
                    cells(p) = PUZ_SPACE;
                    if (ch == PUZ_BLOB)
                        m_blobs.erase(p);
                    if (ch2 != PUZ_FINISH) {
                        cells(p2) = ch;
                        if (ch == PUZ_BLOB)
                            m_blobs.insert(p2);
                    }
                }
            }
        }
    }
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 2; ++i)
        if (children.push_back(*this); !children.back().make_move(i))
            children.pop_back();
}

unsigned int puz_state::get_heuristic() const
{
    if (m_blobs.empty()) return 0;
    bool ar1 = false, ar2 = false, ac1 = false, ac2 = false;
    for (const Position& p : m_blobs) {
        auto& [ir, ic] = p;
        bool r1 = true, r2 = true, c1 = true, c2 = true;
        for (const Position& p2 : m_game->m_finishes) {
            auto& [jr, jc] = p2;
            if (ir < jr)
                r2 = false;
            else if (ir > jr)
                r1 = false;
            else
                r1 = r2 = false;
            if (ic < jc)
                c2 = false;
            else if (ic > jc)
                c1 = false;
            else
                c1 = c2 = false;
        }
        ar1 = ar1 || r1;
        ar2 = ar2 || r2;
        ac1 = ac1 || c1;
        ac2 = ac2 || c2;
    }
    int rn = ar1 && ar2 ? 3 : ar1 || ar2 ? 2 : 1;
    int cn = ac1 && ac2 ? 3 : ac1 || ac2 ? 2 : 1;
    return std::min(rn, cn);
}

ostream& puz_state::dump(ostream& out) const
{
    if (m_move)
        out << "move: " << m_move << endl;
    for (int r = 1; r < rows() - 1; ++r) {
        for (int c = 1; c < cols() - 1; ++c)
            out << cells({r, c}) << ' ';
        println(out);
    }
    return out;
}

}

void solve_puz_turnz()
{
    using namespace puzzles::turnz;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state, false>>
        ("Puzzles/turnz.xml", "Puzzles/turnz.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
