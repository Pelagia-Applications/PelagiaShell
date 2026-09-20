import argparse
import csv
import sys
import tkinter as tk
from tkinter import ttk


THEMES = {
    "ocean": {
        "bg": "#071a2d",
        "fg": "#dff6ff",
        "panel": "#102944",
        "header_bg": "#1e4f7a",
        "header_fg": "#e6f7ff",
        "accent": "#5ec7ff",
        "button": "#1c759f",
        "button_fg": "#f3fbff",
        "tree_bg": "#0e233c",
        "tree_fg": "#e9f6ff",
        "tree_sel": "#0f7db8",
        "tree_sel_fg": "#ffffff",
        "alt_row": "#112d46",
        "number": "#7ee7ff",
        "string": "#ffd166",
        "keyword": "#8ef5a1",
        "warning": "#ff7f7f",
    },
    "sunset": {
        "bg": "#2a120f",
        "fg": "#fff1e3",
        "panel": "#4d231f",
        "header_bg": "#b85c2c",
        "header_fg": "#fff3e5",
        "accent": "#ffb15a",
        "button": "#d96c38",
        "button_fg": "#fff6ee",
        "tree_bg": "#3a1917",
        "tree_fg": "#fff5ee",
        "tree_sel": "#ff8a5b",
        "tree_sel_fg": "#ffffff",
        "alt_row": "#4d231f",
        "number": "#ffd38a",
        "string": "#ffe8a3",
        "keyword": "#ff9f7d",
        "warning": "#ffe0a3",
    },
    "neon": {
        "bg": "#120b1d",
        "fg": "#f8e4ff",
        "panel": "#1d1230",
        "header_bg": "#7428c7",
        "header_fg": "#fff2ff",
        "accent": "#ff57c8",
        "button": "#a63de8",
        "button_fg": "#fff7ff",
        "tree_bg": "#170d26",
        "tree_fg": "#f7ecff",
        "tree_sel": "#ff4fd8",
        "tree_sel_fg": "#141114",
        "alt_row": "#22113a",
        "number": "#ffb3f5",
        "string": "#ffd86b",
        "keyword": "#7ef7c9",
        "warning": "#ff6fb7",
    },
    "hacker": {
        "bg": "#030b06",
        "fg": "#b7ffbf",
        "panel": "#081d0d",
        "header_bg": "#0e5d1d",
        "header_fg": "#d5ffd8",
        "accent": "#52ff75",
        "button": "#0f7e29",
        "button_fg": "#f3fff4",
        "tree_bg": "#05170a",
        "tree_fg": "#d9ffe0",
        "tree_sel": "#18b848",
        "tree_sel_fg": "#021106",
        "alt_row": "#0c200d",
        "number": "#7dffb3",
        "string": "#d4ff8d",
        "keyword": "#6efaee",
        "warning": "#ff9f7d",
    },
}


def parse_args():
    parser = argparse.ArgumentParser(description="GridView-style GUI for PelagiaShell")
    parser.add_argument("--csv", required=True, help="CSV/TSV file to display")
    parser.add_argument("--delimiter", default=",", help="Delimiter: comma, semicolon, pipe, tab")
    parser.add_argument("--title", default="PelagiaShell GridView", help="Window title")
    parser.add_argument("--theme", default="ocean", choices=sorted(THEMES.keys()), help="PelagiaShell theme palette")
    return parser.parse_args()


def normalize_delimiter(value):
    value = (value or "").strip().lower()
    if value in {"comma", ","}:
        return ","
    if value in {"semicolon", ";"}:
        return ";"
    if value in {"pipe", "|"}:
        return "|"
    if value in {"tab", "\\t", "\t"}:
        return "\t"
    return value or ","


