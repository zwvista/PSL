#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Puzzle Game Status Analyzer (puzzle_game_analyzer.py)
分析C++游戏文件，提取游戏信息并生成HTML状态报告

最终版本功能总结
1.文件扫描：自动扫描符合条件的C++文件
2.信息提取：解析游戏系列、谜题集、游戏标题
3.状态分析：5种游戏状态（已解决、部分解决、未解决、无有效解决方案、无解决方案文件），并显示超时关卡（>10秒）
4.变体检测：识别包含Variant/Variation的文件，显示为绿色标签"有变体"
5.有效游戏判断：排除以数字结尾或以Gen结尾的文件，支持白名单
6.特殊标题检测：找出游戏名与标题不匹配的情况
7.iOS实现检测：检查对应Swift目录是否存在
8.Android实现检测：检查对应XML文件是否存在
9.Automator实现检测：检查对应自动截图目录是否存在
10.脚本生成：为iOS和Android项目生成拷贝XML的shell脚本
11.HTML报告：美观的表格展示，包含详细的统计信息

生成的HTML报告包含
1.总体统计：文件总数、包含标签数、不包含标签数、有效游戏总数
2.第一部分表格：包含标签的文件，11列详细信息
3.第二部分表格：不包含标签的文件，5列基本信息（不含Automator列）
4.特殊标题表格：列出所有游戏名与标题不匹配的情况（不含平台列）
5.白名单列表：显示每部分中找到的白名单游戏

生成的脚本
1.iOS版本：在../LogicPuzzlesSwift目录生成copy_puzzle_xml.sh，拷贝XML到Puzzles子目录
2.Android版本：在../LogicPuzzlesAndroid目录生成copy_puzzle_xml.sh，拷贝XML到assets/xml目录
"""

import os
import re
from pathlib import Path

# 配置部分
SEARCH_DIR = "."
SEARCH_STRING = "iOS Game: 100 Logic Games"
VARIANT_PATTERN = re.compile(r'Variant|Variation')  # 搜索 Variant 或 Variation
OUTPUT_DIR = "Archive"
OUTPUT_HTML = os.path.join(OUTPUT_DIR, "Puzzle_Status_Report.html")

# Swift Puzzles 目录
SWIFT_PUZZLES_DIR = "../LogicPuzzlesSwift/LogicPuzzlesSwift/Puzzles"

# Android XML 文件路径
ANDROID_XML_PATH = "../LogicPuzzlesAndroid/app/src/main/assets/xml/{}.xml"

# Automator 自动截图目录
AUTOMATOR_PUZZLES_DIR = "../LogicPuzzlesAutomator/Puzzles"

# 白名单：虽然是数字结尾，但视为有效游戏
WHITELIST_NAMES = {"Square100", "rotate9"}

def find_files():
    """生成器：逐个产生符合条件的文件"""
    search_path = Path(SEARCH_DIR)
    for file_path in search_path.rglob("*.cpp"):
        if "Puzzles" not in str(file_path):
            continue
        if file_path.name in ["main.cpp", "stdafx.cpp"]:
            continue
        yield file_path

def is_valid_game_name(filename):
    """
    判断是否为有效游戏名
    有效游戏名：不以数字结尾，不以Gen结尾（大小写不敏感）
    但有白名单例外：Square100 和 rotate9 视为有效游戏
    """
    name_without_ext = os.path.splitext(filename)[0]
    
    # 检查白名单
    if name_without_ext in WHITELIST_NAMES:
        return True
    
    # 检查是否以Gen结尾（不区分大小写）
    if name_without_ext.lower().endswith('gen'):
        return False
    
    # 检查是否以数字结尾
    if re.search(r'\d+$', name_without_ext):
        return False
    
    return True

def get_game_valid_status(filename):
    """
    获取游戏有效状态标识
    返回: 对应的HTML标记
    """
    name_without_ext = os.path.splitext(filename)[0]
    
    # 检查白名单
    if name_without_ext in WHITELIST_NAMES:
        return '<span class="whitelist-mark">⭕️</span>'
    
    # 检查是否以Gen结尾（不区分大小写）
    if name_without_ext.lower().endswith('gen'):
        return '<span class="invalid-mark">❌</span>'
    
    # 检查是否以数字结尾
    if re.search(r'\d+$', name_without_ext):
        return '<span class="invalid-mark">❌</span>'
    
    return ''

def check_swift_implementation(puzzle_name):
    """
    检查是否存在Swift实现（检查目录是否存在）
    返回: (bool, HTML标记)
    """
    # 只对有效游戏名进行检查
    if not is_valid_game_name(puzzle_name + ".cpp"):
        return False, ''
    
    # 构建Swift目录路径
    swift_dir = os.path.join(SWIFT_PUZZLES_DIR, puzzle_name)
    
    # 检查目录是否存在
    if os.path.exists(swift_dir) and os.path.isdir(swift_dir):
        return True, '<span class="swift-yes">✓</span>'
    else:
        return False, '<span class="swift-no">❌</span>'

def check_android_implementation(puzzle_name):
    """
    检查是否存在Android实现（检查XML文件是否存在）
    返回: (bool, HTML标记)
    """
    # 只对有效游戏名进行检查
    if not is_valid_game_name(puzzle_name + ".cpp"):
        return False, ''
    
    # 构建Android XML文件路径
    android_xml_file = ANDROID_XML_PATH.format(puzzle_name)
    
    # 检查XML文件是否存在
    if os.path.exists(android_xml_file) and os.path.isfile(android_xml_file):
        return True, '<span class="android-yes">✓</span>'
    else:
        return False, '<span class="android-no">❌</span>'

def check_automator_implementation(puzzle_name):
    """
    检查是否存在Automator自动截图实现（检查目录是否存在）
    返回: (bool, HTML标记)
    """
    # 只对有效游戏名进行检查
    if not is_valid_game_name(puzzle_name + ".cpp"):
        return False, ''
    
    # 构建Automator目录路径
    automator_dir = os.path.join(AUTOMATOR_PUZZLES_DIR, puzzle_name)
    
    # 检查目录是否存在
    if os.path.exists(automator_dir) and os.path.isdir(automator_dir):
        return True, '<span class="automator-yes">✓</span>'
    else:
        return False, '<span class="automator-no">❌</span>'

def format_game_name(name):
    """
    在每个大写字母前插入空格（首字母除外）
    例如: "BattleShips" -> "Battle Ships"
         "ADifferentFarmer" -> "A Different Farmer"
         "Square100" -> "Square100"（数字前不插入空格）
    """
    if not name:
        return name
    
    result = []
    for i, char in enumerate(name):
        # 如果不是第一个字符，且当前字符是大写字母
        if i > 0 and char.isupper():
            result.append(' ')
        result.append(char)
    
    return ''.join(result)

def file_contains_string(file_path, search_string):
    """检查文件是否包含目标字符串，并返回匹配行（去除前导空格）"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                if search_string in line:
                    return line.strip()  # 去除前导和尾随空格
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

