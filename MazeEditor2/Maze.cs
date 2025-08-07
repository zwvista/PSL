using ReactiveUI;
using ReactiveUI.SourceGenerators;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;

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

    public partial class Maze : ReactiveObject
    {
        [Reactive]
        public partial Position Size { get; set; } = new Position(8, 8);
        [Reactive]
        public partial MazeMovement CurMovement { get; set; } = MazeMovement.MoveRight;

        public int Height
        {
            get => Size.Row;
            set => Size = new Position(value, IsSquare ? value : Size.Col);
        }

        public int Width
        {
            get => Size.Col;
            set => Size = new Position(Size.Row, value);
        }

        public bool IsSquare
        {
            get;
            set
            {
                if (value)
                    Width = Height;
                this.RaiseAndSetIfChanged(ref field, value);
                this.RaisePropertyChanged(nameof(Width));
            }
        }

        [Reactive]
        public partial List<Position> SelectedPositions { get; private set; } = [new Position()];

        public Position SelectedPosition => SelectedPositions.Last();

        private Position AdjustedPosition(Position p)
        {
            int nArea = Height * Width;
            int n = (p.Row * Width + p.Col + nArea) % nArea;
            return new Position(n / Width, n % Width);
        }

        public void SetSelectedPosition(Position p) =>
            SelectedPositions = [AdjustedPosition(p)];

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
        }

        private Dictionary<Position, char> pos2obj = [];

        [Reactive]
        public partial bool HasWall { get; set; } = false;

        [Reactive]
        public partial HashSet<Position> HorzWall { get; private set; } = [];
        [Reactive]
        public partial HashSet<Position> VertWall { get; private set; } = [];
        [Reactive]
        public partial HashSet<Position> Dots { get; private set; } = [];

        [Reactive]
        public partial char CurObj { get; set; } = ' ';

        public char? GetObject(Position p) =>
            pos2obj.TryGetValue(p, out var ch) ? ch : null;

        public void SetObject(Position p, char ch)
        {
            pos2obj[p] = ch;
            CurObj = ch;
        }

        public bool IsHorzWall(Position p) => HorzWall.Contains(p);
        public bool IsVertWall(Position p) => VertWall.Contains(p);
        public bool IsDot(Position p) => Dots.Contains(p);

        public void ToggleHorzWall(Position p)
        {
            if (!HorzWall.Remove(p))
                HorzWall.Add(p);
        }

        public void ToggleVertWall(Position p)
        {
            if (!VertWall.Remove(p))
                VertWall.Add(p);
        }

        public void ToggleDot(Position p)
        {
            if (!Dots.Remove(p))
                Dots.Add(p);
        }

        public ReactiveCommand<Unit, Unit> FillBorderLines { get; set; }
        public ReactiveCommand<Unit, Unit> EncloseSelectedCells { get; set; }

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
                ClearAll.Execute();
                var strs = value.Split(["`\n"], StringSplitOptions.None)
                                .Where(s => s != "").ToList();
                if (HasWall)
                {
                    Size = new Position(strs.Count / 2, strs[0].Length / 2);
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
                    Size = new Position(strs.Count, strs[0].Length);
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
            }
        }

        public ReactiveCommand<Unit, Unit> ClearAll { get; set; }
        public ReactiveCommand<Unit, Unit> ClearWalls { get; set; }
        public ReactiveCommand<Unit, Unit> ClearChars { get; set; }

        public Maze()
        {
            var hasWall = this.WhenAnyValue(x => x.HasWall)
                .Select(v => v)
                .DistinctUntilChanged();
            FillBorderLines = ReactiveCommand.Create(() =>
            {
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
            }, hasWall);
            EncloseSelectedCells = ReactiveCommand.Create(() =>
            {
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
            }, hasWall);
            ClearWalls = ReactiveCommand.Create(() =>
            {
                HorzWall.Clear();
                VertWall.Clear();
            });
            ClearChars = ReactiveCommand.Create(() =>
            {
                SelectedPositions = new List<Position> { new Position() };
                pos2obj.Clear();
                CurObj = ' ';
            });
            ClearAll = ReactiveCommand.Create(() =>
            {
                ClearWalls.Execute();
                ClearChars.Execute();
            });
        }
    }
}