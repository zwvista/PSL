#!/usr/bin/env python3
"""
100 LG 自动化截图脚本 - 坐标点击版本
"""

import time
import os
import subprocess
import pyautogui
import sys


def activate_100lg():
    """激活100 LG应用程序"""
    try:
        script = '''
        tell application "100 LG"
            activate
        end tell
        '''
        subprocess.run(['osascript', '-e', script], check=True)
        time.sleep(3)
        return True
    except subprocess.CalledProcessError:
        print("错误: 无法找到或激活100 LG应用程序")
        return False


def get_window_info():
    """获取100 LG窗口信息"""
    try:
        script = '''
        tell application "System Events"
            tell process "100 LG"
                if exists window 1 then
                    set windowPos to position of window 1
                    set windowSize to size of window 1
                    return (item 1 of windowPos as text) & "," & (item 2 of windowPos as text) & "," & (item 1 of windowSize as text) & "," & (item 2 of windowSize as text)
                else
                    return "none"
                end if
            end tell
        end tell
        '''
        result = subprocess.run(['osascript', '-e', script],
                                capture_output=True, text=True, check=True)

        if result.stdout.strip() == "none":
            print("错误: 未找到100 LG窗口")
            return None

        parts = result.stdout.strip().split(',')
        if len(parts) == 4:
            window_x, window_y, window_width, window_height = map(int, parts)
            return {
                'x': window_x,
                'y': window_y,
                'width': window_width,
                'height': window_height
            }
        return None
    except subprocess.CalledProcessError as e:
        print(f"获取窗口信息时出错: {e}")
        return None


def create_screenshot_dir():
    """创建截图目录"""
    desktop_path = os.path.expanduser("~/Desktop")
    screenshot_dir = os.path.join(desktop_path, "100LG_Screenshots")

    try:
        os.makedirs(screenshot_dir, exist_ok=True)
        print(f"截图目录已创建: {screenshot_dir}")
        return screenshot_dir
    except Exception as e:
        print(f"创建目录时出错: {e}")
        return None


def take_window_screenshot(window_info, level_number, screenshot_dir):
    """截取窗口截图"""
    try:
        level_str = f"{level_number:03d}"
        screenshot_path = os.path.join(screenshot_dir, f"Level_{level_str}.png")

        cmd = [
            "screencapture", "-x",
            "-R", f"{window_info['x']},{window_info['y']},{window_info['width']},{window_info['height']}",
            screenshot_path
        ]

        subprocess.run(cmd, check=True)
        print(f"截图已保存: Level_{level_str}.png")
        return True

    except subprocess.CalledProcessError as e:
        print(f"截图失败: {e}")
        return False


def calculate_button_position(window_info, level_number):
    """计算关卡按钮的精确位置"""
    # 每页36个关卡 (6x6网格)
    page_level_index = (level_number - 1) % 36  # 0-35

    # 计算行和列 (0-based)
    row = page_level_index // 6  # 0-5
    col = page_level_index % 6  # 0-5

    # 相对坐标参数
    first_button_offset_x = 62  # 第一个按钮距离窗口左边的偏移
    first_button_offset_y = 245  # 第一个按钮距离窗口顶部的偏移
    button_spacing_x = 93  # 按钮水平间距
    button_spacing_y = 93  # 按钮垂直间距

    # 计算绝对坐标
    button_x = window_info['x'] + first_button_offset_x + (col * button_spacing_x)
    button_y = window_info['y'] + first_button_offset_y + (row * button_spacing_y)

    return button_x, button_y


def calculate_more_button_position(window_info):
    """计算More按钮的位置"""
    more_offset_x = 225  # More按钮相对窗口的X坐标
    more_offset_y = 768  # More按钮相对窗口的Y坐标

    more_x = window_info['x'] + more_offset_x
    more_y = window_info['y'] + more_offset_y

    return more_x, more_y


def calculate_back_button_position(window_info):
    """计算Back按钮的位置"""
    back_offset_x = 59  # Back按钮相对窗口的X坐标
    back_offset_y = 790  # Back按钮相对窗口的Y坐标

    back_x = window_info['x'] + back_offset_x
    back_y = window_info['y'] + back_offset_y

    return back_x, back_y


def click_at_position(x, y, description="位置"):
    """在指定坐标点击"""
    try:
        pyautogui.moveTo(x, y, duration=0.3)
        time.sleep(0.1)
        pyautogui.click()
        print(f"点击{description}: [{x}, {y}]")
        return True
    except Exception as e:
        print(f"点击{description}时出错: {e}")
        return False


