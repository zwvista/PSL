from PIL import Image


class PixelStreak:
    """
    用于存储连续像素块信息的类。
    """

    def __init__(self, position, color, count):
        self.position = position
        self.color = color
        self.count = count

    def __repr__(self):
        """
        用于在打印时提供友好的表示。
        """
        return f"PixelStreak(position={self.position}, color={self.color}, count={self.count})"


def analyze_pixel_line_and_store(image_path, y_coord, start_x, end_x):
    """
    分析图像中指定的一行像素，并将连续的像素块信息存储到 PixelStreak 对象列表中。

    参数:
    image_path (str): 图像文件的路径。
    y_coord (int): 要分析的像素行的Y坐标。
    start_x (int): 分析的起始X坐标。
    end_x (int): 分析的结束X坐标。

    返回:
    list: 一个包含 PixelStreak 对象实例的列表。
          如果发生错误，返回 None。
    """
    try:
        with Image.open(image_path) as img:
            width, height = img.size
            if not (0 <= y_coord < height and 0 <= start_x <= end_x < width):
                print(f"错误：请求的坐标范围超出了图像尺寸 ({width}x{height})。")
                return None

            pixels = img.load()
            results = []

            current_streak_color = None
            current_streak_count = 0
            current_streak_start_x = start_x

            for x in range(start_x, end_x + 1):
                current_pixel_color = pixels[x, y_coord]

                if current_streak_color is None:
                    current_streak_color = current_pixel_color
                    current_streak_count = 1
                    current_streak_start_x = x
                elif current_pixel_color == current_streak_color:
                    current_streak_count += 1
                else:
                    results.append(PixelStreak(
                        position=(current_streak_start_x, y_coord),
                        color=current_streak_color,
                        count=current_streak_count
                    ))
                    current_streak_color = current_pixel_color
                    current_streak_count = 1
                    current_streak_start_x = x

            if current_streak_count > 0:
                results.append(PixelStreak(
                    position=(current_streak_start_x, y_coord),
                    color=current_streak_color,
                    count=current_streak_count
                ))

            return results

    except FileNotFoundError:
        print(f"错误：找不到文件 '{image_path}'。请确保路径正确。")
        return None
    except Exception as e:
        print(f"发生了未知错误: {e}")
        return None


def analyze_pixel_column_and_store(image_path, x_coord, start_y, end_y):
    """
    分析图像中指定的一列像素，并将连续的像素块信息存储到 PixelStreak 对象列表中。

    参数:
    image_path (str): 图像文件的路径。
    x_coord (int): 要分析的像素列的X坐标。
    start_y (int): 分析的起始Y坐标。
    end_y (int): 分析的结束Y坐标。

    返回:
    list: 一个包含 PixelStreak 对象实例的列表。
          如果发生错误，返回 None。
    """
    try:
        with Image.open(image_path) as img:
            width, height = img.size
            if not (0 <= x_coord < width and 0 <= start_y <= end_y < height):
                print(f"错误：请求的坐标范围超出了图像尺寸 ({width}x{height})。")
                return None

            pixels = img.load()
            results = []

            current_streak_color = None
            current_streak_count = 0
            current_streak_start_y = start_y

            for y in range(start_y, end_y + 1):
                current_pixel_color = pixels[x_coord, y]

                if current_streak_color is None:
                    current_streak_color = current_pixel_color
                    current_streak_count = 1
                    current_streak_start_y = y
                elif current_pixel_color == current_streak_color:
                    current_streak_count += 1
                else:
                    results.append(PixelStreak(
                        position=(x_coord, current_streak_start_y),
                        color=current_streak_color,
                        count=current_streak_count
                    ))
                    current_streak_color = current_pixel_color
                    current_streak_count = 1
                    current_streak_start_y = y

            if current_streak_count > 0:
                results.append(PixelStreak(
                    position=(x_coord, current_streak_start_y),
                    color=current_streak_color,
                    count=current_streak_count
                ))

            return results

    except FileNotFoundError:
        print(f"错误：找不到文件 '{image_path}'。请确保路径正确。")
        return None
    except Exception as e:
        print(f"发生了未知错误: {e}")
        return None


def report_analysis_results(results):
    """
    打印分析结果列表。

    参数:
    results (list): 从分析函数返回的 PixelStreak 对象列表。
    """
    if results is None:
        print("无法生成报告，因为分析过程中发生了错误。")
        return

    print("--- 像素线/列分析报告 ---")
    if not results:
        print("在指定的范围内没有找到像素数据。")
        return

    for item in results:
        position = f"({item.position[0]}, {item.position[1]})"
        color_str = str(item.color)
        print(f"位置: {position:<15} 颜色: {color_str:<20} 重复次数: {item.count}")

    print("--- 报告结束 ---")

