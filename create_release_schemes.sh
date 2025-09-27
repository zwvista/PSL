#!/bin/bash

# 使用find的-not -name选项直接排除已包含_Release的文件
find . -name "Puzzles*.xcscheme" -not -name "*_Release.xcscheme" | while read file; do
    # 生成新文件名（在原文件名基础上添加 _Release）
    new_file="${file%.xcscheme}_Release.xcscheme"
    
    echo "处理文件: $file"
    echo "创建副本: $new_file"
    
    # 拷贝原文件到新文件
    cp "$file" "$new_file"
    
    # 在新文件中替换 buildConfiguration 设置
    sed -i '' 's/<LaunchAction[^>]*buildConfiguration = "Debug"/<LaunchAction buildConfiguration = "Release"/g' "$new_file"
    sed -i '' 's/<LaunchAction[^>]*buildConfiguration="Debug"/<LaunchAction buildConfiguration="Release"/g' "$new_file"
        
    echo "已完成: $new_file"
    echo "---"
done

echo "所有文件处理完成！"