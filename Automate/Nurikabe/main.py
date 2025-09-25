from common import analyze_horizontal_line, analyze_vertical_line, report_analysis_results, \
    process_pixel_long_results, recognize_digits, level_node_string


def format_digit_matrix(matrix):
    """
    将数字识别结果的二维列表转换为格式化的多行字符串。
    空格保持为空格，每行字符直接拼接，行末加反引号 `。

    参数:
        matrix: 二维列表，例如 [[' ', ' ', ...], ['3', ' ', ...], ...]

    返回:
        格式化后的字符串
    """
    lines = []
    for row in matrix:
        # 将每行的字符直接拼接（空格保留）
        line = ''.join(row)
        # 去掉行尾的空格，但保留中间和开头的空格
        # 如果你希望保留所有空格（包括尾部），则不需要 strip
        # 这里我们保留原始空格，仅在末尾加反引号
        lines.append(line + '`')

    # 合并为多行字符串
    result = '\n'.join(lines)
    return result

def get_level_str_from_image(image_path):
    horizontal_line_results = analyze_horizontal_line(image_path, y_coord=210, start_x=0, end_x=1180)
    processed_line_list = process_pixel_long_results(horizontal_line_results, is_horizontal=True)
    # print(processed_line_list)
    # print("\n" + "="*50 + "\n")
    vertical_line_results = analyze_vertical_line(image_path, x_coord=10, start_y=200, end_y=1380)
    processed_column_list = process_pixel_long_results(vertical_line_results, is_horizontal=False)
    # print(processed_column_list)
    digits_matrix = recognize_digits(image_path, processed_line_list, processed_column_list)
    # print(digits_matrix)
    level_str = format_digit_matrix(digits_matrix)
    return level_str


def main():
    level_image_path = ""
    for i in range(1, 3):
        # 图像信息
        image_path = f'{level_image_path}Level_{i:03d}.png'
        print("正在处理图片 " + image_path)
        level_str = get_level_str_from_image(image_path)
        node = level_node_string(i, level_str)
        with open(f"Levels.txt", "a") as text_file:
            text_file.write(node)

# --- 主程序 ---
if __name__ == "__main__":
    main()