def parse_game_info(line):
    """
    解析游戏信息行
    返回: (game_set, puzzle_set, game_title)
    如果无法解析，相应字段返回 None
    """
    if not line:
        return None, None, None
    
    # 移除 "iOS Game: 100 Logic Games" 前缀
    prefix = "iOS Game: 100 Logic Games"
    if not line.startswith(prefix):
        return None, None, None
    
    remaining = line[len(prefix):].strip()
    
    # 解析 Game Set
    game_set = "1"  # 默认为1
    if remaining.startswith((" ", "/")):
        remaining = remaining.lstrip(" /")
    
    # 检查是否有 Game Set 编号（数字）
    match = re.match(r'^(\d+)', remaining)
    if match:
        game_set = match.group(1)
        remaining = remaining[len(game_set):].strip(" /")
    else:
        # 没有数字，默认为1，但remaining保持不变
        pass
    
    # 解析 Puzzle Set
    if remaining.startswith("Puzzle Set"):
        remaining = remaining[10:].strip()  # 移除 "Puzzle Set"
        puzzle_match = re.match(r'^(\d+)', remaining)
        if puzzle_match:
            puzzle_set = puzzle_match.group(1)
            remaining = remaining[len(puzzle_set):].strip(" /")
        else:
            puzzle_set = None
    else:
        puzzle_set = None
    
    # 剩余部分就是 Game Title
    game_title = remaining if remaining else None
    
    return game_set, puzzle_set, game_title

def check_numbers_continuous(numbers):
    """检查数字列表是否连续（从小到大排序后）"""
    if not numbers:
        return True
    
    sorted_numbers = sorted(numbers)
    for i in range(len(sorted_numbers) - 1):
        if sorted_numbers[i + 1] != sorted_numbers[i] + 1:
            return False
    return True

