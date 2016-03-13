#pragma once

#include <boost/format.hpp>
#include <boost/operators.hpp>

struct Position :
    std::pair<int, int>,
    boost::additive<Position>
{
    Position() {}
    Position(int v1, int v2) : std::pair<int, int>(v1, v2) {}
    Position& operator+=(const Position& x){
        first += x.first, second += x.second;
        return *this;
    }
    Position& operator-=(const Position& x){
        first -= x.first, second -= x.second;
        return *this;
    }
};

void parse_position(const string& str, Position& p);
void parse_positions(const string& str, vector<Position>& vp);

namespace std {
    inline ostream& operator<<(ostream& out, const Position& p)
    {
        out << boost::format("(%1%,%2%)") % p.first % p.second;
        return out;
    }
}
