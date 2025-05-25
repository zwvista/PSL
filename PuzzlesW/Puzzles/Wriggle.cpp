#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::Wriggle{

constexpr auto PUZ_BOX = '#';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_GOAL = '.';

constexpr Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    Position m_size;
    Position m_goal;
    vector<vector<Position>> m_worms;
    string m_cells;
    string m_id;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() + 2, strs[0].length() / 2 + 2))
{

    m_cells = string(rows() * cols(), PUZ_SPACE);
    fill(m_cells.begin(), m_cells.begin() + cols(), PUZ_BOX);
    fill(m_cells.rbegin(), m_cells.rbegin() + cols(), PUZ_BOX);

    int n = cols();
    for (int r = 1; r < rows() - 1; ++r, n += cols()) {
        m_cells[n] = m_cells[n + cols() - 1] = PUZ_BOX;
        const string& vstr = strs.at(r - 1);
        for (int c = 1; c < cols() - 1; ++c) {
            Position p(r, c);
            switch(char ch = vstr[c * 2 - 2]) {
            case PUZ_GOAL:
                m_goal = p;
            case PUZ_BOX:
                m_cells[n + c] = ch; break;
            case PUZ_SPACE:
                break;
            default:
                {
                    size_t nw = ch - '1';
                    size_t sz = m_worms.size();
                    if (nw >= sz) {
                        m_worms.resize(nw + 1);
                        for (size_t i = sz; i <= nw; i++)
                            m_worms[i].resize(26);
                    }
                    ch = vstr[c * 2 - 1];
                    m_worms[nw][ch - 'a'] = p;
                }
            }
        }
    }

    for (vector<Position>& w : m_worms) {
        int n = boost::range::find(w, Position()) - w.begin();
        w.resize(n);
    }
}

struct puz_step
{
    puz_step(int i, int j, int k, const Position& p)
        :worm_index(i), is_head(j == 0), offset_index(k), p(p) {}
    bool operator<(const puz_step& x) const {
        return p < x.p;
    }
    bool same_move(const puz_step& x) const {
        return worm_index == x.worm_index && is_head == x.is_head;
    }
    int worm_index;
    bool is_head;
    Position p;
    int offset_index;
};

ostream & operator<<(ostream &out, const puz_step &mi)
{
    static string_view dirs = "LRUD";
    string head_tail = mi.is_head ? "head" : "tail";
    out << format("move: {} {} {} {}\n", (mi.worm_index + 1), head_tail, mi.p, dirs[mi.offset_index]);
    return out;
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g)
        : m_game(&g), m_worms(g.m_worms) {calc_layout();}
    bool operator<(const puz_state& x) const {
        return m_layout < x.m_layout || m_layout == x.m_layout && m_move < x.m_move;
    }
    int rows() const {return m_game->rows();}
    int cols() const {return m_game->cols();}
    char cells(const Position& p) const {return m_game->cells(p);}
    const Position& goal() const {return m_game->m_goal;}

    // solve_puzzle interface
    bool is_goal_state() const {
        const vector<Position>& w = m_worms[0];
        return w.front() == goal() || w.back() == goal();
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const;
    unsigned int get_distance(const puz_state& child) const {
        unsigned int d = 1;
        if (m_move && !m_move->same_move(*child.m_move))
            d += 1 << 16;
        return d;
    }
    void dump_move(ostream& out) const {if(m_move) out << *m_move;}
    ostream& dump(ostream& out, bool move_only) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out, true);
    }

    bool can_move(int i, int j, int k) const;
    void make_move(int i, int j, int k);
    void calc_layout();
    const puz_game* m_game = nullptr;
    vector<vector<Position>> m_worms;
    boost::optional<puz_step> m_move;
    string m_layout;
};

bool puz_state::can_move(int i, int j, int k) const
{
    using namespace boost::algorithm;
    const vector<Position>& w = m_worms[i];
    Position p = (j == 0 ? w.front() : w.back()) + offset[k];
    return cells(p) != PUZ_BOX && (i == 0 || cells(p) != PUZ_GOAL) &&
        none_of(m_worms, [&](const vector<Position>& w) {
        return any_of_equal(w, p);
    });
}

void puz_state::make_move(int i, int j, int k)
{
    vector<Position>& w = m_worms[i];
    Position p1 = j == 0 ? w.front() : w.back();
    Position p2 = p1 + offset[k];
    m_move = puz_step(i, j, k, p1);
    if (j == 0) {
        w.back() = p2;
        boost::rotate(w, std::prev(w.end()));
    } else {
        w.front() = p2;
        boost::rotate(w, std::next(w.begin()));
    }
    calc_layout();
}

void puz_state::calc_layout()
{
    m_layout = string((rows() - 2) * (cols() - 2) * 2, PUZ_SPACE);
    auto f = [&](const Position& p, char ch_sz, char ch_ord) {
        int n = (p.first - 1) * (cols() - 2) + p.second - 1;
        m_layout[n * 2] = ch_sz;
        m_layout[n * 2 + 1] = ch_ord;
    };
    for (int i = 0; i < m_worms.size(); ++i) {
        const auto& w = m_worms[i];
        char ch_sz = 'a' + (i == 0 ? 0 : w.size());
        char ch_ord = 'a';
        if (w.front() < w.back())
            for (const auto& p : w)
                f(p, ch_sz, ch_ord++);
        else
            BOOST_REVERSE_FOREACH(const auto& p, w)
                f(p, ch_sz, ch_ord++);
    }
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < m_worms.size(); i++)
        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 4; k++)
                if (can_move(i, j, k)) {
                    children.push_back(*this);
                    children.back().make_move(i, j, k);
                }
}

unsigned int puz_state::get_heuristic() const
{
    auto& w = m_worms[0];
    unsigned int d = m_move && m_move->worm_index == 0 ? 0 : 1 << 16;
    return d + std::min(manhattan_distance(w.front(), goal()),
        manhattan_distance(w.back(), goal()));
}

ostream& puz_state::dump(ostream& out, bool move_only) const
{
    dump_move(out);

    if (!move_only) {
        vector<string> vstrs(rows() - 2);
        for (int r = 1; r < rows() - 1; ++r) {
            string& str = vstrs[r - 1];
            str = string((cols() - 2) * 2, PUZ_SPACE);
            for (int c = 1; c < cols() - 1; ++c)
                str[c * 2 - 2] = str[c * 2 - 1] = cells({r, c});
        }
        for (size_t i = 0; i < m_worms.size(); i++) {
            const vector<Position>& w = m_worms[i];
            for (size_t j = 0; j < w.size(); j++) {
                int r, c;
                boost::tie(r, c) = w[j];
                string& str = vstrs[r - 1];
                str[c * 2 - 2] = i + '1';
                str[c * 2 - 1] = j + 'a';
            }
        }
        for (const string& str : vstrs)
            out << str << endl;
    }

    return out;
}

void dump_all(ostream& out, const list<puz_state>& spath)
{
    for (auto it = spath.cbegin(); it != spath.cend(); it++) {
        auto it2 = next(it);
        bool move_only =
            it != spath.cbegin() && it2 != spath.cend() &&
            it->m_move->same_move(*it2->m_move);
        it->dump(out, move_only);
    }
}

}

void solve_puz_Wriggle()
{
    using namespace puzzles::Wriggle;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state> >(
        "Puzzles/Wriggle.xml", "Puzzles/Wriggle.txt", solution_format::CUSTOM_STATES, dump_all);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state> >(
        "Puzzles/Wriggle2.xml", "Puzzles/Wriggle2.txt", solution_format::CUSTOM_STATES, dump_all);
}
