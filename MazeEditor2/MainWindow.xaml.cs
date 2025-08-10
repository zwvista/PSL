using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Controls.Ribbon;
using ReactiveUI;
using System.Reactive.Linq;
using DynamicData.Binding;

namespace MazeEditor2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : RibbonWindow
    {
        readonly Maze maze = new();
        public MainWindow()
        {
            InitializeComponent();
            this.Loaded += (s, e) => DrawMaze();
            Canvas.SizeChanged += (s, e) => DrawMaze();
            this.DataContext = maze;
            maze.WhenAnyValue(x => x.RefreshCount)
                .Subscribe(_ =>DrawMaze());
        }
        void DrawMaze()
        {
            // 清除Canvas上已有的元素
            Canvas.Children.Clear();

            // 获取Canvas的实际尺寸
            double canvasWidth = Canvas.ActualWidth;
            double canvasHeight = Canvas.ActualHeight;

            // 定义棋盘参数
            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(Canvas.ActualWidth, Canvas.ActualHeight) * 0.8 / Math.Max(rows, cols);
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;
            double startX = (Canvas.ActualWidth - boardWidth) / 2;
            double startY = (Canvas.ActualHeight - boardHeight) / 2;

            // 绘制棋盘格子
            for (int r = 0; r <= rows; r++)
                for (int c = 0; c < cols; c++)
                {
                    var p = new Position(r, c);
                    var hasWall = maze.HasWall && maze.HorzWall.Contains(p);
                    var horizontalLine = new Line
                    {
                        Stroke = hasWall ? Brushes.Brown : Brushes.Black,
                        StrokeThickness = hasWall ? 4 : 2,
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
            double circleRadius = cellSize * 0.1;
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
            // 只处理左键点击
            if (e.ChangedButton != MouseButton.Left) return;

            Point clickPosition = e.GetPosition(Canvas);

            // 棋盘参数
            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(Canvas.ActualWidth, Canvas.ActualHeight) * 0.8 / Math.Max(rows, cols);
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;
            double startX = (Canvas.ActualWidth - boardWidth) / 2;
            double startY = (Canvas.ActualHeight - boardHeight) / 2;

            // 检查是否点击在棋盘区域内
            if (!IsPointInBoard(clickPosition, startX, startY, boardWidth, boardHeight)) return;

            // 检测修饰键状态
            bool isCtrlPressed = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isAltPressed = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            // 确定点击区域类型
            ClickAreaType areaType = DetermineClickAreaType(clickPosition, startX, startY, cellSize, rows, cols);
            if (areaType == ClickAreaType.Vertex)
            {
                // 获取顶点坐标（如果是顶点）
                int vertexCol = (int)Math.Round((clickPosition.X - startX) / cellSize);
                int vertexRow = (int)Math.Round((clickPosition.Y - startY) / cellSize);
                vertexCol = Math.Min(Math.Max(vertexCol, 0), cols);
                vertexRow = Math.Min(Math.Max(vertexRow, 0), rows);
            }
            else
            {
                // 获取格子坐标（如果是格子内部或线段）
                int col = (int)((clickPosition.X - startX) / cellSize);
                int row = (int)((clickPosition.Y - startY) / cellSize);
                col = Math.Min(Math.Max(col, 0), cols - 1);
                row = Math.Min(Math.Max(row, 0), rows - 1);
                var p = new Position(row, col);

                if (areaType == ClickAreaType.Cell)
                {
                    if (isAltPressed)
                        maze.ToggleSelectedPosition(p);
                    else
                        maze.SetSelectedPosition(p);
                }
            }
        }

        bool IsPointInBoard(Point point, double startX, double startY, double width, double height)
        {
            return point.X >= startX && point.X <= startX + width &&
                   point.Y >= startY && point.Y <= startY + height;
        }

        ClickAreaType DetermineClickAreaType(Point clickPos, double startX, double startY, double cellSize, int rows, int cols)
        {
            const double lineTolerance = 3.0; // 线段点击容差范围（像素）
            const double vertexTolerance = 5.0; // 顶点点击容差范围（像素）

            // 1. 检查是否点击在顶点上
            for (int r = 0; r <= rows; r++)
            {
                for (int c = 0; c <= cols; c++)
                {
                    double vertexX = startX + c * cellSize;
                    double vertexY = startY + r * cellSize;

                    if (Math.Abs(clickPos.X - vertexX) < vertexTolerance &&
                        Math.Abs(clickPos.Y - vertexY) < vertexTolerance)
                    {
                        return ClickAreaType.Vertex;
                    }
                }
            }

            // 2. 检查是否点击在水平线上
            for (int r = 0; r <= rows; r++)
            {
                double lineY = startY + r * cellSize;
                if (Math.Abs(clickPos.Y - lineY) < lineTolerance)
                {
                    return ClickAreaType.HorizontalLine;
                }
            }

            // 3. 检查是否点击在垂直线上
            for (int c = 0; c <= cols; c++)
            {
                double lineX = startX + c * cellSize;
                if (Math.Abs(clickPos.X - lineX) < lineTolerance)
                {
                    return ClickAreaType.VerticalLine;
                }
            }

            // 4. 默认是格子内部
            return ClickAreaType.Cell;
        }

        // 点击区域类型枚举
        public enum ClickAreaType
        {
            Cell,           // 格子内部
            HorizontalLine, // 水平线段
            VerticalLine,   // 垂直线段
            Vertex          // 顶点
        }

        void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            Point mousePos = e.GetPosition(Canvas);

            // 棋盘参数
            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(Canvas.ActualWidth, Canvas.ActualHeight) * 0.8 / Math.Max(rows, cols);
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;
            double startX = (Canvas.ActualWidth - boardWidth) / 2;
            double startY = (Canvas.ActualHeight - boardHeight) / 2;

            // 检查是否点击在棋盘区域内
            if (!IsPointInBoard(mousePos, startX, startY, boardWidth, boardHeight))
                maze.MousePosition = new Position(-1, -1);
            else
            {
                // 计算当前所在方格
                int col = (int)((mousePos.X - startX) / cellSize);
                int row = (int)((mousePos.Y - startY) / cellSize);
                col = Math.Min(Math.Max(col, 0), cols - 1);
                row = Math.Min(Math.Max(row, 0), rows - 1);
                maze.MousePosition = new Position(row, col);
            }
        }

        void Canvas_PreviewKeyDown(object sender, KeyEventArgs e)
        {

            // 检测修饰键状态
            bool isCtrlPressed = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isAltPressed = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            switch (e.Key)
            {
                case Key.Left:
                    maze.MoveSelectedPosition(Position.Left);
                    e.Handled = true;
                    break;
                case Key.Right:
                    maze.MoveSelectedPosition(Position.Right);
                    e.Handled = true;
                    break;
                case Key.Up:
                    maze.MoveSelectedPosition(Position.Up);
                    e.Handled = true;
                    break;
                case Key.Down:
                    maze.MoveSelectedPosition(Position.Down);
                    e.Handled = true;
                    break;
                default:
                    char ch = e.Key == Key.Enter ? maze.CurObj : e.Key.ToString().FirstOrDefault();
                    maze.SetObject(maze.SelectedPosition, ch);
                    maze.MoveSelectedPosition(maze.CurOffset);
                    e.Handled = true;
                    break;
            }
        }

        private void Canvas_Loaded(object sender, RoutedEventArgs e)
        {
            Canvas.Focus();
            Keyboard.Focus(Canvas);
        }
    }
}
