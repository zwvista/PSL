using System;
using System.Collections.Generic;
using System.Linq;

namespace MazeEditor2
{
    public enum MazeMovement
    {
        None,
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight
    }

    public interface IMazeDelegate
    {
        void UpdateMazeSize();
        void UpdateMazeView();
        void UpdateSelectedPosition();
        void UpdateMousePosition(Position p);
        void UpdateIsSquare();
        void UpdateCurObject();
        void UpdateHasWall();
        MazeMovement CurMovement { get; }
    }

    public class Maze
    {
        private Position size = new Position(8, 8);
        public IMazeDelegate? Delegate { get; set; }

        public int Height
        {
            get => size.Row;
            set
            {
                size = new Position(value, isSquare_ ? value : size.Col);
                Delegate?.UpdateMazeSize();
                Delegate?.UpdateMazeView();
            }
        }

        public int Width
        {
            get => size.Col;
            set
            {
                size = new Position(size.Row, value);
                Delegate?.UpdateMazeSize();
                Delegate?.UpdateMazeView();
            }
        }

        private bool isSquare_ = true;
        public bool IsSquare
        {
            get => isSquare_;
            set
            {
                isSquare_ = value;
                Delegate?.UpdateIsSquare();
                if (value)
                    Width = Height;
            }
        }

        public List<Position> SelectedPositions { get; private set; } = new List<Position> { new Position() };

        public Position SelectedPosition => SelectedPositions.Last();

        private Position AdjustedPosition(Position p)
        {
            int nArea = Height * Width;
            int n = (p.Row * Width + p.Col + nArea) % nArea;
            return new Position(n / Width, n % Width);
        }

        public void SetSelectedPosition(Position p)
        {
            SelectedPositions = new List<Position> { AdjustedPosition(p) };
            Delegate?.UpdateSelectedPosition();
            Delegate?.UpdateMazeView();
        }

        public void ToggleSelectedPosition(Position p)
        {
            var p2 = AdjustedPosition(p);
            int i = SelectedPositions.IndexOf(p2);
            if (i >= 0)
            {
                if (SelectedPositions.Count > 1)
                    SelectedPositions.RemoveAt(i);
            }
            else
            {
                SelectedPositions.Add(p2);
            }
            Delegate?.UpdateSelectedPosition();
            Delegate?.UpdateMazeView();
        }

        private Dictionary<Position, char> pos2obj = new Dictionary<Position, char>();
        private bool hasWall_ = false;

        public bool HasWall
        {
            get => hasWall_;
            set
            {
                hasWall_ = value;
                Delegate?.UpdateHasWall();
                Delegate?.UpdateMazeView();
            }
        }

        public HashSet<Position> HorzWall { get; private set; } = new HashSet<Position>();
        public HashSet<Position> VertWall { get; private set; } = new HashSet<Position>();
        public HashSet<Position> Dots { get; private set; } = new HashSet<Position>();

        public char CurObj { get; private set; } = ' ';

        public char? GetObject(Position p) =>
            pos2obj.TryGetValue(p, out var ch) ? ch : (char?)null;

        public void SetObject(Position p, char ch)
        {
            pos2obj[p] = ch;
            CurObj = ch;
            Delegate?.UpdateCurObject();
            Delegate?.UpdateMazeView();
        }

        public bool IsHorzWall(Position p) => HorzWall.Contains(p);
        public bool IsVertWall(Position p) => VertWall.Contains(p);
        public bool IsDot(Position p) => Dots.Contains(p);

        public void ToggleHorzWall(Position p)
        {
            if (!HorzWall.Remove(p))
                HorzWall.Add(p);
            Delegate?.UpdateMazeView();
        }

        public void ToggleVertWall(Position p)
        {
            if (!VertWall.Remove(p))
                VertWall.Add(p);
            Delegate?.UpdateMazeView();
        }

        public void ToggleDot(Position p)
        {
            if (!Dots.Remove(p))
                Dots.Add(p);
            Delegate?.UpdateMazeView();
        }

        public void FillBorderLines()
        {
            if (!HasWall) return;
            for (int r = 0; r < Height; r++)
            {
                VertWall.Add(new Position(r, 0));
                VertWall.Add(new Position(r, Width));
            }
            for (int c = 0; c < Width; c++)
            {
                HorzWall.Add(new Position(0, c));
                HorzWall.Add(new Position(Height, c));
            }
            Delegate?.UpdateMazeView();
        }

