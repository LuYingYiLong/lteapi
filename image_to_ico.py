"""
将 PNG 批量转成多分辨率 ICO
用法:
    # 单文件
    python png2ico.py icon.png

    # 批量
    python png2ico.py *.png -o ./icons

    # 只生成指定尺寸
    python png2ico.py icon.png -s 32 64 128
"""
import argparse
import sys
import glob
from pathlib import Path
from PIL import Image
from pathlib import Path

# 默认打包进 ICO 的分辨率（从小到大）
DEFAULT_SIZES = [(16, 16), (24, 24), (32, 32),
                 (48, 48), (64, 64), (128, 128), (256, 256)]


def png_to_ico(png_path: Path, ico_path: Path, sizes=None):
    """核心转换函数"""
    sizes = sizes or DEFAULT_SIZES
    try:
        img = Image.open(png_path).convert("RGBA")
        # 提前生成所有尺寸，Pillow 会自动打包进 ICO
        img.save(ico_path, format="ICO", sizes=sizes)
        return True
    except Exception as e:
        print(f"[ERROR] 转换失败: {png_path} -> {e}", file=sys.stderr)
        return False


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="PNG → ICO 批量转换工具")
    p.add_argument("png", nargs="+", help="输入 PNG 文件（支持通配符）")
    p.add_argument("-o", "--out", help="输出目录（默认：同输入文件）")
    p.add_argument("-s", "--sizes", type=int, nargs="+",
                   help="指定边长，如 32 64 128（默认内置多级）")
    p.add_argument("-q", "--quiet", action="store_true",
                   help="静默模式")
    return p


def main(argv = None):
    args = build_arg_parser().parse_args(argv)

    sizes = [(s, s) for s in args.sizes] if args.sizes else None
    out_dir = Path(args.out) if args.out else None

    files = []
    for pattern in args.png:
        p = Path(pattern)
        if not glob.has_magic(pattern) and p.is_file():
            files.append(p)
            continue
        matched = [Path(f) for f in glob.glob(pattern, recursive=False)]
        if not matched:
            print(f"[WARN] 模式未匹配到任何文件: {pattern}", file=sys.stderr)
        files.extend(matched)

    ok = 0
    for png in files:
        if not png.is_file():
            print(f"[SKIP] 不存在: {png}", file=sys.stderr)
            continue
        # 生成输出路径
        ico_name = png.with_suffix(".ico").name
        dest = (out_dir / ico_name) if out_dir else png.with_suffix(".ico")
        if out_dir:
            dest.parent.mkdir(parents=True, exist_ok=True)

        if png_to_ico(png, dest, sizes):
            ok += 1
            if not args.quiet:
                print(f"[OK] {png} -> {dest}")

    if not args.quiet:
        print(f"\n完成: {ok}/{len(files)} 个文件转换成功")
    sys.exit(0 if ok == len(files) else 1)


if __name__ == "__main__":
    main()