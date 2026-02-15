#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
from pathlib import Path

# 配置部分
SEARCH_DIR = "."
SEARCH_STRING = "iOS Game: 100 Logic Games"
VARIANT_PATTERN = re.compile(r'Variant|Variation')  # 搜索 Variant 或 Variation
OUTPUT_DIR = "Archive"
OUTPUT_FILE_A = os.path.join(OUTPUT_DIR, "Files_With_Logic_Game_Tag.txt")
OUTPUT_FILE_B = os.path.join(OUTPUT_DIR, "Files_Without_Logic_Game_Tag.txt")

def find_files():
    """生成器：逐个产生符合条件的文件"""
    search_path = Path(SEARCH_DIR)
    for file_path in search_path.rglob("*.cpp"):
        if "Puzzles" not in str(file_path):
            continue
        if file_path.name in ["main.cpp", "stdafx.cpp"]:
            continue
        yield file_path

def file_contains_string(file_path, search_string):
    """检查文件是否包含目标字符串，并返回匹配行"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                if search_string in line:
                    return line.rstrip()  # 保留原始缩进，只去除行尾空白
    except Exception as e:
        print(f"警告：无法读取文件 {file_path}: {e}")
    return None

def file_contains_variant(file_path):
    """检查文件是否包含变体关键词：Variant 或 Variation"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            return bool(VARIANT_PATTERN.search(content))
    except Exception:
        return False

def check_game_status(txt_file_path):
    """
    检查游戏状态
    返回值：
        - ("Solved", []): 有txt文件且完全解决
        - ("Unsolved", unsolved_levels): 有txt文件但未完全解决，返回未解决的关卡号列表
        - ("No Solution File", []): 没有txt文件
    """
    if not os.path.exists(txt_file_path):
        return "No Solution File", []
    
    try:
        with open(txt_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return "No Solution File", []  # 无法读取文件，视为没有解决方案文件
    
    unsolved_levels = []
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # 检查是否是 "Level " 开头的行
        if line.startswith("Level "):
            # 提取关卡号（"Level "后面的内容）
            level_number = line[6:].strip()  # 去掉"Level "前缀并去除首尾空格
            
            # 检查是否有下一行
            if i + 1 >= len(lines):
                unsolved_levels.append(level_number)  # Level行是最后一行，没有后续行，视为未解决
                i += 1
                continue
            
            # 检查下一行是否以 "Sequence of moves" 开头
            next_line = lines[i + 1].rstrip()
            if not next_line.startswith("Sequence of moves"):
                unsolved_levels.append(level_number)  # 后续行不是"Sequence of moves"，视为未解决
                i += 1  # 只增加1，因为下一行不是有效的Sequence行
            else:
                # 这一对行是有效的，跳过
                i += 2
        else:
            i += 1
    
    if unsolved_levels:
        return "Unsolved", unsolved_levels
    else:
        return "Solved", []

def get_sort_key(file_info):
    """获取排序键值（按文件名排序，大小写不敏感）"""
    if isinstance(file_info, tuple):
        filename = os.path.basename(file_info[0])
        return filename.lower()
    else:
        filename = os.path.basename(file_info)
        return filename.lower()

def main():
    # 创建输出目录
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    files_with_tag = []      # 存储 (相对路径, 匹配行, 是否有变体, 游戏状态, 未解决关卡列表)
    files_without_tag = []   # 存储 相对路径
    
    print("正在处理文件...")
    file_count = 0
    
    for file_path in find_files():
        file_count += 1
        if file_count % 100 == 0:
            print(f"已处理 {file_count} 个文件...")
        
        # 获取相对路径
        try:
            relative_path = os.path.relpath(file_path, SEARCH_DIR)
        except ValueError:
            relative_path = str(file_path)
        
        # 检查是否包含目标字符串
        matching_line = file_contains_string(file_path, SEARCH_STRING)
        
        if matching_line:
            # 包含目标字符串，检查是否有变体
            has_variant = file_contains_variant(file_path)
            
            # 检查同名的txt文件，获取游戏状态
            txt_file_path = file_path.with_suffix('.txt')
            game_status, unsolved_levels = check_game_status(txt_file_path)
            
            files_with_tag.append((relative_path, matching_line, has_variant, game_status, unsolved_levels))
        else:
            # 不包含目标字符串
            files_without_tag.append(relative_path)
    
    print(f"文件扫描完成，共处理 {file_count} 个文件")
    print(f"包含目标字符串: {len(files_with_tag)} 个文件")
    
    # 统计游戏状态
    solved_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "Solved")
    unsolved_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "Unsolved")
    no_solution_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "No Solution File")
    
    print(f"  已解决: {solved_count} 个文件")
    print(f"  未解决: {unsolved_count} 个文件")
    print(f"  无解决方案文件: {no_solution_count} 个文件")
    print(f"不包含目标字符串: {len(files_without_tag)} 个文件")
    
    # 写入输出文件A（包含目标字符串的文件）
    print(f"正在写入 {OUTPUT_FILE_A}...")
    with open(OUTPUT_FILE_A, 'w', encoding='utf-8') as f:
        # 按文件名排序，大小写不敏感
        for item in sorted(files_with_tag, key=get_sort_key):
            path, line, has_variant, game_status, unsolved_levels = item
            f.write(f"{path}\n")
            f.write(f"{line}\n")
            if has_variant:
                f.write(f"        Has Variants\n")
            
            # 根据游戏状态输出不同信息
            if game_status == "Unsolved":
                # 将未解决的关卡号列表转换为字符串，用逗号加空格分隔
                levels_str = ", ".join(unsolved_levels)
                f.write(f"        Unsolved Levels: {levels_str}\n")
            elif game_status == "No Solution File":
                f.write(f"        No Solution File\n")
            # 如果已解决，不输出任何信息
    
    # 写入输出文件B（不包含目标字符串的文件）
    print(f"正在写入 {OUTPUT_FILE_B}...")
    with open(OUTPUT_FILE_B, 'w', encoding='utf-8') as f:
        # 按文件名排序，大小写不敏感
        for path in sorted(files_without_tag, key=get_sort_key):
            f.write(f"{path}\n")
    
    print("脚本执行完毕！")
    print(f"包含 '{SEARCH_STRING}' 的文件列表: {OUTPUT_FILE_A}")
    print(f"不包含 '{SEARCH_STRING}' 的文件列表: {OUTPUT_FILE_B}")

if __name__ == "__main__":
    main()