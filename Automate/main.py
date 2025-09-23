from analyze_pixel import analyze_pixel_line_and_store, report_analysis_results, analyze_pixel_column_and_store, \
    process_pixel_line_results, process_pixel_column_results, get_combined_pixel_colors, compress_colors_to_codes, \
    create_grid_string

# --- 主程序 ---
if __name__ == "__main__":
    # 图像信息
    image_path = 'Parks/Level_01.png'

    stored_line_results = analyze_pixel_line_and_store(image_path, y_coord=300, start_x=0, end_x=1180)
    # report_analysis_results(stored_line_results)
    processed_line_list = process_pixel_line_results(stored_line_results)
    print(processed_line_list)

    print("\n" + "="*50 + "\n")

    stored_column_results = analyze_pixel_column_and_store(image_path, x_coord=100, start_y=200, end_y=1380)
    # report_analysis_results(stored_column_results)
    processed_column_list = process_pixel_column_results(stored_column_results)
    print(processed_column_list)

    print("\n" + "="*50 + "\n")

    combined_colors = get_combined_pixel_colors(image_path, processed_line_list, processed_column_list)
    print(combined_colors)

    coded_matrix = compress_colors_to_codes(combined_colors)
    print(coded_matrix)

    grid_string = create_grid_string(coded_matrix)
    print(grid_string)

