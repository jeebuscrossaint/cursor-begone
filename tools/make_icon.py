"""Generate cursor-begone.ico from scratch.

Design: a classic cursor arrow with a red diagonal strike-through —
the universal "no" symbol applied to a mouse pointer.

Output: ../assets/cursor-begone.ico  (multi-resolution: 16/32/48/64/128/256)
"""
from PIL import Image, ImageDraw
from pathlib import Path

SIZES = [16, 32, 48, 64, 128, 256]


def draw_arrow(draw: ImageDraw.ImageDraw, s: int) -> None:
    # Classic Windows-style cursor pointer, proportional coords
    arrow = [
        (0.18 * s, 0.10 * s),  # tip
        (0.18 * s, 0.72 * s),  # bottom-left edge
        (0.34 * s, 0.58 * s),  # inner notch
        (0.46 * s, 0.86 * s),  # tail bottom point
        (0.56 * s, 0.82 * s),  # tail top point
        (0.44 * s, 0.54 * s),  # tail base
        (0.62 * s, 0.42 * s),  # right edge of head
    ]
    outline_w = max(1, s // 20)
    draw.polygon(arrow, fill=(245, 245, 245, 255),
                 outline=(20, 20, 20, 255), width=outline_w)


def draw_strike(draw: ImageDraw.ImageDraw, s: int) -> None:
    # Bold diagonal "no" line, top-right to bottom-left
    line_w = max(2, s // 10)
    pad = s * 0.08
    draw.line(
        [(s - pad, pad), (pad, s - pad)],
        fill=(220, 40, 40, 255),
        width=line_w,
    )


def make_icon(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_arrow(draw, size)
    draw_strike(draw, size)
    return img


def main() -> None:
    out_dir = Path(__file__).resolve().parent.parent / "assets"
    out_dir.mkdir(exist_ok=True)
    out_path = out_dir / "cursor-begone.ico"

    # Render each size natively for crisp small-size rendering
    images = [make_icon(s) for s in SIZES]
    images[-1].save(
        out_path,
        format="ICO",
        sizes=[(s, s) for s in SIZES],
        append_images=images[:-1],
    )
    print(f"wrote {out_path}  ({out_path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
