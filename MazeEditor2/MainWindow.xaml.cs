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
            this.Loaded += (s, e) => DrawChessBoard();
            ChessBoardCanvas.SizeChanged += (s, e) => DrawChessBoard();
        }

        private void Canvas_KeyDown(object sender, KeyEventArgs e)
        {

        }
        private void DrawChessBoard()
        {
            // 清除Canvas上已有的元素
            ChessBoardCanvas.Children.Clear();

            // 获取Canvas的实际尺寸
            double canvasWidth = ChessBoardCanvas.ActualWidth;
            double canvasHeight = ChessBoardCanvas.ActualHeight;

            // 定义棋盘参数
            int rows = 3;
            int cols = 3;
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
                ChessBoardCanvas.Children.Add(horizontalLine);
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
                ChessBoardCanvas.Children.Add(verticalLine);
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

                    Canvas.SetLeft(textBlock, startX + j * cellSize + cellSize / 2 - textBlock.ActualWidth / 2);
                    Canvas.SetTop(textBlock, startY + i * cellSize + cellSize / 2 - textBlock.ActualHeight / 2);
                    ChessBoardCanvas.Children.Add(textBlock);
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
                    ChessBoardCanvas.Children.Add(circle);
                }
            }
        }
    }
}
