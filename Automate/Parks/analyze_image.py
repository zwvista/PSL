from PIL import Image

def process_pixel_line_results(results, threshold=50):
    """
    筛选像素块结果，只保留重复次数超过阈值的，并返回它们的起始X坐标和长度。

    参数:
    results (list): analyze_pixel_line_and_store 函数返回的 PixelStreak 对象列表。
    threshold (int): 重复次数的最低门槛。

    返回:
    list: 一个包含元组的列表，每个元组的格式为 (起始X坐标, 长度)。
    """
    if results is None:
        return []

    processed_list = []
    for streak in results:
        # 筛选：检查重复次数是否超过门槛
        if streak.count >= threshold:
            # 添加到结果列表，格式为 (起始X坐标, 长度)
            processed_list.append((streak.position[0], streak.count))

    return processed_list

def process_pixel_column_results(results, threshold=50):
    """
    筛选像素列块结果，只保留重复次数超过阈值的，并返回它们的起始Y坐标和长度。

    参数:
    results (list): analyze_pixel_column_and_store 函数返回的 PixelStreak 对象列表。
    threshold (int): 重复次数的最低门槛。

    返回:
    list: 一个包含元组的列表，每个元组的格式为 (起始Y坐标, 长度)。
    """
    if results is None:
        return []

    processed_list = []
    for streak in results:
        if streak.count >= threshold:
            processed_list.append((streak.position[1], streak.count))
    return processed_list


def get_combined_pixel_colors(image_path, line_results, column_results, offset_x=10, offset_y=10):
    """
    根据行和列的分析结果，结合偏移量，提取原始图像中指定位置的像素颜色。

    参数:
    image_path (str): 图像文件的路径。
    line_results (list): process_pixel_line_results 函数返回的列表，格式为 [(x, count), ...]。
    column_results (list): process_pixel_column_results 函数返回的列表，格式为 [(y, count), ...]。
    offset_x (int): X方向的偏移量，默认为10。
    offset_y (int): Y方向的偏移量，默认为10。

    返回:
    list: 一个二维列表，其中包含所有组合坐标点的像素颜色。
          如果发生错误，返回 None。
    """
    try:
        with Image.open(image_path) as img:
            width, height = img.size

            # 提取所有经过偏移的 X 和 Y 坐标
            x_coords = [item[0] + offset_x for item in line_results]
            y_coords = [item[0] + offset_y for item in column_results]

            # 检查所有计算出的坐标是否在图像尺寸内
            if not all(0 <= x < width for x in x_coords) or not all(0 <= y < height for y in y_coords):
                print(f"错误：计算出的坐标超出了图像尺寸 ({width}x{height})。")
                print(f"计算出的X坐标: {x_coords}")
                print(f"计算出的Y坐标: {y_coords}")
                return None

            pixels = img.load()

            # 创建二维数组来存储颜色结果
            color_matrix = []

            for y in y_coords:
                row_colors = []
                for x in x_coords:
                    # 获取指定坐标的像素颜色
                    color = pixels[x, y]
                    row_colors.append(color)
                color_matrix.append(row_colors)

            return color_matrix

    except FileNotFoundError:
        print(f"错误：找不到文件 '{image_path}'。请确保路径正确。")
        return None
    except Exception as e:
        print(f"发生了未知错误: {e}")
        return None


def compress_colors_to_codes(color_matrix):
    """
    将二维颜色数组中的颜色替换为数字代码。
    第一种出现的颜色记作0，第二种记作1，以此类推。

    参数:
    color_matrix (list): 由 get_combined_pixel_colors 函数返回的二维颜色数组。

    返回:
    list: 一个二维列表，其中包含对应颜色的数字代码。
    """
    if not color_matrix:
        return []

    # 使用字典存储颜色到数字的映射
    color_to_code = {}
    next_code = 0

    # 创建新的二维数组来存储数字代码
    coded_matrix = []

    for row in color_matrix:
        coded_row = []
        for color in row:
            # 如果颜色不在字典中，分配一个新的代码
            if color not in color_to_code:
                color_to_code[color] = next_code
                next_code += 1

            # 将颜色代码添加到行中
            coded_row.append(color_to_code[color])
        coded_matrix.append(coded_row)

    return coded_matrix


def create_grid_string(coded_matrix):
    """
    根据颜色数字矩阵生成一个代表网格布局的多行字符串。

    参数:
    coded_matrix (list): 由 compress_colors_to_codes 返回的二维数字矩阵。

    返回:
    str: 一个表示网格布局的多行字符串。
    """
    if not coded_matrix or not coded_matrix[0]:
        return ""

    m = len(coded_matrix)
    n = len(coded_matrix[0])

    output_lines = []

    # 遍历字符串网格的每一行
    for r2 in range(2 * m + 1):
        line_chars = []
        # 遍历字符串网格的每一列
        for c2 in range(2 * n + 2):

            # 规则 1：一行的最后一个字符
            if c2 == 2 * n + 1:
                line_chars.append('`')
                continue

            # 规则 2：网格线的交点或单元格中心
            if (r2 % 2 == 0 and c2 % 2 == 0) or (r2 % 2 != 0 and c2 % 2 != 0):
                line_chars.append(' ')
                continue

            # 规则 3：最上或最下边界的水平线
            if (r2 == 0 or r2 == 2 * m) and c2 % 2 != 0:
                line_chars.append('-')
                continue

            # 规则 4：最左或最右边界的垂直线
            if (c2 == 0 or c2 == 2 * n) and r2 % 2 != 0:
                line_chars.append('|')
                continue

            # 规则 5：内部水平线
            if r2 % 2 == 0 and c2 % 2 != 0:
                r1 = r2 // 2
                c1 = (c2 - 1) // 2
                # 比较相邻单元格的数字
                if coded_matrix[r1 - 1][c1] == coded_matrix[r1][c1]:
                    line_chars.append(' ')
                else:
                    line_chars.append('-')
                continue

            # 规则 6：内部垂直线
            if r2 % 2 != 0 and c2 % 2 == 0:
                r1 = (r2 - 1) // 2
                c1 = c2 // 2
                # 比较相邻单元格的数字
                if coded_matrix[r1][c1 - 1] == coded_matrix[r1][c1]:
                    line_chars.append(' ')
                else:
                    line_chars.append('|')
                continue

        output_lines.append("".join(line_chars))

    return "\n".join(output_lines)