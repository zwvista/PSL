#pragma once

#include <format>
#include <boost/operators.hpp>

struct Position :
    std::pair<int, int>,
    boost::additive<Position>
{
    Position() {}
    Position(int v1, int v2) : std::pair<int, int>(v1, v2) {}
    Position& operator+=(const Position& x) {
        first += x.first, second += x.second;
        return *this;
    }
    Position& operator-=(const Position& x) {
        first -= x.first, second -= x.second;
        return *this;
    }
};

void parse_position(const string& str, Position& p);
void parse_positions(const string& str, vector<Position>& vp);

namespace std {
    template<>
    struct formatter<Position> : formatter<string> {
        auto format(Position p, format_context& ctx) const {
            return formatter<string>::format(
                std::format("({},{})", p.first, p.second),
                ctx);
        }
    };
    inline ostream& operator<<(ostream& out, const Position& p)
    {
        out << std::format("{}", p);
        return out;
    }
}
