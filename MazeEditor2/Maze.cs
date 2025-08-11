using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using ReactiveUI.SourceGenerators;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;

namespace MazeEditor2
{
    public enum MazeMovement
    {
        None,
        Up,
        Down,
        Left,
        Right
    }

    public partial class Maze : ReactiveObject
    {
        [Reactive]
        public partial Position Size { get; set; } = new Position(8, 8);
        [Reactive]
        public partial Position MousePosition { get; set; } = new Position(-1, -1);
        [Reactive]
        public partial MazeMovement CurMovement { get; set; } = MazeMovement.Right;
        public Position CurOffset => CurMovement switch
        {
            MazeMovement.Up => Position.Up,
            MazeMovement.Down => Position.Down,
            MazeMovement.Left => Position.Left,
            MazeMovement.Right => Position.Right,
            _ => Position.Zero
        };

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
                this.RaiseAndSetIfChanged(ref field, value);
                if (value)
                    Width = Height;
                this.RaisePropertyChanged(nameof(Width));
            }
        }

        public ObservableCollection<Position> SelectedPositions { get; private set; } = [new Position()];
        [Reactive]
        public partial Position SelectedPosition { get; private set; }

        private Position AdjustedPosition(Position p)
        {
            int nArea = Height * Width;
            int n = (p.Row * Width + p.Col + nArea) % nArea;
            return new Position(n / Width, n % Width);
        }

        public void SetSelectedPosition(Position p)
        {
            SelectedPositions.Clear();
            SelectedPositions.Add(AdjustedPosition(p));
            Refresh();
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
            Refresh();
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
        [Reactive]
        public partial int RefreshCount { get; set; }

        public char? GetObject(Position p) =>
            pos2obj.TryGetValue(p, out var ch) ? ch : null;

        public void SetObject(Position p, char ch)
        {
            pos2obj[p] = ch;
            CurObj = ch;
        }

        public ReactiveCommand<Unit, Unit> FillAll { get; set; }
        public ReactiveCommand<Unit, Unit> FillBorderCells { get; set; }

        public bool IsHorzWall(Position p) => HorzWall.Contains(p);
        public bool IsVertWall(Position p) => VertWall.Contains(p);
        public bool IsDot(Position p) => Dots.Contains(p);

        public void SetHorzWall(Position p, bool isRemove = false)
        {
            if (!HasWall)
                return;
            if (isRemove)
                HorzWall.Remove(p);
            else
                HorzWall.Add(p);
            Refresh();
        }

        public void ToggleHorzWall(Position p)
        {
            if (!HasWall)
                return;
            if (!HorzWall.Remove(p))
                HorzWall.Add(p);
            Refresh();
        }

        public void SetVertWall(Position p, bool isRemove = false)
        {
            if (!HasWall)
                return;
            if (isRemove)
                VertWall.Remove(p);
            else
                VertWall.Add(p);
            Refresh();
        }

        public void ToggleVertWall(Position p)
        {
            if (!HasWall)
                return;
            if (!VertWall.Remove(p))
                VertWall.Add(p);
            Refresh();
        }

        public void ToggleDot(Position p)
        {
            if (!Dots.Remove(p))
                Dots.Add(p);
            Refresh();
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
                var strs = value.Split(["`\r\n"], StringSplitOptions.None)
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
                Refresh();
            }
        }

        public ReactiveCommand<Unit, Unit> ClearAll { get; set; }
        public ReactiveCommand<Unit, Unit> ClearWalls { get; set; }
        public ReactiveCommand<Unit, Unit> ClearChars { get; set; }

        public Maze()
        {
            var hasWall = this.WhenAnyValue(x => x.HasWall);
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
                Refresh();
            }, hasWall);
            EncloseSelectedCells = ReactiveCommand.Create(() =>
            {
                foreach (var p in SelectedPositions)
                {
                    if (!SelectedPositions.Contains(p + Position.Up))
                        HorzWall.Add(p);
                    if (!SelectedPositions.Contains(p + Position.Down))
                        HorzWall.Add(new Position(p.Row + 1, p.Col));
                    if (!SelectedPositions.Contains(p + Position.Left))
                        VertWall.Add(p);
                    if (!SelectedPositions.Contains(p + Position.Right))
                        VertWall.Add(new Position(p.Row, p.Col + 1));
                }
                Refresh();
            }, hasWall);
            FillAll = ReactiveCommand.Create(() =>
            {
                for (int r = 0; r < Height; r++)
                    for (int c = 0; c < Width; c++)
                        pos2obj[new Position(r, c)] = CurObj;
                Refresh();
            });
            FillBorderCells = ReactiveCommand.Create(() =>
            {
                for (int r = 0; r < Height; r++)
                    pos2obj[new Position(r, 0)] = pos2obj[new Position(r, Width - 1)] = CurObj;
                for (int c = 0; c < Width; c++)
                    pos2obj[new Position(0, c)] = pos2obj[new Position(Height - 1, c)] = CurObj;
                Refresh();
            });
            ClearWalls = ReactiveCommand.Create(() =>
            {
                HorzWall.Clear();
                VertWall.Clear();
                Refresh();
            });
            ClearChars = ReactiveCommand.Create(() =>
            {
                SetSelectedPosition(new Position());
                pos2obj.Clear();
                CurObj = ' ';
                Refresh();
            });
            ClearAll = ReactiveCommand.Create(() =>
            {
                ClearWalls.Execute().Subscribe();
                ClearChars.Execute().Subscribe();
            });
            SelectedPositions.ToObservableChangeSet()
                .Subscribe(_ => SelectedPosition = SelectedPositions.FirstOrDefault());
            this.WhenAnyValue(x => x.Size).Subscribe(sz =>
            {
                IsSquare = sz.Row == sz.Col;
                Refresh();
            });
            this.WhenAnyValue(x => x.HasWall).Subscribe(_ =>
                ClearWalls.Execute().Subscribe());
        }

        public void Refresh() => ++RefreshCount;

        public void MoveSelectedPosition(Position os) =>
            SetSelectedPosition(SelectedPosition + os);
    }
}