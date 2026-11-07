#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""
serial_monitor.py —— 实验一串口监测程序
========================================
实时显示 STM32F407G-DISC1 经 USB 转 TTL 模块发来的串口输出。

功能：
  * 自动扫描并列出本机所有 COM 口，可 -p 指定（默认自动选择 ST-LINK VCP 之外的口）
  * 9600-8-N-1，与实验配置一致（可用 -b 修改）
  * 每行加本地时间戳；关键信息高亮：
      [KEY]  行 → 黄色      RTC 时间行 → 青色
      含 Error/Fault/HardFault → 红色
  * 同时把全部输出写入日志文件 logs\monitor_YYYYMMDD_HHMMSS.log（报告可直接引用）
  * 交互发送：直接输入字符回车即从串口发出（可用来测回显），Ctrl+C 退出

用法：
  python serial_monitor.py                # 自动选口
  python serial_monitor.py -p COM11       # 指定串口
  python serial_monitor.py -b 115200      # 指定波特率
"""

import argparse
import os
import sys
import time
from datetime import datetime

try:
    import serial          # pyserial
    from serial.tools import list_ports
except ImportError:
    # 本机 Python 为绿色版布局，.pth 路径与启动目录有关；
    # 从其他目录启动时 site-packages 可能不在 sys.path，这里手动补一次
    _sp = r"D:\Lib\site-packages"
    if os.path.isdir(_sp) and _sp not in sys.path:
        sys.path.append(_sp)
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        sys.exit("缺少 pyserial：请先执行  pip install pyserial")

# ---------------------------------------------------------------- 终端颜色
def _use_color() -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    if sys.platform == "win32":
        os.system("")          # 触发 ANSI 支持（Win10+ 终端）
        try:
            import ctypes
            k32 = ctypes.windll.kernel32
            hnd = k32.GetStdHandle(-11)
            mode = ctypes.c_uint32()
            if k32.GetConsoleMode(hnd, ctypes.byref(mode)):
                k32.SetConsoleMode(hnd, mode.value | 0x0004)   # ENABLE_VIRTUAL_TERMINAL
                return True
            return False
        except Exception:
            return False
    return sys.stdout.isatty()

COLOR = _use_color()
def c(text: str, code: str) -> str:
    return f"\x1b[{code}m{text}\x1b[0m" if COLOR else text

def highlight(line: str) -> str:
    if "[KEY]" in line:
        return c(line, "33")                    # 黄
    if any(k in line for k in ("Error", "error", "Fault", "FAIL")):
        return c(line, "31")                    # 红
    if "SYSCLK" in line or "Experiment" in line or "USART2" in line:
        return c(line, "32")                    # 绿（程序横幅）
    if len(line) >= 19 and line[4] == "-" and line[7] == "-" and line[10] == " " \
            and line[13] == ":" and line[16] == ":":
        return c(line, "36")                    # 青（RTC 时间行）
    return line

# ---------------------------------------------------------------- 串口枚举
def pick_port(want: str | None) -> str:
    ports = list(list_ports.comports())
    if not ports:
        sys.exit("未发现任何串口设备，请检查 USB 转 TTL 模块是否插好。")
    print("可用串口：")
    for p in ports:
        desc = f"{p.device}  {p.description}"
        print("  " + c(desc, "32" if "ST-Link" not in p.description else "90"))
    if want:
        return want
    # 优先选非 ST-Link 虚拟串口（板载 VCP 没接 PA2/PA3）
    cand = [p for p in ports if "ST-Link" not in p.description]
    return (cand or ports)[0].device

# ---------------------------------------------------------------- 主程序
def main() -> None:
    ap = argparse.ArgumentParser(description="实验一串口实时监测")
    ap.add_argument("-p", "--port", help="串口号，如 COM11（默认自动选择）")
    ap.add_argument("-b", "--baud", type=int, default=9600, help="波特率，默认 9600")
    args = ap.parse_args()

    port = pick_port(args.port)
    logdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
    os.makedirs(logdir, exist_ok=True)
    logfile = os.path.join(logdir, f"monitor_{datetime.now():%Y%m%d_%H%M%S}.log")

    print(f"\n打开 {port} @ {args.baud} 8N1    日志: {logfile}")
    print(c(">>> 输入字符回车即发送到串口（测试回显）；Ctrl+C 退出 <<<\n", "90"))

    try:
        ser = serial.Serial(port, args.baud, bytesize=8, parity="N",
                            stopbits=1, timeout=0.1)
    except serial.SerialException as e:
        sys.exit(f"串口打开失败: {e}")

    rx_buf = bytearray()
    running = True
    while running:
        # --- 读串口 ---
        try:
            chunk = ser.read(256)
        except serial.SerialException as e:
            print(c(f"\n串口错误: {e}", "31"))
            break
        if chunk:
            rx_buf += chunk
            while b"\n" in rx_buf:
                raw, _, rest = rx_buf.partition(b"\n")
                rx_buf = rest
                line = raw.decode("ascii", "replace").rstrip("\r")
                if not line:
                    continue
                stamp = datetime.now().strftime("%H:%M:%S")
                out = f"[{stamp}] {highlight(line)}"
                print(out, flush=True)
                with open(logfile, "a", encoding="utf-8") as f:
                    f.write(f"[{stamp}] {line}\n")
        # --- 键盘发送（非阻塞轮询）---
        if sys.platform == "win32":
            import msvcrt
            typed = bytearray()
            while msvcrt.kbhit():
                ch = msvcrt.getwche()
                if ch in ("\r", "\n"):
                    if typed:
                        msg = typed.decode("ascii", "replace") + "\r\n"
                        ser.write(msg.encode())
                        print(c(f"[TX] {typed.decode('ascii','replace')}", "35"), flush=True)
                        with open(logfile, "a", encoding="utf-8") as f:
                            f.write(f"[TX] {typed.decode('ascii','replace')}\n")
                    typed.clear()
                elif ch == "\x03":                       # Ctrl+C
                    running = False
                    break
                elif ch == "\x08":
                    if typed:
                        typed.pop()
                else:
                    typed += ch
        time.sleep(0.02)

    ser.close()
    print(f"\n已退出，日志保存在 {logfile}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n已退出")
