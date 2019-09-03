#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

namespace puzzles::bloxorz{

#define PUZ_HOLE        '#'
#define PUZ_SPACE        ' '
#define PUZ_GOAL        '.'
#define PUZ_ONE            '1'
#define PUZ_TWO            '2'
#define PUZ_ORANGE        '!'

enum ESwitchActionType {stSwap, stOn, stOff};

typedef pair<bool,        // heavy or not
    vector<pair<int,    // bridge index
    ESwitchActionType> > > puz_switch;
typedef pair<vector<Position>,
    bool                // on or off
    > puz_bridge;
typedef vector<Position> puz_splitter;

const Position offset[] = {
    {0, -1},
    {0, 1},
    {-1, 0},
    {1, 0},
};

struct puz_game
{
    string m_id;
    Position m_size;
    array<Position, 2> m_blocks;
    Position m_goal;
    string m_cells;
    map<Position, puz_switch> m_switches;
    map<Position, puz_splitter> m_splitters;
    map<Position, Position> m_teleporters;
    vector<puz_bridge> m_bridges;

    puz_game(const vector<string>& strs, const xml_node& level);
    int rows() const {return m_size.first;}
    int cols() const {return m_size.second;}
    char cells(const Position& p) const {return m_cells[p.first * cols() + p.second];}
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_size(Position(strs.size() + 4, strs[0].length() + 4))
{
    m_cells = string(rows() * cols(), PUZ_SPACE);
    fill(m_cells.begin(), m_cells.begin() + 2 * cols(), PUZ_HOLE);
    fill(m_cells.rbegin(), m_cells.rbegin() + 2 * cols(), PUZ_HOLE);

    for (int r = 2, n = 2 * cols(); r < rows() - 2; ++r) {
        const string& str = strs[r - 2];
        m_cells[n++] = PUZ_HOLE;
        m_cells[n++] = PUZ_HOLE;
        for (int c = 2; c < cols() - 2; ++c) {
            char ch = str[c - 2];
            switch(ch) {
            case PUZ_TWO:
                m_blocks[0] = m_blocks[1] = Position(r, c);
                ch = PUZ_SPACE;
                break;
            case PUZ_GOAL:
                m_goal = Position(r, c);
                ch = PUZ_SPACE;
                break;
            }
            m_cells[n++] = ch;
        }
        m_cells[n++] = PUZ_HOLE;
        m_cells[n++] = PUZ_HOLE;
    }

    Position os(2, 2);
    for (auto v : level.children()) {
        if (string(v.name()) == "switch") {
            Position p;
            puz_switch switch_;
            parse_position(v.attribute("position").value(), p);
            switch_.first = string(v.attribute("type").value()) == "heavy";
            {
                const string& action = v.attribute("action").value();
                pair<int, ESwitchActionType> pr;
                string::const_iterator first = action.begin();
                qi::phrase_parse(first, action.end(), 
                    +(
                        qi::int_[phx::ref(pr.first) = qi::_1] >>
                        (
                            qi::lit("swap")[phx::ref(pr.second) = stSwap] |
                            qi::lit("on")[phx::ref(pr.second) = stOn] |
                            qi::lit("off")[phx::ref(pr.second) = stOff]
                        )[phx::push_back(phx::ref(switch_.second), phx::ref(pr))]
                    ), ascii::space
                );
            }
            m_switches[p + os] = switch_;
        } else if (string(v.name()) == "bridge") {
            puz_bridge bridge;
            parse_positions(v.attribute("position").value(), bridge.first);
            bridge.second = string(v.attribute("state").value()) == "on";
            for (Position& p2 : bridge.first)
                p2 += os;
            m_bridges.push_back(bridge);
        } else if (string(v.name()) == "splitter") {
            Position p;
            parse_position(v.attribute("position").value(), p);
            puz_splitter splitter;
            parse_positions(v.attribute("locations").value(), splitter);
            for (Position& p2 : splitter)
                p2 += os;
            m_splitters[p + os] = splitter;
        } else if (string(v.name()) == "teleporter") {
            Position p, p2;
            parse_position(v.attribute("position").value(), p);
            parse_position(v.attribute("location").value(), p2);
            m_teleporters[p + os] = p2 + os;
        }
    }
}

struct puz_state
{
    puz_state() {}
    puz_state(const puz_game& g);
    const Position& goal() const {return m_game->m_goal;}
    bool operator<(const puz_state& x) const {
        return m_blocks < x.m_blocks ||
            m_blocks == x.m_blocks && m_bridges < x.m_bridges;
    }
    bool operator==(const puz_state& x) const {
        return m_blocks == x.m_blocks && m_bridges == x.m_bridges;
    }
    bool is_hole(const Position& p) const {return m_game->cells(p) == PUZ_HOLE && m_bridges.count(p) == 0;}
    bool is_orange(const Position& p) const {return m_game->cells(p) == PUZ_ORANGE;}
    bool make_move(int n, int dir);
    bool check_switch(const Position& p, bool heavy_included = false);
    bool check_splitter(const Position& p);
    bool check_teleporter(const Position& p);