def check_timeout_levels(txt_file_path, timeout_seconds=10):
    """
    检查解决方案文件中解决时间超过指定秒数的关卡
    返回: 超时关卡号列表
    """
    timeout_levels = []
    
    if not os.path.exists(txt_file_path):
        return timeout_levels
    
    try:
        with open(txt_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return timeout_levels
    
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # 检查是否是 "Level " 开头的行
        if line.startswith("Level "):
            # 提取关卡号
            level_str = line[6:].strip()
            
            # 检查是否有下一行
            if i + 1 >= len(lines):
                i += 1
                continue
            
            # 检查下一行是否以 "Sequence of moves" 开头
            next_line = lines[i + 1].rstrip()
            if next_line.startswith("Sequence of moves"):
                # 这一关已解决，继续查找时间信息
                # 时间信息通常在关卡的最后一行
                j = i + 2
                time_value = None
                
                # 查找时间行（格式如 "0.000317 [s]"）
                while j < len(lines):
                    time_line = lines[j].rstrip()
                    # 检查是否是时间行（以数字开头，以[s]结尾）
                    if time_line and re.match(r'^[\d\.]+', time_line) and time_line.endswith('[s]'):
                        try:
                            # 提取时间数值
                            time_match = re.match(r'^([\d\.]+)', time_line)
                            if time_match:
                                time_value = float(time_match.group(1))
                                break
                        except ValueError:
                            pass
                    j += 1
                
                # 检查是否超时
                if time_value is not None and time_value > timeout_seconds:
                    try:
                        level_num = int(level_str)
                        timeout_levels.append(level_num)
                    except ValueError:
                        pass
                
                i = j + 1 if j < len(lines) else len(lines)
            else:
                i += 1
        else:
            i += 1
    
    return timeout_levels

def check_game_status(txt_file_path):
    """
    检查游戏状态
    返回值：
        - ("Solved", [], max_level, timeout_levels): 有txt文件且完全解决，所有关卡连续
        - ("Partly Solved", missing_levels, max_level, timeout_levels): 有txt文件且所有关卡都有解决方案，但关卡号不连续（返回缺失的关卡号）
        - ("Unsolved", unsolved_levels, max_level, timeout_levels): 有txt文件但有关卡未解决
        - ("No Solutions", [], max_level, []): 有txt文件但没有可识别的关卡号（无法转换为整数）
        - ("No Solution File", [], None, []): 没有txt文件
    """
    if not os.path.exists(txt_file_path):
        return "No Solution File", [], None, []
    
    try:
        with open(txt_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return "No Solution File", [], None, []  # 无法读取文件，视为没有解决方案文件
    
    solved_levels = []      # 已解决的关卡号（整数）
    unsolved_levels = []    # 未解决的关卡号（原始字符串）
    timeout_levels = []     # 超时的关卡号
    has_any_level = False   # 是否找到任何Level行
    max_level = None        # 最大关卡号
    
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # 检查是否是 "Level " 开头的行
        if line.startswith("Level "):
            has_any_level = True
            # 提取关卡号（"Level "后面的内容）
            level_str = line[6:].strip()
            
            # 尝试转换为整数，用于计算最大值和连续性检查
            try:
                level_num = int(level_str)
                if max_level is None or level_num > max_level:
                    max_level = level_num
            except ValueError:
                level_num = None
            
            # 检查是否有下一行
            if i + 1 >= len(lines):
                unsolved_levels.append(level_str)  # Level行是最后一行，没有后续行，视为未解决
                i += 1
                continue
            
            # 检查下一行是否以 "Sequence of moves" 开头
            next_line = lines[i + 1].rstrip()
            if not next_line.startswith("Sequence of moves"):
                unsolved_levels.append(level_str)  # 后续行不是"Sequence of moves"，视为未解决
                i += 1  # 只增加1，因为下一行不是有效的Sequence行
            else:
                # 这一对行是有效的，视为已解决
                if level_num is not None:
                    solved_levels.append(level_num)
                    
                    # 查找时间信息，检查是否超时
                    j = i + 2
                    time_value = None
                    
                    # 查找时间行（格式如 "0.000317 [s]"）
                    while j < len(lines):
                        time_line = lines[j].rstrip()
                        # 检查是否是时间行（以数字开头，以[s]结尾）
                        if time_line and re.match(r'^[\d\.]+', time_line) and time_line.endswith('[s]'):
                            try:
                                time_match = re.match(r'^([\d\.]+)', time_line)
                                if time_match:
                                    time_value = float(time_match.group(1))
                                    if time_value > 10.0:  # 超过10秒视为超时
                                        timeout_levels.append(level_num)
                                    break
                            except ValueError:
                                pass
                        j += 1
                
                i += 2
        else:
            i += 1
    
    # 如果没有找到任何Level行，视为No Solutions
    if not has_any_level:
        return "No Solutions", [], max_level, []
    
    # 判断游戏状态
    if unsolved_levels:
        return "Unsolved", unsolved_levels, max_level, timeout_levels
    elif not solved_levels:
        # 有Level行，但所有Level号都无法转换为整数
        return "No Solutions", [], max_level, []
    elif not check_numbers_continuous(solved_levels):
        # 所有关卡都有解决方案，但关卡号不连续
        # 找出缺失的关卡号
        missing_levels = []
        sorted_solved = sorted(solved_levels)
        expected_level = 1
        for level in sorted_solved:
            while expected_level < level:
                missing_levels.append(str(expected_level))
                expected_level += 1
            expected_level = level + 1
        
        # 检查最大关卡号之后的缺失
        if max_level:
            while expected_level <= max_level:
                missing_levels.append(str(expected_level))
                expected_level += 1
        
        return "Partly Solved", missing_levels, max_level, timeout_levels
    else:
        return "Solved", [], max_level, timeout_levels

def get_group_name(file_path):
    """从文件路径中提取组名（第一个斜杠之前的内容）"""
    # 使用正斜杠分割路径
    parts = file_path.split('/')
    if parts:
        return parts[0]
    return file_path

def generate_swift_copy_script(files_with_tag):
    """
    生成Swift版本的拷贝XML文件的shell脚本
    对于所有iOS实现存在的游戏，生成拷贝命令
    根据cpp文件的路径构建对应的XML源文件路径
    目标: LogicPuzzlesSwift/Puzzles/游戏名/游戏名.xml
    """
    script_lines = [
        "#!/bin/bash",
        "# 自动生成的Swift拷贝XML脚本",
        "# 生成时间: " + __import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        "",
        "echo \"开始拷贝XML文件到Swift项目...\"",
        ""
    ]
    
    copy_count = 0
    
    for item in files_with_tag:
        path = item[0]  # 相对路径，例如: "PuzzlesA/Puzzles/AbstractPainting.cpp"
        filename = os.path.basename(path)
        puzzle_name = os.path.splitext(filename)[0]
        
        # 只处理有效游戏名
        if not is_valid_game_name(filename):
            continue
        
        # 检查iOS实现是否存在
        swift_exists, _ = check_swift_implementation(puzzle_name)
        if not swift_exists:
            continue
        
        # 从cpp路径构建XML源文件路径
        # 例如: "PuzzlesA/Puzzles/AbstractPainting.cpp" 
        # 转换为: "../PSL/PuzzlesA/Puzzles/AbstractPainting.xml"
        
        # 获取目录部分
        dir_path = os.path.dirname(path)  # 例如: "PuzzlesA/Puzzles"
        
        # 构建源文件路径: ../PSL/ + dir_path + / + puzzle_name + .xml
        source_file = f"../PSL/{dir_path}/{puzzle_name}.xml"
        
        # 目标文件: LogicPuzzlesSwift/Puzzles/游戏名/游戏名.xml
        target_dir = f"LogicPuzzlesSwift/Puzzles/{puzzle_name}"
        target_file = f"{target_dir}/{puzzle_name}.xml"
        
        # 添加拷贝命令
        script_lines.append(f"# 拷贝 {puzzle_name} (来自: {dir_path})")
        script_lines.append(f"mkdir -p {target_dir}")
        script_lines.append(f"if [ -f \"{source_file}\" ]; then")
        script_lines.append(f"    cp \"{source_file}\" \"{target_file}\"")
        script_lines.append(f"    echo \"✓ 已拷贝到Swift: {puzzle_name}\"")
        script_lines.append(f"else")
        script_lines.append(f"    echo \"❌ 源文件不存在: {source_file}\"")
        script_lines.append(f"fi")
        script_lines.append("")
        
        copy_count += 1
    
    script_lines.append(f"echo \"\\nSwift拷贝完成，共处理 {copy_count} 个游戏\"")
    script_lines.append("")
    
    return "\n".join(script_lines), copy_count

def generate_android_copy_script(files_with_tag):
    """
    生成Android版本的拷贝XML文件的shell脚本
    对于所有Android实现存在的游戏，生成拷贝命令
    根据cpp文件的路径构建对应的XML源文件路径
    目标: app/src/main/assets/xml/游戏名.xml
    """
    script_lines = [
        "#!/bin/bash",
        "# 自动生成的Android拷贝XML脚本",
        "# 生成时间: " + __import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        "",
        "echo \"开始拷贝XML文件到Android项目...\"",
        ""
    ]
    
    copy_count = 0
    
    for item in files_with_tag:
        path = item[0]  # 相对路径，例如: "PuzzlesA/Puzzles/AbstractPainting.cpp"
        filename = os.path.basename(path)
        puzzle_name = os.path.splitext(filename)[0]
        
        # 只处理有效游戏名
        if not is_valid_game_name(filename):
            continue
        
        # 检查Android实现是否存在
        android_exists, _ = check_android_implementation(puzzle_name)
        if not android_exists:
            continue
        
        # 从cpp路径构建XML源文件路径
        # 例如: "PuzzlesA/Puzzles/AbstractPainting.cpp" 
        # 转换为: "../PSL/PuzzlesA/Puzzles/AbstractPainting.xml"
        
        # 获取目录部分
        dir_path = os.path.dirname(path)  # 例如: "PuzzlesA/Puzzles"
        
        # 构建源文件路径: ../PSL/ + dir_path + / + puzzle_name + .xml
        source_file = f"../PSL/{dir_path}/{puzzle_name}.xml"
        
        # 目标文件: app/src/main/assets/xml/游戏名.xml
        target_dir = "app/src/main/assets/xml"
        target_file = f"{target_dir}/{puzzle_name}.xml"
        
        # 添加拷贝命令
        script_lines.append(f"# 拷贝 {puzzle_name} (来自: {dir_path})")
        script_lines.append(f"mkdir -p {target_dir}")
        script_lines.append(f"if [ -f \"{source_file}\" ]; then")
        script_lines.append(f"    cp \"{source_file}\" \"{target_file}\"")
        script_lines.append(f"    echo \"✓ 已拷贝到Android: {puzzle_name}\"")
        script_lines.append(f"else")
        script_lines.append(f"    echo \"❌ 源文件不存在: {source_file}\"")
        script_lines.append(f"fi")
        script_lines.append("")
        
        copy_count += 1
    
    script_lines.append(f"echo \"\\nAndroid拷贝完成，共处理 {copy_count} 个游戏\"")
    script_lines.append("")
    
    return "\n".join(script_lines), copy_count

def generate_html(files_with_tag, files_without_tag, valid_games_count):
    """生成HTML报告"""
    
    # 状态对应的CSS类
    status_classes = {
        "Solved": "status-solved",
        "Partly Solved": "status-partly",
        "Unsolved": "status-unsolved",
        "No Solutions": "status-no-solutions",
        "No Solution File": "status-no-file"
    }
    
    # 状态显示文本
    status_display = {
        "Solved": "✓ 已解决",
        "Partly Solved": "⚠️ 部分解决",
        "Unsolved": "❌ 未解决",
        "No Solutions": "📄 无有效解决方案",
        "No Solution File": "❓ 无解决方案文件"
    }
    
    html = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Puzzle Game Status Report</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
            color: #333;
        }}
        h1 {{
            color: #2c3e50;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
            margin-top: 30px;
        }}
        h1:first-of-type {{
            margin-top: 0;
        }}
        h2 {{
            color: #34495e;
            margin-top: 30px;
            margin-bottom: 15px;
        }}
        h3 {{
            color: #2c3e50;
            margin-top: 25px;
            margin-bottom: 10px;
        }}
        .summary {{
            background-color: white;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }}
        .summary-item {{
            display: inline-block;
            margin-right: 30px;
            padding: 8px 15px;
            background-color: #ecf0f1;
            border-radius: 20px;
            font-weight: bold;
        }}
        .valid-games {{
            background-color: #27ae60;
            color: white;
        }}
        .special-games {{
            background-color: #f39c12;
            color: white;
        }}
        .whitelist-games {{
            background-color: #8e44ad;
            color: white;
        }}
        .status-summary {{
            background-color: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            border-left: 5px solid #3498db;
        }}
        .status-summary h3 {{
            margin-top: 0;
            color: #2c3e50;
            font-size: 1.1em;
        }}
        .status-summary .stats {{
            display: flex;
            flex-wrap: wrap;
            gap: 15px;
        }}
        .status-summary .stat-item {{
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.95em;
        }}
        .whitelist-list {{
            margin-top: 10px;
            padding: 10px;
            background-color: #f3e5f5;
            border-radius: 8px;
            font-size: 0.95em;
        }}
        .whitelist-item {{
            display: inline-block;
            margin-right: 10px;
            padding: 2px 8px;
            background-color: #8e44ad;
            color: white;
            border-radius: 12px;
            font-size: 0.9em;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            background-color: white;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            border-radius: 8px;
            overflow: hidden;
            margin-bottom: 30px;
        }}
        th {{
            background-color: #3498db;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: 600;
        }}
        td {{
            padding: 10px 12px;
            border-bottom: 1px solid #e0e0e0;
        }}
        tr:hover {{
            background-color: #f8f9fa;
        }}
        .variant-badge {{
            display: inline-block;
            padding: 4px 10px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: 500;
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }}
        .swift-yes {{
            color: #27ae60;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .swift-no {{
            color: #e74c3c;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .android-yes {{
            color: #27ae60;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .android-no {{
            color: #e74c3c;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .automator-yes {{
            color: #27ae60;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .automator-no {{
            color: #e74c3c;
            font-size: 1.2em;
            font-weight: bold;
            text-align: center;
        }}
        .whitelist-mark {{
            color: #27ae60;
            font-size: 1.2em;
            text-align: center;
        }}
        .invalid-mark {{
            color: #e74c3c;
            font-size: 1.2em;
            text-align: center;
        }}
        .error-x {{
            color: #e74c3c;
            font-size: 1.2em;
            font-weight: bold;
        }}
        .status-badge {{
            display: inline-block;
            padding: 4px 10px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: 500;
        }}
        .status-solved {{
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }}
        .status-partly {{
            background-color: #fff3cd;
            color: #856404;
            border: 1px solid #ffeeba;
        }}
        .status-unsolved {{
            background-color: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }}
        .status-no-solutions {{
            background-color: #e2e3e5;
            color: #383d41;
            border: 1px solid #d6d8db;
        }}
        .status-no-file {{
            background-color: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }}
        .group-name {{
            font-weight: 600;
            color: #2c3e50;
        }}
        .puzzle-info {{
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }}
        .numeric-cell {{
            text-align: center;
            font-family: 'Courier New', monospace;
        }}
        .name-cell {{
            font-weight: 600;
        }}
        .title-cell {{
            font-style: italic;
        }}
        .formatted-cell {{
            color: #7f8c8d;
        }}
        .valid-status-cell {{
            text-align: center;
            font-size: 1.1em;
        }}
        .special-row {{
            background-color: #fff9e6;
        }}
        .special-row:hover {{
            background-color: #fff2d9;
        }}
        .footer {{
            text-align: right;
            color: #7f8c8d;
            font-size: 0.9em;
            margin-top: 20px;
            padding-top: 10px;
            border-top: 1px solid #e0e0e0;
        }}
    </style>
