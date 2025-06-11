#include "stdafx.h"
#include "astar_solver.h"
#include "solve_puzzle.h"

/*
    iOS Game: Smart Matches ~ Puzzle Games
*/

namespace puzzles::MatchstickEquation{

constexpr auto PUZ_REMOVE = 0;
constexpr auto PUZ_ADD = 1;
constexpr auto PUZ_MOVE = 2;

//  . - .1
// 2|   |4
//  . - .8
//16|   |32
//  . - .64
constexpr int matchstick_digits[] = {
    0b1110111, // 0
    0b0100100, // 1
    0b1011101, // 2
    0b1101101, // 3
    0b0101110, // 4
    0b1101011, // 5
    0b1111011, // 6
    0b0100101, // 7
    0b1111111, // 8
    0b1101111, // 9
};

// operator from, operator to, matchstick to remove, matchstick to add
const map<char, tuple<char, int, int>> op2op = {
    {'+', {'-', 1, 0}},
    {'-', {'+', 0, 1}},
    {'*', {'/', 1, 0}},
    {'/', {'*', 0, 1}},
};

struct puz_game
{
    string m_id;
    int m_remove_count, m_add_count;
    // digit from * 10 + digit to, matchstick to remove, matchstick to add
    vector<pair<int, int>> m_digit2digit;
    vector<int> m_digits, m_indexes;
    char m_operator;

    puz_game(const vector<string>& strs, const xml_node& level);
};

puz_game::puz_game(const vector<string>& strs, const xml_node& level)
    : m_id(level.attribute("id").value())
    , m_digit2digit(100)
    , m_indexes{0}
{
    int n = level.attribute("matchsticks").as_int();
    string action = level.attribute("action").value();
    if (action == "remove")
        m_remove_count = n, m_add_count = 0;
    else if (action == "add")
        m_remove_count = 0, m_add_count = n;
    else
        m_remove_count = m_add_count = n;
    for (int i = 0; i < 10; ++i) {
        int d1 = matchstick_digits[i];
        for (int j = 0; j < 10; ++j) {
            int d2 = matchstick_digits[j];
            auto& v = m_digit2digit[i * 10 + j];
            int n1 = 0, n2 = 0;
            for (int k = 0; k < 7; ++k) {
                int b1 = d1 & 1 << k;
                int b2 = d2 & 1 << k;
                if (b1 && !b2) ++n1;
                else if (!b1 && b2) ++n2;
            }
            v = {n1, n2};
        }
    }
    string equation = level.attribute("equation").value();
    int operand1, operand2, result;
    qi::phrase_parse(equation.begin(), equation.end(),
        qi::int_[phx::ref(operand1) = qi::_1] >>
        qi::char_[phx::ref(m_operator) = qi::_1] >>
        qi::int_[phx::ref(operand2) = qi::_1] >> '=' >>
        qi::int_[phx::ref(result) = qi::_1], qi::space);
    auto f = [&](int n) {
        int index = m_indexes.back();
        do {
            m_digits.push_back(n % 10);
            ++index;
            n /= 10;
        } while(n != 0);
        m_indexes.push_back(index);
    };
    f(operand1);
    f(operand2);
    f(result);
}

struct puz_state
{
    puz_state(const puz_game& g);
    bool operator<(const puz_state& x) const { 
        return tie(m_digits, m_operator, m_index) < tie(x.m_digits, x.m_operator, x.m_index);
    }
    void prepare_equation(int& operand1, int& operand2, int& result) const;
    bool is_valid_equation() const;
    void make_move(int digit_to);
    void make_move(char op_to);

    //solve_puzzle interface
    bool is_goal_state() const {
        return get_heuristic() == 0 && is_valid_equation();
    }
    void gen_children(list<puz_state>& children) const;
    unsigned int get_heuristic() const { return m_remove_count + m_add_count; }
    unsigned int get_distance(const puz_state& child) const { return m_distance; }
    void dump_move(ostream& out) const {}
    ostream& dump(ostream& out) const;

    const puz_game* m_game = nullptr;
    int m_remove_count, m_add_count;
    vector<int> m_digits;
    char m_operator;
    int m_index = 0;
    unsigned int m_distance = 0;
};

puz_state::puz_state(const puz_game& g)
: m_game(&g), m_remove_count(g.m_remove_count), m_add_count(g.m_add_count)
, m_digits(g.m_digits), m_operator(g.m_operator)
{
}

void puz_state::prepare_equation(int& operand1, int& operand2, int& result) const
{
    auto f = [&](int& n, int idx) {
        auto& indexes = m_game->m_indexes;
        n = 0;
        for (int i = indexes[idx + 1] - 1; i >= indexes[idx]; --i)
            n = n * 10 + m_digits[i];
    };
    f(operand1, 0);
    f(operand2, 1);
    f(result, 2);
}

bool puz_state::is_valid_equation() const
{
    int operand1, operand2, result;
    prepare_equation(operand1, operand2, result);
    switch(m_operator) {
    case '+':
        return operand1 + operand2 == result;
    case '-':
        return operand1 - operand2 == result;
    case '*':
        return operand1 * operand2 == result;
    case '/':
        return operand1 / operand2 == result && operand1 / operand2 * operand2 == operand1;
    default:
        return false;
    }
}

void puz_state::make_move(int digit_to)
{
    auto d = get_heuristic();
    int& digit_from = m_digits[m_index++];
    auto& [remove_count, add_count] = m_game->m_digit2digit[digit_from * 10 + digit_to];
    m_remove_count -= remove_count, m_add_count -= add_count;
    digit_from = digit_to;
    m_distance = d - get_heuristic();
}

void puz_state::make_move(char op_to)
{
    auto d = get_heuristic();
    ++m_index;
    if (m_operator != op_to) {
        auto& [op_to2, remove_count, add_count] = op2op.at(m_operator);
        m_remove_count -= remove_count, m_add_count -= add_count;
        m_operator = op_to;
    }
    m_distance = d - get_heuristic();
}

void puz_state::gen_children(list<puz_state>& children) const
{
    int sz = m_digits.size();
    if (m_index == sz) {
        children.push_back(*this);
        children.back().make_move(m_operator);
        auto& [op_to, remove_count, add_count] = op2op.at(m_operator);
        if (remove_count <= m_remove_count && add_count <= m_add_count) {
            children.push_back(*this);
            children.back().make_move(op_to);
        }
    } else if (m_index < sz) {
        int digit_from = m_digits[m_index];
        for (int digit_to = 0; digit_to < 10; ++digit_to) {
            auto& [remove_count, add_count] = m_game->m_digit2digit[digit_from * 10 + digit_to];
            if (remove_count <= m_remove_count && add_count <= m_add_count) {
                children.push_back(*this);
                children.back().make_move(digit_to);
            }
        }
    }
}

ostream& puz_state::dump(ostream& out) const
{
    int operand1, operand2, result;
    prepare_equation(operand1, operand2, result);
    out << format("{}{}{}={}\n", operand1, m_operator, operand2, result);
    return out;
}

}

void solve_puz_MatchstickEquation()
{
    using namespace puzzles::MatchstickEquation;
    solve_puzzle<puz_game, puz_state, puz_solver_astar<puz_state>>(
        "Puzzles/MatchstickEquation.xml", "Puzzles/MatchstickEquation.txt", solution_format::GOAL_STATE_ONLY);
}
