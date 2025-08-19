using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Xml.Linq;

namespace MazeEditor2
{
    public readonly record struct Position(int Row, int Col)
        : IComparable<Position>
    {
        public Position() : this(0, 0) { }

        public static Position operator +(Position a, Position b) =>
            new(a.Row + b.Row, a.Col + b.Col);

        public static Position operator -(Position a, Position b) =>
            new(a.Row - b.Row, a.Col - b.Col);

        public int CompareTo(Position other)
        {
            int rowCmp = Row.CompareTo(other.Row);
            if (rowCmp != 0) return rowCmp;
            return Col.CompareTo(other.Col);
        }

        public static bool operator <(Position a, Position b) =>
            a.Row < b.Row || (a.Row == b.Row && a.Col < b.Col);

        public static bool operator >(Position a, Position b) =>
            a.Row > b.Row || (a.Row == b.Row && a.Col > b.Col);

        public static bool operator <=(Position a, Position b) =>
            a < b || a == b;

        public static bool operator >=(Position a, Position b) =>
            a > b || a == b;

        public override string ToString() =>
            $"({Row},{Col})";

        public static readonly Position Zero = new(0, 0);
        public static readonly Position Up = new(-1, 0);
        public static readonly Position Down = new(1, 0);
        public static readonly Position Left = new(0, -1);
        public static readonly Position Right = new(0, 1);
    }
}