    // solve_puzzle interface
    bool is_goal_state() const {return m_blocks[0] == goal() && m_blocks[1] == goal();}
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const {
        return min(manhattan_distance(m_blocks[0], goal()),
            manhattan_distance(m_blocks[1], goal()));
    }
    unsigned int get_distance(const puz_state& child) const {return 1;}
    void dump_move(ostream& out) const {if(!m_move.empty()) out << m_move;}
    ostream& dump(ostream& out) const;
    friend ostream& operator<<(ostream& out, const puz_state& state) {
        return state.dump(out);
    }

    const puz_game* m_game;
    array<Position, 2> m_blocks;
    bool m_split;
    vector<bool> m_bridge_states;
    set<Position> m_bridges;
    string m_move;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_blocks(g.m_blocks), m_split(false)
{
    for (const puz_bridge& bridge : m_game->m_bridges) {
        m_bridge_states.push_back(bridge.second);
        if (bridge.second)
            m_bridges.insert(bridge.first.begin(), bridge.first.end());
    }
}

bool puz_state::make_move(int n, int dir)
{
    static char* moves = "lrud";
    const Position& os = offset[dir];
    bool triggered = false;

    m_move.clear();
    if (m_split) {
        Position& block = m_blocks[n];
        block += os;
        if (is_hole(block))
            return false;
        triggered = check_switch(block);
        if (is_hole(m_blocks[1 - n]))
            return false;
        for (int j = 0; j < 4; ++j)
            if (m_blocks[0] + offset[j] == m_blocks[1]) {
                m_split = false;
                break;
            }
        m_move += n + '0';
    } else {
        if (m_blocks[0] == m_blocks[1]) {
            m_blocks[0] += os;
            m_blocks[1] = m_blocks[0] + os;
        } else {
            m_blocks[0] += os;
            m_blocks[1] += os;
            if (m_blocks[0] + os == m_blocks[1])
                m_blocks[0] = m_blocks[1];
            else if (m_blocks[0] - os == m_blocks[1])
                m_blocks[1] = m_blocks[0];
        }
        if (is_hole(m_blocks[0]) || is_hole(m_blocks[1]))
            return false;
        if (m_blocks[0] == m_blocks[1]) {
            if (is_orange(m_blocks[0]))
                return false;
            triggered = check_teleporter(m_blocks[0]);
            triggered = check_switch(m_blocks[0], true) || triggered;
            triggered = check_splitter(m_blocks[0]) || triggered;
            if (triggered && (is_hole(m_blocks[0]) || is_hole(m_blocks[1])))
                return false;
        } else {
            triggered = check_switch(m_blocks[0]);
            triggered = check_switch(m_blocks[1]) || triggered;
        }
    }

    boost::sort(m_blocks);
    m_move += moves[dir];
    if (triggered)
        m_move += "..";
    return true;
}

bool puz_state::check_switch(const Position& p, bool heavy_included)
{
    map<Position, puz_switch>::const_iterator i = m_game->m_switches.find(p);
    if (i == m_game->m_switches.end()) return false;

    const puz_switch& switch_ = i->second;
    if (!heavy_included && switch_.first) return false;

    typedef pair<int, ESwitchActionType> pair_type;
    for (const pair_type& pr : switch_.second) {
        int index = pr.first;
        ESwitchActionType type = pr.second;
        bool on = 
            type == stOn ? true :
            type == stOff ? false :
            !m_bridge_states[index];
        if (on == m_bridge_states[index]) continue;

        const puz_bridge& bridge = m_game->m_bridges[index];
        if (m_bridge_states[index] = on)
            m_bridges.insert(bridge.first.begin(), bridge.first.end());
        else
            for (const Position& p : bridge.first)
                m_bridges.erase(p);
    }
    return true;
}

bool puz_state::check_splitter(const Position& p)
{
    map<Position, puz_splitter>::const_iterator i = m_game->m_splitters.find(p);
    if (i == m_game->m_splitters.end()) return false;
    m_split = true;
    m_blocks[0] = i->second[0];
    m_blocks[1] = i->second[1];
    return true;
}

bool puz_state::check_teleporter(const Position& p)
{
    map<Position, Position>::const_iterator i = m_game->m_teleporters.find(p);
    if (i == m_game->m_teleporters.end()) return false;
    m_blocks[0] = m_blocks[1] = i->second;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i) {
        int movable = m_split ? 2 : 1;
        for (int n = 0; n < movable; ++n) {
            children.push_back(*this);
            if (!children.back().make_move(n, i))
                children.pop_back();
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    if (!m_move.empty())
        out << "move: " << m_move << endl;
    for (int r = 2; r < m_game->rows() - 2; ++r) {
        for (int c = 2; c < m_game->cols() - 2; ++c) {
            Position p(r, c);
            out << (p == m_blocks[0] && p == m_blocks[1] ? PUZ_TWO :
                p == m_blocks[0] || p == m_blocks[1] ? PUZ_ONE :
                p == goal() ? PUZ_GOAL : m_game->cells(p));
        }
        out << endl;
    }
    return out;
}

}

void solve_puz_bloxorz()
{
    using namespace puzzles::bloxorz;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/bloxorz.xml", "Puzzles/bloxorz_a.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_idastar<puz_state>>(
        "Puzzles/bloxorz.xml", "Puzzles/bloxorz_ida.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/bloc.xml", "Puzzles/bloc.txt", solution_format::MOVES_ONLY_SINGLE_LINE);
}
