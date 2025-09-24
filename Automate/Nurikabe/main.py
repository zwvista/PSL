from Nurikabe.analyze_image import recognize_digits, format_digit_matrix
from common import analyze_pixel_line_and_store, analyze_pixel_column_and_store, report_analysis_results, \
    process_pixel_line_results, process_pixel_column_results

def get_level_str_from_image(image_path):
    stored_line_results = analyze_pixel_line_and_store(image_path, y_coord=210, start_x=0, end_x=1180)
    processed_line_list = process_pixel_line_results(stored_line_results)
    # print(processed_line_list)
    # print("\n" + "="*50 + "\n")
    stored_column_results = analyze_pixel_column_and_store(image_path, x_coord=10, start_y=200, end_y=1380)
    processed_column_list = process_pixel_column_results(stored_column_results)
    # print(processed_column_list)
    digits_matrix = recognize_digits(image_path, processed_line_list, processed_column_list)
    # print(digits_matrix)
    level_str = format_digit_matrix(digits_matrix)
    return level_str


def main():
    relative_path = "../Hitori/"
    with open(f"{relative_path}Levels.txt", "w") as text_file:
        for i in range(1, 3):
            # 图像信息
            image_path = f'{relative_path}level_images/Level_{i:03d}.png'
            level_str = get_level_str_from_image(image_path)
            node = f"""<level id="{i}">
  <![CDATA[
{level_str}
  ]]>
</level>
"""
            text_file.write(node)

# --- 主程序 ---
if __name__ == "__main__":
    main()
