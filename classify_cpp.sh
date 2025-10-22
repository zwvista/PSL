#!/bin/bash

# 定义搜索的根目录，如果脚本在要搜索的目录执行，可以设置为 '.'
SEARCH_DIR="."

# 定义要查找的字符串
SEARCH_STRING="iOS Game: 100 Logic Games"

# 定义输出文件名
# 包含目标字符串的文件路径及匹配行
OUTPUT_FILE_A="Archive/Files_With_Logic_Game_Tag.txt"
# 不包含目标字符串的文件相对路径
OUTPUT_FILE_B="Archive/Files_Without_Logic_Game_Tag.txt"

# 清空或创建输出文件
> "$OUTPUT_FILE_A"
> "$OUTPUT_FILE_B"

# 使用 find 命令查找文件
# -type f: 只查找文件
# -name "*.cpp": 查找以 .cpp 结尾的文件
# -path "*Puzzles*": 文件路径中包含 "Puzzles" (注意路径匹配时是 *Puzzles*，而不是 \Puzzles\)
# -not -name "main.cpp": 排除文件名 main.cpp
# -not -name "stdafx.cpp": 排除文件名 stdafx.cpp
# -print0: 使用空字符作为分隔符，可以正确处理包含空格或特殊字符的文件名
find "$SEARCH_DIR" -type f \
    -name "*.cpp" \
    -path "*Puzzles*" \
    -not -name "main.cpp" \
    -not -name "stdafx.cpp" \
    -print0 |

# 使用 while 循环处理找到的每个文件
while IFS= read -r -d $'\0' file_path; do
    # 尝试在文件中查找目标字符串
    # -m 1: 只输出一次匹配结果，找到即停止
    # -i: 忽略大小写（如果需要精确匹配，请移除 -i）
    # -E: 使用扩展正则表达式（虽然这里不需要，但这是一个好习惯）
    # -F: 解释PATTERN为固定字符串，而不是正则表达式
    # -H: 即使只有一个文件，也总是打印文件名（虽然 find 已经提供了路径）
    # GREP_RESULT=$(grep -m 1 -F "$SEARCH_STRING" "$file_path")
    # 为了满足A文件的格式要求（相对路径 + 匹配行），我们使用 grep 捕获匹配行
    
    # 相对路径，去除 $SEARCH_DIR/ 前缀
    relative_path="${file_path#$SEARCH_DIR/}"

    # 查找匹配的行
    MATCHING_LINE=$(grep -F -m 1 "$SEARCH_STRING" "$file_path" 2>/dev/null)

    if [ -n "$MATCHING_LINE" ]; then
        # 如果找到匹配的行 (A文件)
        # 将相对路径和匹配行写入 A 文件
        echo "$relative_path" >> "$OUTPUT_FILE_A"
        echo -e "$MATCHING_LINE" >> "$OUTPUT_FILE_A"
    else
        # 如果没有找到匹配的行 (B文件)
        # 将相对路径写入 B 文件
        echo "$relative_path" >> "$OUTPUT_FILE_B"
    fi

done

echo "脚本执行完毕。"
echo "包含 '$SEARCH_STRING' 的文件列表和内容已保存到: $OUTPUT_FILE_A"
echo "不包含 '$SEARCH_STRING' 的文件列表已保存到: $OUTPUT_FILE_B"