def load_rows(csv_path, delimiter):
    rows = []
    with open(csv_path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        reader = csv.reader(fh, delimiter=delimiter)
        for row in reader:
            if any(cell.strip() for cell in row):
                rows.append(row)
    return rows


def build_copy_text(tree):
    selected = tree.selection()
    if not selected:
        return ""
    values = tree.item(selected[0], "values")
    return "\t".join(str(v) for v in values)


def color_for_value(value, palette):
    text = str(value).strip()
    if not text:
        return palette["tree_fg"]
    lowered = text.lower()
    if lowered in {"true", "false", "null", "none", "ok", "error", "warning", "info"}:
        return palette["keyword"]
    try:
        float(text)
        return palette["number"]
    except ValueError:
        pass
    if any(ch in text for ch in [":", "=", "/", "-", "."]) or text.startswith("#"):
        return palette["string"]
    if text.upper() in {"ERROR", "FAIL", "WARN", "WARNING"}:
        return palette["warning"]
    return palette["tree_fg"]


def apply_theme(root, theme_name):
    palette = THEMES.get(theme_name, THEMES["ocean"])

    root.configure(bg=palette["bg"])
    style = ttk.Style(root)
    style.theme_use("clam")

    style.configure("Treeview", background=palette["tree_bg"], foreground=palette["tree_fg"], fieldbackground=palette["tree_bg"], rowheight=28)
    style.map("Treeview", background=[("selected", palette["tree_sel"])], foreground=[("selected", palette["tree_sel_fg"])])
    style.configure("Treeview.Heading", background=palette["header_bg"], foreground=palette["header_fg"], relief="flat", font=("Segoe UI", 10, "bold"))
    style.map("Treeview.Heading", background=[("selected", palette["header_bg"])])
    style.configure("TFrame", background=palette["panel"])
    style.configure("TLabel", background=palette["panel"], foreground=palette["fg"], font=("Segoe UI", 10))
    style.configure("TButton", background=palette["button"], foreground=palette["button_fg"], padding=(10, 6), font=("Segoe UI", 10, "bold"))
    style.map("TButton", background=[("active", palette["accent"])])
    style.configure("Horizontal.TScrollbar", background=palette["accent"], troughcolor=palette["panel"], bordercolor=palette["panel"], arrowcolor=palette["fg"])
    style.configure("Vertical.TScrollbar", background=palette["accent"], troughcolor=palette["panel"], bordercolor=palette["panel"], arrowcolor=palette["fg"])
    style.configure("OddRow.TFrame", background=palette["alt_row"])
    for tag_name in ["odd", "even"]:
        style.map("Treeview", background=[("selected", palette["tree_sel"])])

    return palette


def main():
    args = parse_args()
    rows = load_rows(args.csv, normalize_delimiter(args.delimiter))
    if not rows:
        raise ValueError(f"No usable rows found in {args.csv}")

    theme = args.theme.lower()
    root = tk.Tk()
    palette = apply_theme(root, theme)
    root.title(args.title)
    root.minsize(720, 420)
    root.deiconify()
    root.state("normal")
    root.attributes("-topmost", True)
    root.update_idletasks()
    width = root.winfo_screenwidth() - 80
    height = root.winfo_screenheight() - 120
    x = (root.winfo_screenwidth() - width) // 2
    y = (root.winfo_screenheight() - height) // 2
    root.geometry(f"{width}x{height}+{x}+{y}")
    root.state("zoomed")
    root.focus_force()
    root.update_idletasks()
    root.after(120, lambda: root.attributes("-topmost", False))

    frame = ttk.Frame(root, padding=10)
    frame.pack(fill=tk.BOTH, expand=True)
    frame.configure(style="TFrame")

    tree = ttk.Treeview(frame, show="headings", selectmode="browse")
    tree.pack(fill=tk.BOTH, expand=True, side=tk.TOP)

    y_scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=tree.yview)
    x_scroll = ttk.Scrollbar(frame, orient=tk.HORIZONTAL, command=tree.xview)
    y_scroll.pack(side=tk.RIGHT, fill=tk.Y)
    x_scroll.pack(side=tk.BOTTOM, fill=tk.X)
    tree.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)

    headers = rows[0]
    tree["columns"] = headers
    for header in headers:
        tree.heading(header, text=header)
        tree.column(header, width=max(140, len(header) * 10), stretch=tk.YES, anchor="w")

    for idx, row in enumerate(rows[1:]):
        padded = list(row)
        while len(padded) < len(headers):
            padded.append("")
        if len(padded) > len(headers):
            padded = padded[:len(headers)]
        tag = "even" if idx % 2 == 0 else "odd"
        tree.insert("", tk.END, values=padded, tags=(tag,))

    tree.tag_configure("even", background=palette["tree_bg"])
    tree.tag_configure("odd", background=palette["alt_row"])

    for child in tree.get_children():
        values = tree.item(child, "values")
        for col_index, value in enumerate(values):
            color = color_for_value(value, palette)
            tree.item(child, values=tree.item(child, "values"))
            tree.tag_bind(child, "<Button-1>", lambda e, c=child: None)
            tree.set(child, headers[col_index], value)

    toolbar = ttk.Frame(frame)
    toolbar.pack(fill=tk.X, pady=(8, 0))
    ttk.Label(toolbar, text=f"Rows: {len(rows)}  •  Theme: {theme}", foreground=palette["fg"]).pack(side=tk.LEFT, padx=(0, 12))
    ttk.Button(toolbar, text="Copy selected", command=lambda: copy_selected(tree, root)).pack(side=tk.RIGHT)
    ttk.Button(toolbar, text="Close", command=root.destroy).pack(side=tk.RIGHT, padx=(0, 8))

    root.mainloop()


def copy_selected(tree, root):
    text = build_copy_text(tree)
    if not text:
        return
    root.clipboard_clear()
    root.clipboard_append(text)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        sys.stderr.write(f"gridview_gui: {exc}\n")
        sys.exit(1)
