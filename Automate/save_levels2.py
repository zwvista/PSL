#!/usr/bin/env python3
"""
100 LG 自动化截图脚本 - 使用精确UI路径点击按钮
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


def click_level_button(level_number):
    """点击关卡按钮（使用精确UI路径）"""
    # 计算在当前页面的按钮索引 (1-36)
    button_index = ((level_number - 1) % 36) + 1

    try:
        script = f'''
        tell application "System Events"
            tell process "100 LG"
                click static text 1 of button {button_index} of group 2 of group 1 of group 1 of group 1 of group 1 of group 1 of group 1 of group 2 of group 2 of window 1
            end tell
        end tell
        '''

        subprocess.run(['osascript', '-e', script], check=True)
        print(f"✓ 点击关卡按钮 {level_number:03d} (按钮索引: {button_index})")
        return True

    except subprocess.CalledProcessError as e:
        print(f"✗ 点击关卡按钮 {level_number:03d} 失败: {e}")
        return False


def click_more_button():
    """点击More按钮（使用精确UI路径）"""
    try:
        script = '''
        tell application "System Events"
            tell process "100 LG"
                click static text 1 of button 1 of group 3 of group 1 of group 1 of group 1 of group 1 of group 1 of group 1 of group 2 of group 2 of window 1
            end tell
        end tell
        '''

        subprocess.run(['osascript', '-e', script], check=True)
        print("✓ 点击More按钮")
        return True

    except subprocess.CalledProcessError as e:
        print(f"✗ 点击More按钮失败: {e}")
        return False


def click_back_button():
    """点击Back按钮（使用精确UI路径）"""
    try:
        script = '''
        tell application "System Events"
            tell process "100 LG"
                click button 1 of group 5 of group 1 of group 1 of group 1 of group 1 of group 1 of group 1 of group 2 of group 2 of window 1
            end tell
        end tell
        '''

        subprocess.run(['osascript', '-e', script], check=True)
        print("✓ 点击Back按钮")
        return True

    except subprocess.CalledProcessError as e:
        print(f"✗ 点击Back按钮失败: {e}")
        return False


def navigate_to_page(target_page):
    """导航到指定页面"""
    if target_page == 0:
        print("已在第一页，无需导航")
        return True

    print(f"导航到第{target_page + 1}页...")

    # 从当前页翻到目标页
    for page in range(target_page):
        if click_more_button():
            time.sleep(2)  # 等待翻页
        else:
            print(f"第{page + 1}次翻页失败")
            return False

    print(f"成功导航到第{target_page + 1}页")
    return True


def process_level_range(start_page, end_level, window_info, screenshot_dir):
    """
    处理指定范围的关卡

    参数:
    - start_page: 起始页面 (0-12)，0表示第一页
    - end_level: 结束关卡号 (如400)
    - window_info: 窗口信息
    - screenshot_dir: 截图目录
    """
    # 计算起始关卡号
    start_level = start_page * 36 + 1
    if start_level > end_level:
        print(f"起始关卡{start_level}大于结束关卡{end_level}，无需处理")
        return

    print(f"处理范围: 第{start_page + 1}页 关卡{start_level:03d} - 关卡{end_level:03d}")

    # 导航到起始页面
    if not navigate_to_page(start_page):
        print("导航到起始页面失败，停止处理")
        return

    current_level = start_level
    current_page = start_page

    while current_level <= end_level:
        # 检查是否需要翻页
        level_in_page = ((current_level - 1) % 36) + 1

        if level_in_page == 1 and current_level > start_level:
            # 需要翻页
            current_page += 1
            if click_more_button():
                time.sleep(2)
            else:
                print("翻页失败，停止处理")
                break

        print(f"\n[第{current_page + 1}页] 处理关卡 {current_level:03d} (位置{level_in_page}/36)")

        # 处理当前关卡
        try:
            # 点击关卡按钮
            if not click_level_button(current_level):
                print("点击关卡按钮失败，停止处理")
                break

            # 等待游戏加载
            time.sleep(4)

            # 截图
            if not take_window_screenshot(window_info, current_level, screenshot_dir):
                print("截图失败，但继续处理")

            # 点击返回按钮
            if not click_back_button():
                print("点击返回按钮失败，停止处理")
                break

            # 等待返回完成
            time.sleep(2)

            print(f"✓ 关卡 {current_level:03d} 处理完成")

        except Exception as e:
            print(f"处理关卡 {current_level:03d} 时出错: {e}")
            print("停止处理")
            break

        current_level += 1

    completed_level = min(current_level - 1, end_level)
    print(f"\n关卡范围处理完成: {start_level:03d} - {completed_level:03d}")
    return completed_level


def main():
    """主函数"""
    print("=== 100 LG 自动化截图脚本 (精确UI路径版) ===")

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

    # 获取窗口信息（主要用于截图）
    print("获取窗口信息...")
    window_info = get_window_info()
    if not window_info:
        return

    print(f"窗口位置: [{window_info['x']}, {window_info['y']}]")
    print(f"窗口大小: {window_info['width']}x{window_info['height']}")

    # 配置处理参数
    START_PAGE = 1  # 起始页面: 0=第一页, 1=第二页, 等等
    END_LEVEL = 50  # 结束关卡号

    print(f"\n配置参数:")
    print(f"起始页面: 第{START_PAGE + 1}页 (从关卡{START_PAGE * 36 + 1:03d}开始)")
    print(f"结束关卡: 第{END_LEVEL:03d}关")
    print(f"截图目录: {screenshot_dir}")

    # 执行处理
    print("\n开始处理...")
    print("=" * 50)

    try:
        completed_level = process_level_range(START_PAGE, END_LEVEL, window_info, screenshot_dir)

        print("=" * 50)
        print("=== 自动化完成 ===")
        print(f"成功处理到关卡: {completed_level:03d}")

        # 完成通知
        try:
            script = f'''
            display notification "已完成关卡 {START_PAGE * 36 + 1:03d} 到 {completed_level:03d} 的截图！" with title "100 LG 自动化完成" sound name "Glass"
            '''
            subprocess.run(['osascript', '-e', script], check=True)
        except:
            print("(通知发送失败)")

    except KeyboardInterrupt:
        print("\n用户中断操作")
    except Exception as e:
        print(f"程序执行出错: {e}")


if __name__ == "__main__":
    try:
        import pyautogui
    except ImportError:
        print("请先安装依赖: pip3 install pyautogui")
        sys.exit(1)

    main()