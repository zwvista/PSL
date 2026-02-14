#!/bin/bash

# 配置部分 - 便于修改
SEARCH_DIR="."
SEARCH_STRING="iOS Game: 100 Logic Games"
VARIANT_STRINGS="Variant|Variation"  # 使用 | 分隔多个搜索字符串
OUTPUT_DIR="Archive"
OUTPUT_FILE_A="${OUTPUT_DIR}/Files_With_Logic_Game_Tag.txt"
OUTPUT_FILE_B="${OUTPUT_DIR}/Files_Without_Logic_Game_Tag.txt"

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 初始化临时文件
TEMP_DIR=$(mktemp -d)
TEMP_FILE_A="$TEMP_DIR/temp_A.txt"
TEMP_FILE_B="$TEMP_DIR/temp_B.txt"
trap "rm -rf $TEMP_DIR" EXIT  # 确保临时文件被清理

# 查找并处理文件
find "$SEARCH_DIR" -type f \
    -name "*.cpp" \
    -path "*Puzzles*" \
    -not -name "main.cpp" \
    -not -name "stdafx.cpp" \
    -print0 | while IFS= read -r -d $'\0' file_path; do
    
    # 获取相对路径
    relative_path="${file_path#$SEARCH_DIR/}"
    
    # 查找目标字符串匹配行（只取第一行），保持原始内容（包括缩进）
    matching_line=$(grep -F -m 1 "$SEARCH_STRING" "$file_path" 2>/dev/null)
    
    if [ -n "$matching_line" ]; then
        # 包含目标字符串，继续搜索 Variant 或 VARIANT
        
        # 使用 grep -E 支持正则表达式，搜索 Variant 或 VARIANT
        # -q: 静默模式，只检查是否存在，不输出内容
        if grep -q -E "$VARIANT_STRINGS" "$file_path" 2>/dev/null; then
            # 找到 Variant 或 VARIANT - 输出三行
            printf "%s\n%s\n%s\n" "$relative_path" "$matching_line" "Has Variants" >> "$TEMP_FILE_A"
        else
            # 没找到 Variant 或 VARIANT - 只输出两行
            printf "%s\n%s\n" "$relative_path" "$matching_line" >> "$TEMP_FILE_A"
        fi
    else
        # 不包含目标字符串
        echo "$relative_path" >> "$TEMP_FILE_B"
    fi
done

# 处理A文件
if [ -f "$TEMP_FILE_A" ]; then
    > "$OUTPUT_FILE_A"  # 清空输出文件
    
    # 由于文件可能包含不同行数的组（2行或3行），需要特殊处理
    # 先读取所有内容到数组，然后按组处理
    lines=()
    while IFS= read -r line; do
        lines+=("$line")
    done < "$TEMP_FILE_A"
    
    # 创建临时文件用于存储合并后的行，以便排序
    TEMP_A_SORTABLE="$TEMP_DIR/temp_A_sortable.txt"
    > "$TEMP_A_SORTABLE"
    
    # 遍历数组，按组处理
    i=0
    while [ $i -lt ${#lines[@]} ]; do
        path="${lines[$i]}"
        i=$((i + 1))
        content="${lines[$i]}"
        i=$((i + 1))
        
        # 检查是否还有第三行（变体信息）
        if [ $i -lt ${#lines[@]} ] && [ "${lines[$i]}" = "Has Variants" ]; then
            variant="${lines[$i]}"
            i=$((i + 1))
            # 有三行，合并为一行用于排序
            echo "$path|$content|$variant" >> "$TEMP_A_SORTABLE"
        else
            # 只有两行，合并为一行用于排序（第三字段为空）
            echo "$path|$content|" >> "$TEMP_A_SORTABLE"
        fi
    done
    
    # 排序并输出
    sort "$TEMP_A_SORTABLE" | while IFS='|' read -r path content variant; do
        if [ -n "$variant" ]; then
            # 有变体信息，输出三行
            # 第一行：路径（不缩进）
            echo "$path" >> "$OUTPUT_FILE_A"
            # 第二行：匹配行（保持原始缩进，不添加额外制表符）
            echo "$content" >> "$OUTPUT_FILE_A"
            # 第三行：八个空格 + "Has Variants"
            echo "        $variant" >> "$OUTPUT_FILE_A"
        else
            # 没有变体信息，只输出两行
            echo "$path" >> "$OUTPUT_FILE_A"
            echo "$content" >> "$OUTPUT_FILE_A"
        fi
    done
fi

# 处理B文件（简单排序）
[ -f "$TEMP_FILE_B" ] && sort "$TEMP_FILE_B" > "$OUTPUT_FILE_B"

echo "脚本执行完毕。"
echo "包含 '$SEARCH_STRING' 的文件列表: $OUTPUT_FILE_A"
echo "不包含 '$SEARCH_STRING' 的文件列表: $OUTPUT_FILE_B"