</head>
<body>
    <h1>Puzzle Game Status Report</h1>
    
    <div class="summary">
        <span class="summary-item">📊 总计: {len(files_with_tag) + len(files_without_tag)} 个文件</span>
        <span class="summary-item">✅ 包含标签: {len(files_with_tag)} 个</span>
        <span class="summary-item">❌ 不包含标签: {len(files_without_tag)} 个</span>
        <span class="summary-item valid-games">🎮 有效游戏总数: {valid_games_count} 个</span>
    </div>
"""
    
    # 第一部分：包含标签的文件
    html += f"""
    <h2>📋 Files With Logic Game Tag</h2>
"""
    
    # 统计信息（第一部分）- 修复：现在有7个元素
    solved_count = sum(1 for _, _, _, status, _, _, _ in files_with_tag if status == "Solved")
    partly_solved_count = sum(1 for _, _, _, status, _, _, _ in files_with_tag if status == "Partly Solved")
    unsolved_count = sum(1 for _, _, _, status, _, _, _ in files_with_tag if status == "Unsolved")
    no_solutions_count = sum(1 for _, _, _, status, _, _, _ in files_with_tag if status == "No Solutions")
    no_solution_file_count = sum(1 for _, _, _, status, _, _, _ in files_with_tag if status == "No Solution File")
    
    # 计算第一部分中的有效游戏数量和白名单游戏
    valid_in_tag = 0
    whitelist_in_tag = []  # 存储白名单游戏名
    special_titles = []  # 存储特殊游戏标题的信息 (name, title, formatted_name)
    
    for item in files_with_tag:
        path = item[0]
        line = item[1]
        filename = os.path.basename(path)
        
        # 检查白名单
        name_without_ext = os.path.splitext(filename)[0]
        if name_without_ext in WHITELIST_NAMES:
            whitelist_in_tag.append(name_without_ext)
        
        if is_valid_game_name(filename):
            valid_in_tag += 1
            
            # 解析游戏信息获取Game Title
            _, _, game_title = parse_game_info(line)
            
            if game_title:
                puzzle_name = os.path.splitext(filename)[0]
                formatted_name = format_game_name(puzzle_name)
                
                # 判断是否为特殊标题：只有当格式化后的Name与Game Title完全相同时才不是特殊标题
                if formatted_name != game_title:
                    special_titles.append((puzzle_name, game_title, formatted_name))
    
    # 计算iOS、Android和Automator实现统计
    swift_exists_count = 0
    swift_missing_count = 0
    android_exists_count = 0
    android_missing_count = 0
    automator_exists_count = 0
    automator_missing_count = 0
    
    for item in files_with_tag:
        path = item[0]
        filename = os.path.basename(path)
        puzzle_name = os.path.splitext(filename)[0]
        
        # 只对有效游戏名进行统计
        if is_valid_game_name(filename):
            swift_exists, _ = check_swift_implementation(puzzle_name)
            if swift_exists:
                swift_exists_count += 1
            else:
                swift_missing_count += 1
            
            android_exists, _ = check_android_implementation(puzzle_name)
            if android_exists:
                android_exists_count += 1
            else:
                android_missing_count += 1
            
            automator_exists, _ = check_automator_implementation(puzzle_name)
            if automator_exists:
                automator_exists_count += 1
            else:
                automator_missing_count += 1
    
    html += f"""    <div class="status-summary">
        <h3>📊 状态统计</h3>
        <div class="stats">
            <span class="stat-item status-solved">✅ 已解决: {solved_count}</span>
            <span class="stat-item status-partly">⚠️ 部分解决: {partly_solved_count}</span>
            <span class="stat-item status-unsolved">❌ 未解决: {unsolved_count}</span>
            <span class="stat-item status-no-solutions">📄 无有效解决方案: {no_solutions_count}</span>
            <span class="stat-item status-no-file">❓ 无解决方案文件: {no_solution_file_count}</span>
            <span class="stat-item valid-games">🎮 有效游戏: {valid_in_tag}</span>
            <span class="stat-item special-games">⭐ 特殊标题: {len(special_titles)}</span>
        </div>
        <div class="stats" style="margin-top: 10px;">
            <span class="stat-item swift-yes">🍎 iOS实现: {swift_exists_count}</span>
            <span class="stat-item swift-no">🍎 iOS缺失: {swift_missing_count}</span>
            <span class="stat-item android-yes">🤖 Android实现: {android_exists_count}</span>
            <span class="stat-item android-no">🤖 Android缺失: {android_missing_count}</span>
            <span class="stat-item automator-yes">⚡️ Automator: {automator_exists_count}</span>
            <span class="stat-item automator-no">⚡️ Automator缺失: {automator_missing_count}</span>
        </div>
