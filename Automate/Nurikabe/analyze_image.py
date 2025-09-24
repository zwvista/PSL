import cv2
import pytesseract
import easyocr

def recognize_digits(image_path, line_list, column_list):
    """
    读取图片中多个区域的数字，不进行图像预处理。

    参数:
        image_path: 图片路径
        line_list: [(x1, w1), (x2, w2), ...] 水平列（x坐标和宽度）
        column_list: [(y1, h1), (y2, h2), ...] 垂直行（y坐标和高度）

    返回:
        二维列表，每个元素是识别出的字符或空格
    """
    # 读取图像
    img = cv2.imread(image_path)

    # 存储识别结果
    result = []

    reader = easyocr.Reader(['en'])  # 初始化，只加载英文模型
    for row_idx, (y, h) in enumerate(column_list):
        row_result = []
        for col_idx, (x, w) in enumerate(line_list):
            # 裁剪感兴趣区域(ROI)
            roi = img[y:y+h, x:x+w]

            text = reader.readtext(roi)[0][1]

            # # 图像预处理（可选但推荐）
            # gray = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
            # gray = cv2.convertScaleAbs(gray, alpha=1.5, beta=0)
            # _, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
            #
            # # 使用 Tesseract 识别文字，这里我们假设只识别数字
            # custom_config = r'--oem 3 --psm 6 outputbase digits'
            # text = pytesseract.image_to_string(thresh, config=custom_config).strip()
            # # text = pytesseract.image_to_string(roi, config=custom_config).strip()

            # 将识别的结果添加到当前行的结果列表中
            row_result.append(text if text else ' ')  # 如果没有识别出内容则返回 ' '

        # 将当前行的结果添加到最终结果列表中
        result.append(row_result)

    return result


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
