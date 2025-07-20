#include "stdafx.h"
#include "astar_solver.h"
#include "idastar_solver.h"
#include "solve_puzzle.h"

/*
    https://bloxorz.org/
 
    Description:
    1) Tap two finger to show this help.The aim of the game is to get the block to fall into the square hole at the end of each stage. There are 33 stages to complete.
    2) To move the block around the world, wipe left, right, up and down arrow keys. Be careful not to fall off the edges. The level will be restarted if this happens.
    3) Bridges and switches are located in many levels. The switches are activated when they are pressed down by the block. You do not need to stay resting on the switch to keep bridges closed.
    4) There are two types of switches: "Heavy" x-shaped ones and "soft" octagon ones... Soft switches (octagons) are activated when any part of your block presses it. Hard switches (x's) require much more pressure, so your block must be standing on its end to activate them.
    5) When activated, each switch may behave differently. Some will swap the bridges from open to closed to open each time it is used. Some will create bridges permanently. Green or red colored squares will flash to indicate which bridges are being operated.
    6) Orange tiles are more fragile than the rest of the land. If your block stands up vertically on an orange tile, the tile will give way and your block will fall through.
    7) Finally, there is a third type of switch shaped like this: ( ) It teleports your block to different locations, splitting it into two smaller blocks at the same time. These can be controlled individually and will rejoin into a normal block when both are places next to each other.
    8) You can select which small block to use at any time by click the block. Small blocks can still operate soft switches, but they aren't big enough to activate heavy switches. Also, small blocks cannot go through the exit hole -- only a complete block can finish the stage.
    9) Remember the passcode for each stage. It is located in the top right corner. You can skip straight back to each stage later on by going to "Load Stage" in the main menu and entering the 6 digit level code.

*/

namespace puzzles::bloxorz{

constexpr auto PUZ_HOLE = '#';
constexpr auto PUZ_SPACE = ' ';
constexpr auto PUZ_GOAL = '.';
constexpr auto PUZ_ONE = '1';
constexpr auto PUZ_TWO = '2';
constexpr auto PUZ_ORANGE = '!';

enum ESwitchActionType {stSwap, stOn, stOff};

using puz_switch = pair<bool,        // heavy or not
    vector<pair<int,    // bridge index
    ESwitchActionType> > > ;
using puz_bridge = pair<vector<Position>,
    bool                // on or off
    >;
using puz_splitter = vector<Position>;

constexpr Position offset[] = {
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
        string_view str = strs[r - 2];
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
        if (string_view name = v.name(); name == "switch") {
            puz_switch switch_;
            auto p = parse_position(v.attribute("position").value());
            switch_.first = string(v.attribute("type").value()) == "heavy";
            {
                string action = v.attribute("action").value();
                pair<int, ESwitchActionType> pr;
                qi::phrase_parse(action.begin(), action.end(),
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
        } else if (name == "bridge") {
            puz_bridge bridge = {
                parse_positions(v.attribute("position").value()),
                string(v.attribute("state").value()) == "on"
            };
            for (auto& p2 : bridge.first)
                p2 += os;
            m_bridges.push_back(bridge);
        } else if (name == "splitter") {
            auto p = parse_position(v.attribute("position").value());
            auto splitter = parse_positions(v.attribute("locations").value());
            for (auto& p2 : splitter)
                p2 += os;
            m_splitters[p + os] = splitter;
        } else if (name == "teleporter") {
            auto p = parse_position(v.attribute("position").value());
            auto p2 = parse_position(v.attribute("location").value());
            m_teleporters[p + os] = p2 + os;
        }
    }
}

struct puz_state
{
    puz_state(const puz_game& g);
    const Position& goal() const {return m_game->m_goal;}
    bool operator<(const puz_state& x) const {
        return tie(m_blocks, m_bridges) < tie(x.m_blocks, x.m_bridges);
    }
    bool operator==(const puz_state& x) const {
        return tie(m_blocks, m_bridges) == tie(x.m_blocks, x.m_bridges);
    }
    bool is_hole(const Position& p) const {return m_game->cells(p) == PUZ_HOLE && !m_bridges.contains(p);}
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

    const puz_game* m_game = nullptr;
    array<Position, 2> m_blocks;
    bool m_split;
    vector<bool> m_bridge_states;
    set<Position> m_bridges;
    string m_move;
};

puz_state::puz_state(const puz_game& g)
    : m_game(&g), m_blocks(g.m_blocks), m_split(false)
{
    for (auto& bridge : m_game->m_bridges) {
        m_bridge_states.push_back(bridge.second);
        if (bridge.second)
            m_bridges.insert(bridge.first.begin(), bridge.first.end());
    }
}

bool puz_state::make_move(int n, int dir)
{
    static string_view moves = "lrud";
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
    auto i = m_game->m_switches.find(p);
    if (i == m_game->m_switches.end()) return false;

    auto& switch_ = i->second;
    if (!heavy_included && switch_.first) return false;

    for (auto& [index, type] : switch_.second) {
        bool on = 
            type == stOn ? true :
            type == stOff ? false :
            !m_bridge_states[index];
        if (on == m_bridge_states[index]) continue;

        auto& bridge = m_game->m_bridges[index];
        if ((m_bridge_states[index] = on))
            m_bridges.insert(bridge.first.begin(), bridge.first.end());
        else
            for (auto& p : bridge.first)
                m_bridges.erase(p);
    }
    return true;
}

bool puz_state::check_splitter(const Position& p)
{
    auto i = m_game->m_splitters.find(p);
    if (i == m_game->m_splitters.end()) return false;
    m_split = true;
    m_blocks[0] = i->second[0];
    m_blocks[1] = i->second[1];
    return true;
}

bool puz_state::check_teleporter(const Position& p)
{
    auto i = m_game->m_teleporters.find(p);
    if (i == m_game->m_teleporters.end()) return false;
    m_blocks[0] = m_blocks[1] = i->second;
    return true;
}

void puz_state::gen_children(list<puz_state>& children) const
{
    for (int i = 0; i < 4; ++i) {
        int movable = m_split ? 2 : 1;
        for (int n = 0; n < movable; ++n)
            if (children.push_back(*this); !children.back().make_move(n, i))
                children.pop_back();
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
        println(out);
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