"""
    
    # 添加白名单游戏列表
    if whitelist_in_tag:
        whitelist_in_tag.sort()
        whitelist_items = ''.join([f'<span class="whitelist-item">{name}</span>' for name in whitelist_in_tag])
        html += f"""        <div class="whitelist-list">
            <strong>⭕️ 白名单游戏:</strong> {whitelist_items}
        </div>
"""
    
    html += """    </div>
    
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>Game Set</th>
                <th>Puzzle Set</th>
                <th>Game Title</th>
                <th>有效状态</th>
                <th>关卡数</th>
                <th>变体</th>
                <th>iOS</th>
                <th>Android</th>
                <th>Automator</th>
                <th>状态</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for item in sorted(files_with_tag, key=lambda x: os.path.basename(x[0]).lower()):
        path, line, has_variant, game_status, unsolved_levels, max_level, timeout_levels = item
        
        # 注意：这里需要更新files_with_tag的数据结构，增加timeout_levels字段
        # 在main函数中也需要相应修改
        
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        
        # 解析游戏信息
        game_set, puzzle_set, game_title = parse_game_info(line)
        
        # 获取游戏有效状态标识
        filename = os.path.basename(path)
        valid_status_cell = get_game_valid_status(filename)
        
        # 变体列 - 类似状态列的样式
        if has_variant:
            variant_cell = '<span class="variant-badge">✓ 有变体</span>'
        else:
            variant_cell = ''
        
        # iOS实现列
        _, swift_cell = check_swift_implementation(puzzle_name)
        
        # Android实现列
        _, android_cell = check_android_implementation(puzzle_name)
        
        # Automator实现列
        _, automator_cell = check_automator_implementation(puzzle_name)
        
        # 状态列
        status_text = status_display.get(game_status, game_status)
        status_class = status_classes.get(game_status, "")
        
        # 对于Unsolved状态，添加未解决的关卡号
        if game_status == "Unsolved" and unsolved_levels:
            levels_str = ", ".join(unsolved_levels)
            status_text = f"❌ 未解决 ({levels_str})"
            # 如果有超时关卡，也添加
            if timeout_levels:
                timeout_str = ", ".join(map(str, timeout_levels))
                status_text += f"<br><span style='font-size: 0.85em;'>⏱️ 超时: {timeout_str}</span>"
        
        # 对于Partly Solved状态，添加缺失的关卡号
        elif game_status == "Partly Solved" and unsolved_levels:
            levels_str = ", ".join(unsolved_levels)
            status_text = f"⚠️ 部分解决 (缺失: {levels_str})"
            # 如果有超时关卡，也添加
            if timeout_levels:
                timeout_str = ", ".join(map(str, timeout_levels))
                status_text += f"<br><span style='font-size: 0.85em;'>⏱️ 超时: {timeout_str}</span>"
        
        # 对于Solved状态，如果有超时关卡，显示超时信息
        elif game_status == "Solved" and timeout_levels:
            timeout_str = ", ".join(map(str, timeout_levels))
            status_text = f"✓ 已解决<br><span style='font-size: 0.85em;'>⏱️ 超时: {timeout_str}</span>"
        
        # 关卡数列
        if max_level is not None:
            level_cell = f'<span class="numeric-cell">{max_level}</span>'
        else:
            level_cell = '<span class="error-x">❌</span>'
        
        # 处理无法解析的字段
        game_set_cell = game_set if game_set else '<span class="error-x">❌</span>'
        puzzle_set_cell = puzzle_set if puzzle_set else '<span class="error-x">❌</span>'
        game_title_cell = game_title if game_title else '<span class="error-x">❌</span>'
        
        html += f"""            <tr>
                <td class="name-cell"><strong>{puzzle_name}</strong></td>
                <td class="numeric-cell">{game_set_cell}</td>
                <td class="numeric-cell">{puzzle_set_cell}</td>
                <td class="title-cell">{game_title_cell}</td>
                <td class="valid-status-cell">{valid_status_cell}</td>
                <td class="numeric-cell">{level_cell}</td>
                <td class="variant-cell">{variant_cell}</td>
                <td class="swift-cell">{swift_cell}</td>
                <td class="android-cell">{android_cell}</td>
                <td class="automator-cell">{automator_cell}</td>
                <td><span class="status-badge {status_class}">{status_text}</span></td>
            </tr>
"""
    
    html += """        </tbody>
    </table>
"""
    
    # 特殊游戏标题表格 - 只保留三列
    if special_titles:
        html += f"""
    <h3>⭐ 特殊游戏标题 (格式化后的Name ≠ Game Title)</h3>
    <table>
        <thead>
            <tr>
                <th>游戏名 (Name)</th>
                <th>游戏标题 (Title)</th>
                <th>格式化后的Name</th>
            </tr>
        </thead>
        <tbody>
"""
        
        # 按游戏名排序
        for name, title, formatted_name in sorted(special_titles, key=lambda x: x[0].lower()):
            html += f"""            <tr class="special-row">
                <td class="name-cell"><strong>{name}</strong></td>
                <td class="title-cell">{title}</td>
                <td class="formatted-cell">{formatted_name}</td>
            </tr>
"""
        
        html += """        </tbody>
    </table>
"""
    else:
        html += """
    <div class="status-summary">
        <p>✨ 没有发现特殊游戏标题，所有格式化后的Name都与Game Title匹配。</p>
    </div>
"""
    
    # 第二部分：不包含标签的文件
    html += f"""
    <h2>📋 Files Without Logic Game Tag</h2>
"""
    
    # 统计信息（第二部分）
    valid_in_without = 0
    whitelist_in_without = []  # 存储白名单游戏名
    swift_exists_in_without = 0
    swift_missing_in_without = 0
    android_exists_in_without = 0
    android_missing_in_without = 0
    
    for path in files_without_tag:
        filename = os.path.basename(path)
        if is_valid_game_name(filename):
            valid_in_without += 1
            puzzle_name = os.path.splitext(filename)[0]
            swift_exists, _ = check_swift_implementation(puzzle_name)
            if swift_exists:
                swift_exists_in_without += 1
            else:
                swift_missing_in_without += 1
            
            android_exists, _ = check_android_implementation(puzzle_name)
            if android_exists:
                android_exists_in_without += 1
            else:
                android_missing_in_without += 1
        
        # 检查白名单
        name_without_ext = os.path.splitext(filename)[0]
        if name_without_ext in WHITELIST_NAMES:
            whitelist_in_without.append(name_without_ext)
    
    html += f"""    <div class="status-summary">
        <h3>📊 统计信息</h3>
        <div class="stats">
            <span class="stat-item">📄 文件总数: {len(files_without_tag)}</span>
            <span class="stat-item valid-games">🎮 有效游戏: {valid_in_without}</span>
        </div>
        <div class="stats" style="margin-top: 10px;">
            <span class="stat-item swift-yes">🍎 iOS实现: {swift_exists_in_without}</span>
            <span class="stat-item swift-no">🍎 iOS缺失: {swift_missing_in_without}</span>
            <span class="stat-item android-yes">🤖 Android实现: {android_exists_in_without}</span>
            <span class="stat-item android-no">🤖 Android缺失: {android_missing_in_without}</span>
        </div>
"""
    
    # 添加白名单游戏列表
    if whitelist_in_without:
        whitelist_in_without.sort()
        whitelist_items = ''.join([f'<span class="whitelist-item">{name}</span>' for name in whitelist_in_without])
        html += f"""        <div class="whitelist-list">
            <strong>⭕️ 白名单游戏:</strong> {whitelist_items}
        </div>
"""
    
    html += """    </div>
    
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>组名</th>
                <th>有效状态</th>
                <th>iOS</th>
                <th>Android</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for path in sorted(files_without_tag, key=lambda x: os.path.basename(x).lower()):
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        group_name = get_group_name(path)
        filename = os.path.basename(path)
        valid_status_cell = get_game_valid_status(filename)
        _, swift_cell = check_swift_implementation(puzzle_name)
        _, android_cell = check_android_implementation(puzzle_name)
        
        html += f"""            <tr>
                <td><strong>{puzzle_name}</strong></td>
                <td class="group-name">{group_name}</td>
                <td class="valid-status-cell">{valid_status_cell}</td>
                <td class="swift-cell">{swift_cell}</td>
                <td class="android-cell">{android_cell}</td>
            </tr>
