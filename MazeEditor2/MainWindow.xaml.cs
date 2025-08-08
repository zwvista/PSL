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
            double cellSize = Math.Min(canvasWidth, canvasHeight) * 0.8 / Math.Max(rows, cols);
            // 确保cellSize不小于10
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;

            // 计算棋盘左上角位置使其居中
            double startX = (canvasWidth - boardWidth) / 2;
            double startY = (canvasHeight - boardHeight) / 2;

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
            Point clickPosition = e.GetPosition(MazeCanvas);

            // 计算点击位置相对于棋盘的坐标
            double canvasWidth = MazeCanvas.ActualWidth;
            double canvasHeight = MazeCanvas.ActualHeight;

            int rows = maze.Height;
            int cols = maze.Width;
            double cellSize = Math.Min(canvasWidth, canvasHeight) * 0.8 / Math.Max(rows, cols);
            // 确保cellSize不小于10
            cellSize = Math.Max(cellSize, 10);
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;

            double startX = (canvasWidth - boardWidth) / 2;
            double startY = (canvasHeight - boardHeight) / 2;

            // 检查是否点击在棋盘区域内
            if (clickPosition.X >= startX && clickPosition.X <= startX + boardWidth &&
                clickPosition.Y >= startY && clickPosition.Y <= startY + boardHeight)
            {
                int col = (int)((clickPosition.X - startX) / cellSize);
                int row = (int)((clickPosition.Y - startY) / cellSize);

                // 确保索引在有效范围内
                col = Math.Min(Math.Max(col, 0), cols - 1);
                row = Math.Min(Math.Max(row, 0), rows - 1);

                MessageBox.Show($"点击了格子: 行 {row + 1}, 列 {col + 1}");
            }
        }
    }
}
