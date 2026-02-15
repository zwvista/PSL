#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Puzzle Game Status Analyzer (puzzle_game_analyzer.py)
分析C++游戏文件，提取游戏信息并生成HTML状态报告
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
    """
    name_without_ext = os.path.splitext(filename)[0]
    
    # 检查是否以Gen结尾（不区分大小写）
    if name_without_ext.lower().endswith('gen'):
        return False
    
    # 检查是否以数字结尾
    if re.search(r'\d+$', name_without_ext):
        return False
    
    return True

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

def check_game_status(txt_file_path):
    """
    检查游戏状态
    返回值：
        - ("Solved", [], max_level): 有txt文件且完全解决，所有关卡连续
        - ("Partly Solved", [], max_level): 有txt文件且所有关卡都有解决方案，但关卡号不连续
        - ("Unsolved", unsolved_levels, max_level): 有txt文件但有关卡未解决
        - ("No Solutions", [], max_level): 有txt文件但没有可识别的关卡号（无法转换为整数）
        - ("No Solution File", [], None): 没有txt文件
    """
    if not os.path.exists(txt_file_path):
        return "No Solution File", [], None
    
    try:
        with open(txt_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return "No Solution File", [], None  # 无法读取文件，视为没有解决方案文件
    
    solved_levels = []      # 已解决的关卡号（整数）
    unsolved_levels = []    # 未解决的关卡号（原始字符串）
    all_levels = []         # 所有出现的关卡号（原始字符串）
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
            all_levels.append(level_str)
            
            # 尝试转换为整数，用于计算最大值
            try:
                level_num = int(level_str)
                if max_level is None or level_num > max_level:
                    max_level = level_num
            except ValueError:
                pass
            
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
                # 尝试将关卡号转换为整数，用于连续性检查
                try:
                    solved_levels.append(int(level_str))
                except ValueError:
                    # 如果无法转换为整数，忽略（不参与连续性检查）
                    pass
                i += 2
        else:
            i += 1
    
    # 如果没有找到任何Level行，视为No Solutions
    if not has_any_level:
        return "No Solutions", [], max_level
    
    # 判断游戏状态
    if unsolved_levels:
        return "Unsolved", unsolved_levels, max_level
    elif not solved_levels:
        # 有Level行，但所有Level号都无法转换为整数
        return "No Solutions", [], max_level
    elif not check_numbers_continuous(solved_levels):
        # 所有关卡都有解决方案，但关卡号不连续
        return "Partly Solved", [], max_level
    else:
        return "Solved", [], max_level

def get_group_name(file_path):
    """从文件路径中提取组名（第一个斜杠之前的内容）"""
    # 使用正斜杠分割路径
    parts = file_path.split('/')
    if parts:
        return parts[0]
    return file_path

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
        .variant-yes {{
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
    
    # 统计信息（第一部分）
    solved_count = sum(1 for _, _, _, status, _, _ in files_with_tag if status == "Solved")
    partly_solved_count = sum(1 for _, _, _, status, _, _ in files_with_tag if status == "Partly Solved")
    unsolved_count = sum(1 for _, _, _, status, _, _ in files_with_tag if status == "Unsolved")
    no_solutions_count = sum(1 for _, _, _, status, _, _ in files_with_tag if status == "No Solutions")
    no_solution_file_count = sum(1 for _, _, _, status, _, _ in files_with_tag if status == "No Solution File")
    
    # 计算第一部分中的有效游戏数量
    valid_in_tag = 0
    for item in files_with_tag:
        path = item[0]  # 第一个元素是文件路径
        filename = os.path.basename(path)
        if is_valid_game_name(filename):
            valid_in_tag += 1
    
    html += f"""    <div class="status-summary">
        <h3>📊 状态统计</h3>
        <div class="stats">
            <span class="stat-item status-solved">✅ 已解决: {solved_count}</span>
            <span class="stat-item status-partly">⚠️ 部分解决: {partly_solved_count}</span>
            <span class="stat-item status-unsolved">❌ 未解决: {unsolved_count}</span>
            <span class="stat-item status-no-solutions">📄 无有效解决方案: {no_solutions_count}</span>
            <span class="stat-item status-no-file">❓ 无解决方案文件: {no_solution_file_count}</span>
            <span class="stat-item valid-games">🎮 有效游戏: {valid_in_tag}</span>
        </div>
    </div>
    
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>组名</th>
                <th>游戏信息</th>
                <th>Game Set</th>
                <th>Puzzle Set</th>
                <th>Game Title</th>
                <th>关卡数</th>
                <th>变体</th>
                <th>状态</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for item in sorted(files_with_tag, key=lambda x: os.path.basename(x[0]).lower()):
        path, line, has_variant, game_status, unsolved_levels, max_level = item
        
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        group_name = get_group_name(path)
        
        # 解析游戏信息
        game_set, puzzle_set, game_title = parse_game_info(line)
        
        # 变体列
        variant_cell = '<span class="variant-yes">⭕️</span>' if has_variant else ''
        
        # 状态列
        status_text = status_display.get(game_status, game_status)
        status_class = status_classes.get(game_status, "")
        
        # 对于Unsolved状态，添加未解决的关卡号
        if game_status == "Unsolved" and unsolved_levels:
            levels_str = ", ".join(unsolved_levels)
            status_text = f"❌ 未解决 ({levels_str})"
        
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
                <td><strong>{puzzle_name}</strong></td>
                <td class="group-name">{group_name}</td>
                <td class="puzzle-info">{line}</td>
                <td class="numeric-cell">{game_set_cell}</td>
                <td class="numeric-cell">{puzzle_set_cell}</td>
                <td>{game_title_cell}</td>
                <td class="numeric-cell">{level_cell}</td>
                <td class="variant-yes">{variant_cell}</td>
                <td><span class="status-badge {status_class}">{status_text}</span></td>
            </tr>
"""
    
    html += """        </tbody>
    </table>
"""
    
    # 第二部分：不包含标签的文件
    html += f"""
    <h2>📋 Files Without Logic Game Tag</h2>
"""
    
    # 统计信息（第二部分）
    valid_in_without = sum(1 for path in files_without_tag if is_valid_game_name(os.path.basename(path)))
    
    html += f"""    <div class="status-summary">
        <h3>📊 统计信息</h3>
        <div class="stats">
            <span class="stat-item">📄 文件总数: {len(files_without_tag)}</span>
            <span class="stat-item valid-games">🎮 有效游戏: {valid_in_without}</span>
        </div>
    </div>
    
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>组名</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for path in sorted(files_without_tag, key=lambda x: os.path.basename(x).lower()):
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        group_name = get_group_name(path)
        html += f"""            <tr>
                <td><strong>{puzzle_name}</strong></td>
                <td class="group-name">{group_name}</td>
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
    
    files_with_tag = []      # 存储 (相对路径, 匹配行, 是否有变体, 游戏状态, 未解决关卡列表, 最大关卡数)
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
            game_status, unsolved_levels, max_level = check_game_status(txt_file_path)
            
            files_with_tag.append((relative_path, matching_line, has_variant, game_status, unsolved_levels, max_level))
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

if __name__ == "__main__":
    main()