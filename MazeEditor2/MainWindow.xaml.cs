using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Ribbon;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Xceed.Wpf.Toolkit;

namespace MazeEditor2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : RibbonWindow
    {
        readonly Maze maze = new();
        readonly double tolerance = 8.0; // 点击容差范围（像素）
        double cellSize, boardWidth, boardHeight, startX, startY;
        // 棋盘参数
        int rows => maze.Height;
        int cols => maze.Width;
        public MainWindow()
        {
            InitializeComponent();
            this.Loaded += (s, e) => DrawMaze();
            Canvas.SizeChanged += (s, e) => DrawMaze();
            this.DataContext = maze;
            maze.WhenAnyValue(x => x.RefreshCount)
                .Subscribe(_ =>DrawMaze());
            CommandManager.AddExecutedHandler(Canvas, OnExecuted);
            CommandManager.AddCanExecuteHandler(Canvas, OnCanExecute);
        }
        void DrawMaze()
        {
            // 清除Canvas上已有的元素
            Canvas.Children.Clear();

            // 获取Canvas的实际尺寸
            double canvasWidth = Canvas.ActualWidth;
            double canvasHeight = Canvas.ActualHeight;

            // 定义棋盘参数
            cellSize = Math.Max(Math.Min(canvasWidth, canvasHeight) * 0.8 / Math.Max(rows, cols), 10);
            boardWidth = cols * cellSize;
            boardHeight = rows * cellSize;
            startX = (canvasWidth - boardWidth) / 2;
            startY = (canvasHeight - boardHeight) / 2;

            // 绘制棋盘格子
            for (int r = 0; r <= rows; r++)
                for (int c = 0; c < cols; c++)
                {
                    var p = new Position(r, c);
                    var hasWall = maze.HasWall && maze.HorzWall.Contains(p);
                    var horizontalLine = new Line
                    {
                        Stroke = hasWall ? Brushes.Brown : Brushes.Black,
                        StrokeThickness = hasWall ? 8 : 2,
                        X1 = startX + c * cellSize,
                        X2 = startX + (c + 1) * cellSize,
                        Y1 = startY + r * cellSize,
                        Y2 = startY + r * cellSize
                    };
                    Canvas.Children.Add(horizontalLine);
                }

            for (int c = 0; c <= cols; c++)
                for (int r = 0; r < rows; r++)
                {
                    var p = new Position(r, c);
                    var hasWall = maze.HasWall && maze.VertWall.Contains(p);
                    var verticalLine = new Line
                    {
                        Stroke = hasWall ? Brushes.Brown : Brushes.Black,
                        StrokeThickness = hasWall ? 4 : 2,
                        X1 = startX + c * cellSize,
                        X2 = startX + c * cellSize,
                        Y1 = startY + r * cellSize,
                        Y2 = startY + (r + 1) * cellSize
                    };
                    Canvas.Children.Add(verticalLine);
                }

            // 绘制所有格子背景和交互区域
            for (int r = 0; r < rows; r++)
                for (int c = 0; c < cols; c++)
                {
                    var p = new Position(r, c);
                    bool isSelected = maze.SelectedPositions.Contains(p);
                    // 创建格子背景矩形
                    Rectangle cellBackground = new Rectangle
                    {
                        Width = cellSize,
                        Height = cellSize,
                        Fill = isSelected ? Brushes.Green : Brushes.Transparent, // 中央格子设为绿色
                        Opacity = 0.3 // 设置透明度使背景不太突兀
                    };

                    Canvas.SetLeft(cellBackground, startX + c * cellSize);
                    Canvas.SetTop(cellBackground, startY + r * cellSize);
                    Canvas.Children.Add(cellBackground);
                }

            // 在每个格子中央添加字符"O"
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    var ch = maze.GetObject(new Position(r, c)) ?? ' ';
                    TextBlock textBlock = new()
                    {
                        Text = ch.ToString(),
                        FontSize = cellSize * 0.4,
                        FontWeight = FontWeights.Bold,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center
                    };

                    // 先测量文本大小
                    textBlock.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
                    textBlock.Arrange(new Rect(0, 0, textBlock.DesiredSize.Width, textBlock.DesiredSize.Height));

                    // 计算中心位置
                    double centerX = startX + c * cellSize + cellSize / 2;
                    double centerY = startY + r * cellSize + cellSize / 2;

                    // 设置位置（考虑文本自身大小）
                    Canvas.SetLeft(textBlock, centerX - textBlock.DesiredSize.Width / 2);
                    Canvas.SetTop(textBlock, centerY - textBlock.DesiredSize.Height / 2);

                    Canvas.Children.Add(textBlock);
                }
            }

            // 在所有顶点处绘制小圆
            double circleRadius = cellSize * 0.2;
            for (int r = 0; r <= rows; r++)
            {
                for (int c = 0; c <= cols; c++)
                {
                    var p = new Position(r, c);
                    if (!(maze.HasWall && maze.Dots.Contains(p)))
                        continue;
                    Ellipse circle = new Ellipse
                    {
                        Width = circleRadius * 2,
                        Height = circleRadius * 2,
                        Fill = Brushes.Red,
                        Stroke = Brushes.Black,
                        StrokeThickness = 1
                    };

                    Canvas.SetLeft(circle, startX + c * cellSize - circleRadius);
                    Canvas.SetTop(circle, startY + r * cellSize - circleRadius);
                    Canvas.Children.Add(circle);
                }
            }
        }

        void Canvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Canvas.Focus();
            Keyboard.Focus(Canvas);

            // 只处理左键点击
            if (e.ChangedButton != MouseButton.Left) return;

            Point clickPosition = e.GetPosition(Canvas);
            var pointX = clickPosition.X - startX;
            var pointY = clickPosition.Y - startY;
            var t = maze.HasWall ? tolerance : 0.0;

            // 检查是否点击在棋盘区域内
            if (!(pointX >= -t && pointX <= boardWidth + t &&
                pointY >= -t && pointY <= boardHeight + t)) return;

            // 检测修饰键状态
            bool isCtrlPressed = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isAltPressed = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            // 计算当前所在方格
            int col = (int)((pointX + t) / cellSize);
            int row = (int)((pointY + t) / cellSize);
            col = Math.Min(Math.Max(col, 0), cols);
            row = Math.Min(Math.Max(row, 0), rows);
            var onHorzLine = pointY >= row * cellSize - t && pointY <= row * cellSize + t;
            var onVertLine = pointX >= col * cellSize - t && pointX <= col * cellSize + t;

            // 检查是否点击在顶点、水平线或垂直线上
            if (onHorzLine || onVertLine)
            {
                var p = new Position(row, col);
                // 1. 检查是否点击在顶点上
                if (onHorzLine && onVertLine)
                    maze.ToggleDot(p);
                // 2. 检查是否点击在水平线上
                else if (onHorzLine)
                    maze.ToggleHorzWall(p);
                // 3. 检查是否点击在垂直线上
                else if (onVertLine)
                    maze.ToggleVertWall(p);
            }
            else
            {
                var p = new Position(Math.Min(row, rows - 1), Math.Min(col, cols - 1));
                // 4. 默认是格子内部
                if (isAltPressed)
                    maze.ToggleSelectedPosition(p);
                else
                    maze.SetSelectedPosition(p);
            }
        }

        void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            Point mousePos = e.GetPosition(Canvas);
            var pointX = mousePos.X - startX;
            var pointY = mousePos.Y - startY;

            // 检查是否点击在棋盘区域内
            if (!(pointX >= 0 && pointX <= boardWidth &&
                  pointY >= 0 && pointY <= boardHeight))
                maze.MousePosition = new Position(-1, -1);
            else
            {
                // 计算当前所在方格
                int col = (int)(pointX / cellSize);
                int row = (int)(pointY / cellSize);
                col = Math.Min(Math.Max(col, 0), cols - 1);
                row = Math.Min(Math.Max(row, 0), rows - 1);
                maze.MousePosition = new Position(row, col);
            }
        }

        void Canvas_KeyDown(object sender, KeyEventArgs e)
        {
            // 检测修饰键状态
            bool isCtrlPressed = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isAltPressed = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            switch (e.Key)
            {
                case Key.Left:
                    if (!isCtrlPressed)
                        maze.MoveSelectedPosition(Position.Left);
                    else
                        maze.SetVertWall(maze.SelectedPosition, isShiftPressed);
                    e.Handled = true;
                    break;
                case Key.Right:
                    if (!isCtrlPressed)
                        maze.MoveSelectedPosition(Position.Right);
                    else
                        maze.SetVertWall(maze.SelectedPosition + Position.Right, isShiftPressed);
                    e.Handled = true;
                    break;
                case Key.Up:
                    if (!isCtrlPressed)
                        maze.MoveSelectedPosition(Position.Up);
                    else
                        maze.SetHorzWall(maze.SelectedPosition, isShiftPressed);
                    e.Handled = true;
                    break;
                case Key.Down:
                    if (!isCtrlPressed)
                        maze.MoveSelectedPosition(Position.Down);
                    else
                        maze.SetHorzWall(maze.SelectedPosition + Position.Down, isShiftPressed);
                    e.Handled = true;
                    break;
                case Key.Enter:
                    maze.SetObject(maze.SelectedPosition, maze.CurObj);
                    maze.MoveSelectedPosition(maze.CurOffset);
                    e.Handled = true;
                    break;
            }
        }

        void Canvas_Loaded(object sender, RoutedEventArgs e)
        {
            Canvas.Focus();
            Keyboard.Focus(Canvas);
        }

        void Canvas_TextInput(object sender, TextCompositionEventArgs e)
        {
            // 处理可打印字符
            if (!string.IsNullOrEmpty(e.Text))
            {
                maze.SetObject(maze.SelectedPosition, e.Text[0]);
                maze.MoveSelectedPosition(maze.CurOffset);
            }
        }
        void OnExecuted(object sender, ExecutedRoutedEventArgs e)
        {
            if (e.Command == ApplicationCommands.Copy)
            {
                Clipboard.SetData(DataFormats.Text, maze.Data);
                e.Handled = true;
            }
            else if (e.Command == ApplicationCommands.Paste)
            {
                if (Clipboard.ContainsText())
                    maze.Data = Clipboard.GetText();
                e.Handled = true;
            }
        }

        void OnCanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            if (e.Command == ApplicationCommands.Copy)
            {
                e.CanExecute = true;
                e.Handled = true;
            }
            else if (e.Command == ApplicationCommands.Paste)
            {
                e.CanExecute = Clipboard.ContainsText();
                e.Handled = true;
            }
        }
        private void ButtonSpinner_Spin(object sender, SpinEventArgs e)
        {
            ButtonSpinner spinner = (ButtonSpinner)sender;
            TextBox txtBox = (TextBox)spinner.Content;

            int value = String.IsNullOrEmpty(txtBox.Text) ? 0 : Convert.ToInt32(txtBox.Text);
            if (e.Direction == SpinDirection.Increase)
                value++;
            else
                value--;
            txtBox.Text = value.ToString();
        }
    }
}
