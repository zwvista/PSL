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

namespace MazeEditor2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : RibbonWindow
    {
        private readonly Maze maze = new();
        public MainWindow()
        {
            InitializeComponent();
            this.Loaded += (s, e) => DrawMaze();
            MazeCanvas.SizeChanged += (s, e) => DrawMaze();
            this.DataContext = maze;
            maze.WhenAnyValue(x => x.Size)
                .Subscribe(v =>
                {
                    maze.IsSquare = v.Row == v.Col;
                    DrawMaze();
                });
        }

        private void Canvas_KeyDown(object sender, KeyEventArgs e)
        {

        }
        private void DrawMaze()
        {
            // 清除Canvas上已有的元素
            MazeCanvas.Children.Clear();

            // 获取Canvas的实际尺寸
            double canvasWidth = MazeCanvas.ActualWidth;
            double canvasHeight = MazeCanvas.ActualHeight;

            // 定义棋盘参数
            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(MazeCanvas.ActualWidth, MazeCanvas.ActualHeight) * 0.8 / Math.Max(rows, cols);
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;
            double startX = (MazeCanvas.ActualWidth - boardWidth) / 2;
            double startY = (MazeCanvas.ActualHeight - boardHeight) / 2;

            // 绘制棋盘格子
            for (int r = 0; r <= rows; r++)
            {
                Line horizontalLine = new Line
                {
                    Stroke = Brushes.Black,
                    StrokeThickness = 2,
                    X1 = startX,
                    X2 = startX + boardWidth,
                    Y1 = startY + r * cellSize,
                    Y2 = startY + r * cellSize
                };
                MazeCanvas.Children.Add(horizontalLine);
            }

            for (int c = 0; c <= cols; c++)
            {
                Line verticalLine = new Line
                {
                    Stroke = Brushes.Black,
                    StrokeThickness = 2,
                    X1 = startX + c * cellSize,
                    X2 = startX + c * cellSize,
                    Y1 = startY,
                    Y2 = startY + boardHeight
                };
                MazeCanvas.Children.Add(verticalLine);
            }

            // 绘制所有格子背景和交互区域
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    // 创建格子背景矩形
                    Rectangle cellBackground = new Rectangle
                    {
                        Width = cellSize,
                        Height = cellSize,
                        Fill = (r == 1 && c == 1) ? Brushes.Green : Brushes.Transparent, // 中央格子设为绿色
                        Opacity = 0.3 // 设置透明度使背景不太突兀
                    };

                    Canvas.SetLeft(cellBackground, startX + c * cellSize);
                    Canvas.SetTop(cellBackground, startY + r * cellSize);
                    MazeCanvas.Children.Add(cellBackground);
                }
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

                    MazeCanvas.Children.Add(textBlock);
                }
            }

            // 在所有顶点处绘制小圆
            double circleRadius = cellSize * 0.1;
            for (int r = 0; r <= rows; r++)
            {
                for (int c = 0; c <= cols; c++)
                {
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
                    MazeCanvas.Children.Add(circle);
                }
            }

            // 启用鼠标事件
            MazeCanvas.MouseDown += MazeCanvas_MouseDown;
        }

        private void MazeCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            // 只处理左键点击
            if (e.ChangedButton != MouseButton.Left) return;

            Point clickPosition = e.GetPosition(MazeCanvas);

            // 棋盘参数
            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(MazeCanvas.ActualWidth, MazeCanvas.ActualHeight) * 0.8 / Math.Max(rows, cols);
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;
            double startX = (MazeCanvas.ActualWidth - boardWidth) / 2;
            double startY = (MazeCanvas.ActualHeight - boardHeight) / 2;

            // 检查是否点击在棋盘区域内
            if (!IsPointInBoard(clickPosition, startX, startY, boardWidth, boardHeight)) return;

            // 检测修饰键状态
            bool isCtrlPressed = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isAltPressed = Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt);
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            // 确定点击区域类型
            ClickAreaType areaType = DetermineClickAreaType(clickPosition, startX, startY, cellSize, rows, cols);

            // 获取格子坐标（如果是格子内部或线段）
            int col = (int)((clickPosition.X - startX) / cellSize);
            int row = (int)((clickPosition.Y - startY) / cellSize);
            col = Math.Min(Math.Max(col, 0), cols - 1);
            row = Math.Min(Math.Max(row, 0), rows - 1);

            // 获取顶点坐标（如果是顶点）
            int vertexCol = (int)Math.Round((clickPosition.X - startX) / cellSize);
            int vertexRow = (int)Math.Round((clickPosition.Y - startY) / cellSize);
            vertexCol = Math.Min(Math.Max(vertexCol, 0), cols);
            vertexRow = Math.Min(Math.Max(vertexRow, 0), rows);

            // 构建消息
            string message = BuildClickMessage(areaType, row, col, vertexRow, vertexCol, isCtrlPressed, isAltPressed, isShiftPressed);

            MessageBox.Show(message);
        }

        private bool IsPointInBoard(Point point, double startX, double startY, double width, double height)
        {
            return point.X >= startX && point.X <= startX + width &&
                   point.Y >= startY && point.Y <= startY + height;
        }

        private ClickAreaType DetermineClickAreaType(Point clickPos, double startX, double startY, double cellSize, int rows, int cols)
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

        private string BuildClickMessage(ClickAreaType areaType, int row, int col,
                                       int vertexRow, int vertexCol,
                                       bool ctrl, bool alt, bool shift)
        {
            StringBuilder sb = new StringBuilder();

            // 添加区域类型信息
            switch (areaType)
            {
                case ClickAreaType.Cell:
                    sb.AppendLine($"点击了格子内部: 行 {row + 1}, 列 {col + 1}");
                    break;
                case ClickAreaType.HorizontalLine:
                    sb.AppendLine($"点击了水平线段: 行 {row + 1}");
                    break;
                case ClickAreaType.VerticalLine:
                    sb.AppendLine($"点击了垂直线段: 列 {col + 1}");
                    break;
                case ClickAreaType.Vertex:
                    sb.AppendLine($"点击了顶点: 行 {vertexRow}, 列 {vertexCol}");
                    break;
            }

            // 添加修饰键信息
            sb.AppendLine($"Ctrl键: {(ctrl ? "按下" : "未按下")}");
            sb.AppendLine($"Alt键: {(alt ? "按下" : "未按下")}");
            sb.AppendLine($"Shift键: {(shift ? "按下" : "未按下")}");

            return sb.ToString();
        }

        // 点击区域类型枚举
        public enum ClickAreaType
        {
            Cell,           // 格子内部
            HorizontalLine, // 水平线段
            VerticalLine,   // 垂直线段
            Vertex          // 顶点
        }
    }
}
