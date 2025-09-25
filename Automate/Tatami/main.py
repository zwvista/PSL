from common import analyze_horizontal_line, analyze_vertical_line, report_analysis_results, \
    process_pixel_long_results, process_pixel_short_results, recognize_digits, level_node_string


def recognize_walls(image_path, line_list, column_list):
    row_walls = set()
    col_walls = set()
    try:
        for col_idx, (x, w) in enumerate(line_list):
            vertical_line_results = analyze_vertical_line(image_path, x_coord=x+10, start_y=200, end_y=1380)
            processed_column_grid = process_pixel_short_results(vertical_line_results, is_horizontal=False)
            for row_idx, (y, h) in enumerate(processed_column_grid):
                if row_idx == 0 or row_idx == len(processed_column_grid) - 1 or h > 4:
                    row_walls.add((row_idx, col_idx))

        for row_idx, (y, h) in enumerate(column_list):
            horizontal_line_results = analyze_horizontal_line(image_path, y_coord=y+10, start_x=0, end_x=1180)
            processed_line_grid = process_pixel_short_results(horizontal_line_results, is_horizontal=True)
            for col_idx, (x, w) in enumerate(processed_line_grid):
                if col_idx == 0 or col_idx == len(processed_line_grid) - 1 or w > 4:
                    col_walls.add((row_idx, col_idx))

        return row_walls, col_walls

    except FileNotFoundError:
        print(f"错误：找不到文件 '{image_path}'。请确保路径正确。")
        return None
    except Exception as e:
        print(f"发生了未知错误: {e}")
        return None


def format_digit_matrix(matrix, walls):
    lines = []
    rows = len(matrix)
    cols = len(matrix[0])
    row_walls, col_walls = walls
    for r in range(rows + 1):
        line = ''
        for c in range(cols + 1):
            line += ' '
            if c == cols:
                break
            line += '-' if (r, c) in row_walls else ' '
        lines.append(line + '`')
        if r == rows:
            break
        digits = matrix[r]
        line = ''
        for c in range(cols + 1):
            line += '|' if (r, c) in col_walls else ' '
            if c == cols:
                break
            line += digits[c]
        lines.append(line + '`')

    # 合并为多行字符串
    result = '\n'.join(lines)
    return result

def get_level_str_from_image(image_path):
    horizontal_line_results = analyze_horizontal_line(image_path, y_coord=210, start_x=0, end_x=1180)
    processed_line_list = process_pixel_long_results(horizontal_line_results, is_horizontal=True)
    vertical_line_results = analyze_vertical_line(image_path, x_coord=10, start_y=200, end_y=1380)
    processed_column_list = process_pixel_long_results(vertical_line_results, is_horizontal=False)
    digits_matrix = recognize_digits(image_path, processed_line_list, processed_column_list)
    walls = recognize_walls(image_path, processed_line_list, processed_column_list)
    level_str = format_digit_matrix(digits_matrix, walls)
    return level_str


def main():
    level_image_path = "/Users/zwvista/Documents/Programs/Games/100LG/Tatami/"
    for i in range(1, 201):
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