        public void EncloseSelectedCells()
        {
            if (!HasWall) return;
            foreach (var p in SelectedPositions)
            {
                if (!SelectedPositions.Contains(new Position(p.Row - 1, p.Col)))
                    HorzWall.Add(p);
                if (!SelectedPositions.Contains(new Position(p.Row + 1, p.Col)))
                    HorzWall.Add(new Position(p.Row + 1, p.Col));
                if (!SelectedPositions.Contains(new Position(p.Row, p.Col - 1)))
                    VertWall.Add(p);
                if (!SelectedPositions.Contains(new Position(p.Row, p.Col + 1)))
                    VertWall.Add(new Position(p.Row, p.Col + 1));
            }
            Delegate?.UpdateMazeView();
        }

        public string Data
        {
            get
            {
                var str = "";
                if (HasWall)
                {
                    for (int r = 0; r <= Height; r++)
                    {
                        for (int c = 0; c < Width; c++)
                        {
                            var p = new Position(r, c);
                            str += IsDot(p) ? "O" : " ";
                            str += IsHorzWall(p) ? "-" : " ";
                        }
                        str += IsDot(new Position(r, Width)) ? "O" : " ";
                        str += "`\n";
                        if (r == Height) break;
                        for (int c = 0; c <= Width; c++)
                        {
                            var p = new Position(r, c);
                            str += IsVertWall(p) ? "|" : " ";
                            if (c == Width) break;
                            var ch = GetObject(p);
                            str += ch.HasValue ? ch.Value.ToString() : " ";
                        }
                        str += "`\n";
                    }
                }
                else
                {
                    for (int r = 0; r < Height; r++)
                    {
                        for (int c = 0; c < Width; c++)
                        {
                            var p = new Position(r, c);
                            var ch = GetObject(p);
                            str += ch.HasValue ? ch.Value.ToString() : " ";
                        }
                        str += "`\n";
                    }
                }
                return str;
            }
            set
            {
                ClearAll();
                var strs = value.Split(new[] { "`\n" }, StringSplitOptions.None)
                                .Where(s => s != "").ToList();
                if (HasWall)
                {
                    size = new Position(strs.Count / 2, strs[0].Length / 2);
                    for (int r = 0; r <= Height; r++)
                    {
                        var str1 = strs[2 * r];
                        for (int c = 0; c < Width; c++)
                        {
                            if (str1[2 * c] == 'O')
                                Dots.Add(new Position(r, c));
                            if (str1[2 * c + 1] == '-')
                                HorzWall.Add(new Position(r, c));
                        }
                        if (str1[2 * Width] == 'O')
                            Dots.Add(new Position(r, Width));
                        if (r == Height) break;
                        var str2 = strs[2 * r + 1];
                        for (int c = 0; c <= Width; c++)
                        {
                            if (str2[2 * c] == '|')
                                VertWall.Add(new Position(r, c));
                            if (c == Width) break;
                            var ch = str2[2 * c + 1];
                            if (ch != ' ')
                                pos2obj[new Position(r, c)] = ch;
                        }
                    }
                }
                else
                {
                    size = new Position(strs.Count, strs[0].Length);
                    for (int r = 0; r < Height; r++)
                    {
                        var str = strs[r];
                        for (int c = 0; c < Width; c++)
                        {
                            var ch = str[c];
                            if (ch != ' ')
                                pos2obj[new Position(r, c)] = ch;
                        }
                    }
                }
                UpdateMaze();
            }
        }

        public void ClearAll()
        {
            ClearWalls();
            ClearChars();
        }

        public void ClearWalls()
        {
            HorzWall.Clear();
            VertWall.Clear();
            Delegate?.UpdateMazeView();
        }

        public void ClearChars()
        {
            SelectedPositions = new List<Position> { new Position() };
            pos2obj.Clear();
            CurObj = ' ';
            Delegate?.UpdateSelectedPosition();
            Delegate?.UpdateCurObject();
            Delegate?.UpdateMazeView();
        }

        public void UpdateMaze()
        {
            Delegate?.UpdateMazeSize();
            Delegate?.UpdateIsSquare();
            Delegate?.UpdateSelectedPosition();
            Delegate?.UpdateMazeView();
            Delegate?.UpdateHasWall();
        }
    }
}
