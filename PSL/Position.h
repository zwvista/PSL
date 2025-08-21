#pragma once

#include <format>
#include <boost/operators.hpp>

struct Position :
    std::pair<int, int>,
    boost::additive<Position>
{
    constexpr Position() {}
    using std::pair<int, int>::pair;
    constexpr Position& operator+=(const Position& x) {
        first += x.first, second += x.second;
        return *this;
    }
    constexpr Position& operator-=(const Position& x) {
        first -= x.first, second -= x.second;
        return *this;
    }
    static const Position North;
    static const Position NorthEast;
    static const Position East;
    static const Position SouthEast;
    static const Position South;
    static const Position SouthWest;
    static const Position West;
    static const Position NorthWest;
    static const Position Zero;
    static const std::array<Position, 4> Directions4;
    static const std::array<Position, 8> Directions8;
    static const std::array<Position, 4> WallsOffset4;
};

inline constexpr Position Position::North{-1,  0};
inline constexpr Position Position::NorthEast{-1,  1};
inline constexpr Position Position::East{0,  1};
inline constexpr Position Position::SouthEast{1,  1};
inline constexpr Position Position::South{1,  0};
inline constexpr Position Position::SouthWest{1, -1};
inline constexpr Position Position::West{0, -1};
inline constexpr Position Position::NorthWest{-1, -1};
inline constexpr Position Position::Zero{0,  0};

inline constexpr std::array<Position, 4> Position::Directions4{
    Position::North,
    Position::East,
    Position::South,
    Position::West,
};

inline constexpr std::array<Position, 8> Position::Directions8{
    Position::North,
    Position::NorthEast,
    Position::East,
    Position::SouthEast,
    Position::South,
    Position::SouthWest,
    Position::West,
    Position::NorthWest,
};

inline constexpr std::array<Position, 4> Position::WallsOffset4{
    Position::Zero, // North
    Position::East,
    Position::South,
    Position::Zero, // West
};

Position parse_position(const std::string& str);
std::vector<Position> parse_positions(const std::string& str);

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
        out << format("{}", p);
        return out;
    }
}
