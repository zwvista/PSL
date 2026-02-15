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
        - ("Solved", []): 有txt文件且完全解决，所有关卡连续
        - ("Partly Solved", []): 有txt文件且所有关卡都有解决方案，但关卡号不连续
        - ("Unsolved", unsolved_levels): 有txt文件但有关卡未解决
        - ("No Solutions", []): 有txt文件但没有可识别的关卡号（无法转换为整数）
        - ("No Solution File", []): 没有txt文件
    """
    if not os.path.exists(txt_file_path):
        return "No Solution File", []
    
    try:
        with open(txt_file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return "No Solution File", []  # 无法读取文件，视为没有解决方案文件
    
    solved_levels = []      # 已解决的关卡号（整数）
    unsolved_levels = []    # 未解决的关卡号（原始字符串）
    all_levels = []         # 所有出现的关卡号（原始字符串）
    has_any_level = False   # 是否找到任何Level行
    
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # 检查是否是 "Level " 开头的行
        if line.startswith("Level "):
            has_any_level = True
            # 提取关卡号（"Level "后面的内容）
            level_str = line[6:].strip()
            all_levels.append(level_str)
            
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
        return "No Solutions", []
    
    # 判断游戏状态
    if unsolved_levels:
        return "Unsolved", unsolved_levels
    elif not solved_levels:
        # 有Level行，但所有Level号都无法转换为整数
        return "No Solutions", []
    elif not check_numbers_continuous(solved_levels):
        # 所有关卡都有解决方案，但关卡号不连续
        return "Partly Solved", []
    else:
        return "Solved", []

def generate_html(files_with_tag, files_without_tag):
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
    <title>Puzzle Status Report</title>
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
        .file-path {{
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            color: #7f8c8d;
        }}
        .puzzle-info {{
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
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
    <h1>Puzzle Status Report</h1>
    
    <div class="summary">
        <span class="summary-item">📊 总计: {len(files_with_tag) + len(files_without_tag)} 个文件</span>
        <span class="summary-item">✅ 包含标签: {len(files_with_tag)} 个</span>
        <span class="summary-item">❌ 不包含标签: {len(files_without_tag)} 个</span>
    </div>
"""
    
    # 第一部分：包含标签的文件
    html += f"""
    <h2>📋 Files With Logic Game Tag</h2>
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>文件路径</th>
                <th>游戏信息</th>
                <th>变体</th>
                <th>状态</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for item in sorted(files_with_tag, key=lambda x: os.path.basename(x[0]).lower()):
        path, line, has_variant, game_status, unsolved_levels = item
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        
        # 变体列
        variant_cell = '<span class="variant-yes">⭕️</span>' if has_variant else ''
        
        # 状态列
        status_text = status_display.get(game_status, game_status)
        status_class = status_classes.get(game_status, "")
        
        # 对于Unsolved状态，添加未解决的关卡号
        if game_status == "Unsolved" and unsolved_levels:
            levels_str = ", ".join(unsolved_levels)
            status_text = f"❌ 未解决 ({levels_str})"
        
        html += f"""            <tr>
                <td><strong>{puzzle_name}</strong></td>
                <td class="file-path">{path}</td>
                <td class="puzzle-info">{line}</td>
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
    <table>
        <thead>
            <tr>
                <th>游戏名</th>
                <th>文件路径</th>
            </tr>
        </thead>
        <tbody>
"""
    
    # 按游戏名排序（大小写不敏感）
    for path in sorted(files_without_tag, key=lambda x: os.path.basename(x).lower()):
        puzzle_name = os.path.splitext(os.path.basename(path))[0]
        html += f"""            <tr>
                <td><strong>{puzzle_name}</strong></td>
                <td class="file-path">{path}</td>
            </tr>
"""
    
    # 统计信息
    solved_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "Solved")
    partly_solved_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "Partly Solved")
    unsolved_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "Unsolved")
    no_solutions_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "No Solutions")
    no_solution_file_count = sum(1 for _, _, _, status, _ in files_with_tag if status == "No Solution File")
    
    html += f"""        </tbody>
    </table>
    
    <div class="summary">
        <h3>📊 状态统计 (包含标签的文件)</h3>
        <span class="summary-item">✅ 已解决: {solved_count}</span>
        <span class="summary-item">⚠️ 部分解决: {partly_solved_count}</span>
        <span class="summary-item">❌ 未解决: {unsolved_count}</span>
        <span class="summary-item">📄 无有效解决方案: {no_solutions_count}</span>
        <span class="summary-item">❓ 无解决方案文件: {no_solution_file_count}</span>
    </div>
    
    <div class="footer">
        生成时间: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
    </div>
</body>
</html>
"""
    
    return html

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
    
    # 生成HTML报告
    print(f"正在生成HTML报告...")
    html_content = generate_html(files_with_tag, files_without_tag)
    
    # 写入HTML文件
    with open(OUTPUT_HTML, 'w', encoding='utf-8') as f:
        f.write(html_content)
    
    print(f"HTML报告已生成: {OUTPUT_HTML}")
    print(f"包含目标字符串: {len(files_with_tag)} 个文件")
    print(f"不包含目标字符串: {len(files_without_tag)} 个文件")

if __name__ == "__main__":
    main()