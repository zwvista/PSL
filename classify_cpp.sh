#!/bin/bash

# 定义搜索的根目录，如果脚本在要搜索的目录执行，可以设置为 '.'
SEARCH_DIR="."

# 定义要查找的字符串
SEARCH_STRING="iOS Game: 100 Logic Games"

# 定义输出目录
OUTPUT_DIR="Archive"

# 定义输出文件名
# 包含目标字符串的文件路径及匹配行
OUTPUT_FILE_A="${OUTPUT_DIR}/Files_With_Logic_Game_Tag.txt"
# 不包含目标字符串的文件相对路径
OUTPUT_FILE_B="${OUTPUT_DIR}/Files_Without_Logic_Game_Tag.txt"

# 用于临时存储结果，以便后续排序
TEMP_FILE_A="${OUTPUT_DIR}/.temp_A.txt"
TEMP_FILE_B="${OUTPUT_DIR}/.temp_B.txt"

# 1. 确保输出目录存在
mkdir -p "$OUTPUT_DIR"

# 2. 清空或创建临时输出文件
> "$TEMP_FILE_A"
> "$TEMP_FILE_B"

# 清空或创建输出文件
> "$OUTPUT_FILE_A"
> "$OUTPUT_FILE_B"

# 3. 使用 find 命令查找文件
# -type f: 只查找文件
# -name "*.cpp": 查找以 .cpp 结尾的文件
# -path "*Puzzles*": 文件路径中包含 "Puzzles"
# -not -name "main.cpp": 排除文件名 main.cpp
# -not -name "stdafx.cpp": 排除文件名 stdafx.cpp
# -print0: 使用空字符作为分隔符，可以正确处理包含空格或特殊字符的文件名
find "$SEARCH_DIR" -type f \
    -name "*.cpp" \
    -path "*Puzzles*" \
    -not -name "main.cpp" \
    -not -name "stdafx.cpp" \
    -print0 |

# 4. 使用 while 循环处理找到的每个文件
while IFS= read -r -d $'\0' file_path; do
    
    # 获取相对路径，去除 $SEARCH_DIR/ 前缀
    # 使用 `sed` 安全地处理 $SEARCH_DIR 为 '.' 的情况
    relative_path=$(echo "$file_path" | sed "s#^${SEARCH_DIR}/##")

    # 查找匹配的行
    # -F: 固定字符串匹配
    # -m 1: 找到第一个匹配项后立即停止
    MATCHING_LINE=$(grep -F -m 1 "$SEARCH_STRING" "$file_path" 2>/dev/null)

    if [ -n "$MATCHING_LINE" ]; then
        # 如果找到匹配的行 (A文件)
        # 格式: 相对路径<换行符>匹配行
        echo "$relative_path" >> "$TEMP_FILE_A"
        echo "$MATCHING_LINE" >> "$TEMP_FILE_A"
    else
        # 如果没有找到匹配的行 (B文件)
        # 格式: 相对路径
        echo "$relative_path" >> "$TEMP_FILE_B"
    fi

done

# 5. 对临时文件进行排序并写入最终文件
# A文件排序：通过临时文件写入，然后使用自定义的 sort 逻辑。
# 由于A文件是路径和匹配行交替的两行一组，直接 sort 可能会打乱组。
# 更好的方法是使用 sort 的 key 来确保路径行被排序，同时保持下一行的内容跟随。
# 但是对于 A 文件的格式，最简单的做法是：
# a. 将路径和匹配行合并成一行，以一个特殊分隔符（如 `|`）分隔
# b. 对合并后的行排序
# c. 再拆分成两行输出
# 为了简化，我们只对路径行进行排序，并使用一个更稳健的循环：

# 临时 A 文件重新处理：将路径和内容合并为一行，然后排序
# 注意：您的样例是两行，如果严格按照样例，则需要如下处理，但无法使用简单的 sort：
# PuzzlesF\Puzzles\Farmer.cpp
#     iOS Game: 100 Logic Games 2/Puzzle Set 2/Farmer

# 由于 shell 难以对两行一组的块进行排序，我们先将数据改为易于排序的格式：
# PuzzlesF/Puzzles/Farmer.cpp|    iOS Game: 100 Logic Games 2/Puzzle Set 2/Farmer
# 然后再转回两行格式。

if [ -f "$TEMP_FILE_A" ]; then
    # 将临时A文件中的路径和内容合并为一行，以换行符分隔路径和内容
    # 然后进行排序
    
    # 创建一个临时文件来存储合并后的单行数据
    TEMP_A_SINGLE_LINE="${OUTPUT_DIR}/.temp_A_single.txt"
    > "$TEMP_A_SINGLE_LINE"

    # 将两行一组的数据合并为一行（路径|内容），方便排序
    while read -r path && read -r line; do
        echo "${path}|${line}" >> "$TEMP_A_SINGLE_LINE"
    done < "$TEMP_FILE_A"

    # 对合并后的单行数据按路径排序，然后拆分为两行输出到最终文件A
    sort "$TEMP_A_SINGLE_LINE" | while IFS='|' read -r path line; do
        echo "$path" >> "$OUTPUT_FILE_A"
        echo -e "\t$line" >> "$OUTPUT_FILE_A"
    done

    # 清理中间文件
    rm "$TEMP_A_SINGLE_LINE"
fi

# B文件排序：直接对路径列表进行排序
if [ -f "$TEMP_FILE_B" ]; then
    sort "$TEMP_FILE_B" > "$OUTPUT_FILE_B"
fi

# 清理临时文件
rm "$TEMP_FILE_A" "$TEMP_FILE_B"

echo "脚本执行完毕。"
echo "包含 '$SEARCH_STRING' 的文件列表和内容已保存到: $OUTPUT_FILE_A"
echo "不包含 '$SEARCH_STRING' 的文件列表已保存到: $OUTPUT_FILE_B"