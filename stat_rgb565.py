# -*- coding: utf-8 -*-
"""
统计素材 PNG 转 RGB565 后的体积。
仅解析 PNG 头(IHDR)取宽高，无需 Pillow。
体积估算 = 宽 x 高 x 每像素字节数(RGB565 默认 2)。
用法:
  python stat_rgb565.py                    # 用默认素材目录
  python stat_rgb565.py --root <目录>
  python stat_rgb565.py --px 1             # 索引色 1 字节/像素
"""
import os, sys, struct, argparse

def png_size(path):
    try:
        with open(path, 'rb') as f:
            data = f.read(33)
    except OSError:
        return None, None
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        return None, None
    if data[12:16] != b'IHDR':
        return None, None
    w = struct.unpack('>I', data[16:20])[0]
    h = struct.unpack('>I', data[20:24])[0]
    return w, h

def human(n):
    if n >= 1024 * 1024:
        return f"{n/1024/1024:.2f} MB"
    if n >= 1024:
        return f"{n/1024:.1f} KB"
    return f"{n} B"

def main():
    parser = argparse.ArgumentParser(description="统计素材 PNG 转 RGB565 后的体积")
    parser.add_argument("--root", default=r"D:\PYH_PRJ\GPlus\鲸落素材-汽车_UI控件提取\鲸落素材-汽车 (2)_UI控件提取_1024x600")
    parser.add_argument("--px", type=int, default=2, help="每像素字节数(默认2=RGB565, 1=索引色)")
    args = parser.parse_args()

    root = args.root
    if not os.path.isdir(root):
        print("找不到目录:", root)
        sys.exit(1)

    totals = {}     # 子目录 -> 字节
    files = []      # (子目录, 文件名, w, h, 字节)
    grand = 0
    for dirpath, _dirnames, filenames in os.walk(root):
        for fn in sorted(filenames):
            if not fn.lower().endswith('.png'):
                continue
            p = os.path.join(dirpath, fn)
            w, h = png_size(p)
            if w is None:
                print("  跳过无法解析:", p)
                continue
            folder = os.path.basename(dirpath)
            b = w * h * args.px
            files.append((folder, fn, w, h, b))
            totals[folder] = totals.get(folder, 0) + b
            grand += b

    if not files:
        print("没有找到 PNG")
        sys.exit(0)

    print(f"根目录: {root}")
    print(f"像素深度: {args.px} 字节/像素 (RGB565=2)")
    print(f"PNG 文件数: {len(files)}\n")

    print(f"{'子目录':<18}{'文件':<44}{'W':>6}{'H':>6}{'RGB565':>12}")
    print("-" * 92)
    for folder, fn, w, h, b in files:
        print(f"{folder:<18}{fn:<44}{w:>6}{h:>6}{human(b):>12}")

    print("\n======== 分目录汇总 ========")
    for folder in sorted(totals):
        cnt = sum(1 for f in files if f[0] == folder)
        print(f"{folder:<18}{cnt:>3} 张  ->  {human(totals[folder])}")

    print("\n======== 总计 ========")
    print(f"全部 PNG 转 RGB565 合计: {human(grand)} ({grand} B)")

    base = 1024 * 600 * args.px
    nor = 8 * 1024 * 1024
    pct = grand / nor * 100
    print(f"\n参考: 整幅 1024x600 底图 RGB565 = {human(base)} ({base} B)")
    print(f"参考: 8MB NOR = {human(nor)}")
    print(f"全部素材转 RGB565 约占 NOR: {pct:.1f}%  (未计入字库/固件代码)")

    print("\n======== 体积最大的 5 张 ========")
    for folder, fn, w, h, b in sorted(files, key=lambda x: x[4], reverse=True)[:5]:
        print(f"{folder:<18}{fn:<44}{w}x{h} -> {human(b)}")

if __name__ == "__main__":
    main()