def navigate_to_level(window_info, start_level):
    """导航到起始关卡所在的页面"""
    if start_level <= 1:
        print("从第一页开始，无需导航")
        return True

    # 计算起始关卡所在的页面 (0-based)
    start_page = (start_level - 1) // 36

    if start_page == 0:
        print("从第一页开始，无需导航")
        return True

    print(f"导航到第{start_page + 1}页 (关卡{start_level:03d}所在页面)...")

    # 从第一页翻到目标页
    for page in range(start_page):
        more_x, more_y = calculate_more_button_position(window_info)
        if click_at_position(more_x, more_y, f"More(第{page + 1}次翻页)"):
            time.sleep(2)  # 等待翻页
            # 更新窗口信息
            new_window_info = get_window_info()
            if new_window_info:
                window_info.update(new_window_info)
            else:
                print("翻页后无法获取窗口信息")
                return False
        else:
            print(f"第{page + 1}次翻页失败")
            return False

    print(f"成功导航到第{start_page + 1}页")
    return True


def process_level_range(start_level, end_level, window_info, screenshot_dir):
    """
    处理指定范围的关卡

    参数:
    - start_level: 起始关卡号 (从1开始)
    - end_level: 结束关卡号
    - window_info: 窗口信息
    - screenshot_dir: 截图目录
    """
    if start_level > end_level:
        print(f"起始关卡{start_level:03d}大于结束关卡{end_level:03d}，无需处理")
        return

    print(f"处理范围: 关卡{start_level:03d} - 关卡{end_level:03d}")

    # 导航到起始关卡所在的页面
    if not navigate_to_level(window_info, start_level):
        print("导航到起始页面失败")
        return

    current_level = start_level
    current_page = (start_level - 1) // 36  # 当前页面 (0-based)

    while current_level <= end_level:
        # 检查是否需要翻页
        level_in_page = ((current_level - 1) % 36) + 1

        if level_in_page == 1 and current_level > start_level:
            # 需要翻页
            current_page += 1
            more_x, more_y = calculate_more_button_position(window_info)
            if click_at_position(more_x, more_y, f"More(翻到第{current_page + 1}页)"):
                time.sleep(2)
                # 更新窗口信息
                new_window_info = get_window_info()
                if new_window_info:
                    window_info.update(new_window_info)
                else:
                    print("翻页后无法获取窗口信息，停止处理")
                    break
            else:
                print("翻页失败，停止处理")
                break

        print(f"\n[第{current_page + 1}页] 处理关卡 {current_level:03d} (位置{level_in_page}/36)")

        # 处理当前关卡
        try:
            # 点击关卡按钮
            button_x, button_y = calculate_button_position(window_info, current_level)
            if not click_at_position(button_x, button_y, f"关卡{current_level:03d}"):
                print(f"点击关卡{current_level:03d}失败，跳过")
                current_level += 1
                continue

            # 等待游戏加载
            time.sleep(4)

            # 截图
            take_window_screenshot(window_info, current_level, screenshot_dir)

            # 点击返回按钮
            back_x, back_y = calculate_back_button_position(window_info)
            click_at_position(back_x, back_y, "返回")

            # 等待返回完成
            time.sleep(2)

            print(f"✓ 关卡 {current_level:03d} 处理完成")

        except Exception as e:
            print(f"处理关卡 {current_level:03d} 时出错: {e}")

        current_level += 1

    completed_level = min(current_level - 1, end_level)
    print(f"\n关卡范围处理完成: {start_level:03d} - {completed_level:03d}")


def main():
    """主函数"""
    print("=== 100 LG 自动化截图脚本 (坐标点击版) ===")

    # 安全检查
    pyautogui.FAILSAFE = True
    pyautogui.PAUSE = 0.2

    # 激活应用程序
    print("激活100 LG应用程序...")
    if not activate_100lg():
        return

    # 创建截图目录
    screenshot_dir = create_screenshot_dir()
    if not screenshot_dir:
        return

    # 获取窗口信息
    print("获取窗口信息...")
    window_info = get_window_info()
    if not window_info:
        return

    print(f"窗口位置: [{window_info['x']}, {window_info['y']}]")
    print(f"窗口大小: {window_info['width']}x{window_info['height']}")

    # 配置处理参数
    START_LEVEL = 1  # 起始关卡: 从1开始
    END_LEVEL = 81  # 结束关卡号

    print(f"\n配置参数:")
    print(f"起始关卡: 第{START_LEVEL:03d}关")
    print(f"结束关卡: 第{END_LEVEL:03d}关")
    print(f"截图目录: {screenshot_dir}")

    # 执行处理
    print("\n开始处理...")
    print("=" * 50)

    process_level_range(START_LEVEL, END_LEVEL, window_info, screenshot_dir)

    print("=" * 50)
    print("=== 自动化完成 ===")

    # 完成通知
    try:
        script = f'''
        display notification "关卡截图任务已完成！" with title "100 LG 自动化完成" sound name "Glass"
        '''
        subprocess.run(['osascript', '-e', script], check=True)
    except:
        print("(通知发送失败)")


if __name__ == "__main__":
    try:
        import pyautogui
    except ImportError:
        print("请先安装依赖: pip3 install pyautogui")
        sys.exit(1)

    main()