"""
    
    html += f"""        </tbody>
    </table>
    
    <div class="footer">
        生成时间: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
    </div>
</body>
</html>
"""
    
    return html

def main():
    """主函数"""
    # 创建输出目录
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    files_with_tag = []      # 存储 (相对路径, 匹配行, 是否有变体, 游戏状态, 未解决/缺失关卡列表, 最大关卡数, 超时关卡列表)
    files_without_tag = []   # 存储 相对路径
    valid_games = set()      # 存储有效游戏名（去重）
    
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
        
        # 检查是否为有效游戏名
        filename = os.path.basename(file_path)
        if is_valid_game_name(filename):
            puzzle_name = os.path.splitext(filename)[0]
            valid_games.add(puzzle_name)
        
        # 检查是否包含目标字符串
        matching_line = file_contains_string(file_path, SEARCH_STRING)
        
        if matching_line:
            # 包含目标字符串，检查是否有变体
            has_variant = file_contains_variant(file_path)
            
            # 检查同名的txt文件，获取游戏状态
            txt_file_path = file_path.with_suffix('.txt')
            game_status, unsolved_levels, max_level, timeout_levels = check_game_status(txt_file_path)
            
            files_with_tag.append((relative_path, matching_line, has_variant, game_status, unsolved_levels, max_level, timeout_levels))
        else:
            # 不包含目标字符串
            files_without_tag.append(relative_path)
    
    print(f"文件扫描完成，共处理 {file_count} 个文件")
    print(f"有效游戏名总数: {len(valid_games)} 个")
    
    # 生成HTML报告
    print(f"正在生成HTML报告...")
    html_content = generate_html(files_with_tag, files_without_tag, len(valid_games))
    
    # 写入HTML文件
    with open(OUTPUT_HTML, 'w', encoding='utf-8') as f:
        f.write(html_content)
    
    print(f"HTML报告已生成: {OUTPUT_HTML}")
    print(f"包含目标字符串: {len(files_with_tag)} 个文件")
    print(f"不包含目标字符串: {len(files_without_tag)} 个文件")
    print(f"有效游戏名总数: {len(valid_games)} 个")
    
    # 生成Swift版本的拷贝XML脚本
    print("\n正在生成Swift版本拷贝XML脚本...")
    swift_script_content, swift_copy_count = generate_swift_copy_script(files_with_tag)
    
    # 写入shell脚本到 ../LogicPuzzlesSwift 目录
    swift_script_path = os.path.join("..", "LogicPuzzlesSwift", "copy_puzzle_xml.sh")
    
    # 确保目标目录存在
    os.makedirs(os.path.dirname(swift_script_path), exist_ok=True)
    
    with open(swift_script_path, 'w', encoding='utf-8') as f:
        f.write(swift_script_content)
    
    # 设置执行权限
    os.chmod(swift_script_path, 0o755)
    
    print(f"Swift版本脚本已生成: {swift_script_path}")
    print(f"共找到 {swift_copy_count} 个需要拷贝XML的游戏 (iOS实现)")
    
    # 生成Android版本的拷贝XML脚本
    print("\n正在生成Android版本拷贝XML脚本...")
    android_script_content, android_copy_count = generate_android_copy_script(files_with_tag)
    
    # 写入shell脚本到 ../LogicPuzzlesAndroid 目录
    android_script_path = os.path.join("..", "LogicPuzzlesAndroid", "copy_puzzle_xml.sh")
    
    # 确保目标目录存在
    os.makedirs(os.path.dirname(android_script_path), exist_ok=True)
    
    with open(android_script_path, 'w', encoding='utf-8') as f:
        f.write(android_script_content)
    
    # 设置执行权限
    os.chmod(android_script_path, 0o755)
    
    print(f"Android版本脚本已生成: {android_script_path}")
    print(f"共找到 {android_copy_count} 个需要拷贝XML的游戏 (Android实现)")

if __name__ == "__main__":
    main()