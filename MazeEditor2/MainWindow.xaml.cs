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
            double boardWidth = cols * cellSize;
            double boardHeight = rows * cellSize;

            // 计算棋盘左上角位置使其居中
            double startX = (canvasWidth - boardWidth) / 2;
            double startY = (canvasHeight - boardHeight) / 2;

            // 绘制棋盘格子
            for (int i = 0; i <= rows; i++)
            {
                Line horizontalLine = new Line
                {
                    Stroke = Brushes.Black,
                    StrokeThickness = 2,
                    X1 = startX,
                    X2 = startX + boardWidth,
                    Y1 = startY + i * cellSize,
                    Y2 = startY + i * cellSize
                };
                MazeCanvas.Children.Add(horizontalLine);
            }

            for (int j = 0; j <= cols; j++)
            {
                Line verticalLine = new Line
                {
                    Stroke = Brushes.Black,
                    StrokeThickness = 2,
                    X1 = startX + j * cellSize,
                    X2 = startX + j * cellSize,
                    Y1 = startY,
                    Y2 = startY + boardHeight
                };
                MazeCanvas.Children.Add(verticalLine);
            }

            // 在每个格子中央添加字符"O"
            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    TextBlock textBlock = new()
                    {
                        Text = "O",
                        FontSize = cellSize * 0.4,
                        FontWeight = FontWeights.Bold,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center
                    };

                    // 先测量文本大小
                    textBlock.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
                    textBlock.Arrange(new Rect(0, 0, textBlock.DesiredSize.Width, textBlock.DesiredSize.Height));

                    // 计算中心位置
                    double centerX = startX + j * cellSize + cellSize / 2;
                    double centerY = startY + i * cellSize + cellSize / 2;

                    // 设置位置（考虑文本自身大小）
                    Canvas.SetLeft(textBlock, centerX - textBlock.DesiredSize.Width / 2);
                    Canvas.SetTop(textBlock, centerY - textBlock.DesiredSize.Height / 2);

                    MazeCanvas.Children.Add(textBlock);
                }
            }

            // 在所有顶点处绘制小圆
            double circleRadius = cellSize * 0.1;
            for (int i = 0; i <= rows; i++)
            {
                for (int j = 0; j <= cols; j++)
                {
                    Ellipse circle = new Ellipse
                    {
                        Width = circleRadius * 2,
                        Height = circleRadius * 2,
                        Fill = Brushes.Red,
                        Stroke = Brushes.Black,
                        StrokeThickness = 1
                    };

                    Canvas.SetLeft(circle, startX + j * cellSize - circleRadius);
                    Canvas.SetTop(circle, startY + i * cellSize - circleRadius);
                    MazeCanvas.Children.Add(circle);
                }
            }

            // 为每个格子添加可点击的透明按钮
            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols; col++)
                {
                    Button cellButton = new Button
                    {
                        Width = cellSize,
                        Height = cellSize,
                        Background = Brushes.Transparent,
                        BorderBrush = Brushes.Transparent,
                        BorderThickness = new Thickness(0),
                        Tag = new Point(col, row) // 存储格子坐标
                    };

                    cellButton.Click += CellButton_Click;

                    Canvas.SetLeft(cellButton, startX + col * cellSize);
                    Canvas.SetTop(cellButton, startY + row * cellSize);

                    MazeCanvas.Children.Add(cellButton);
                }
            }
        }
        private void CellButton_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = sender as Button;
            Point position = (Point)clickedButton.Tag;

            int col = (int)position.X;
            int row = (int)position.Y;

            MessageBox.Show($"点击了格子: 行 {row + 1}, 列 {col + 1}");
        }
    }
}
