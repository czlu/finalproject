#!/usr/bin/env python3
"""
Pizza Topping Scanner GUI
Reads color sensor data from micro:bit over serial and displays readings + detected ingredient.
Usage: python3 gui.py [serial_port]
"""

import sys
import re
import threading
import serial
import tkinter as tk

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbmodem102"
BAUD = 38400

# Ingredient colors
COLORS = {
    "Pepperoni": "#E74C3C",
    "Spinach":   "#27AE60",
    "Cheese":    "#F1C40F",
    "Nothing":   "#888888",
}

# Parse: V:20.2 B:33.6 G:51.5 Y:49.7 O:61.7 R:39.2 => Pepperoni
LINE_RE = re.compile(
    r"V:([\d.-]+)\s+B:([\d.-]+)\s+G:([\d.-]+)\s+Y:([\d.-]+)\s+O:([\d.-]+)\s+R:([\d.-]+)\s+=>\s+(\w+)"
)


class ScannerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Pizza Topping Scanner")
        self.root.configure(bg="#1A1A2E")
        self.root.geometry("520x720")

        # Title
        tk.Label(root, text="\U0001F355 Pizza Scanner", font=("Helvetica", 28, "bold"),
                 fg="white", bg="#1A1A2E").pack(pady=(20, 5))

        # Status
        self.status_frame = tk.Frame(root, bg="#1A1A2E")
        self.status_frame.pack(pady=(0, 10))
        self.status_dot = tk.Label(self.status_frame, text="\u25CF", font=("Helvetica", 14),
                                   fg="#555", bg="#1A1A2E")
        self.status_dot.pack(side=tk.LEFT, padx=(0, 6))
        self.status_label = tk.Label(self.status_frame, text="Connecting...",
                                     font=("Helvetica", 13), fg="#888", bg="#1A1A2E")
        self.status_label.pack(side=tk.LEFT)

        # Current ingredient display
        self.current_label = tk.Label(root, text="---", font=("Helvetica", 36, "bold"),
                                      fg="#666", bg="#16213E", pady=15)
        self.current_label.pack(padx=30, pady=(0, 10), fill=tk.X)

        # Channel bars frame
        bar_frame = tk.Frame(root, bg="#1A1A2E")
        bar_frame.pack(padx=30, fill=tk.X, pady=(0, 10))

        self.bars = {}
        channels = [
            ("V", "Violet", "#8B5CF6"),
            ("B", "Blue",   "#3B82F6"),
            ("G", "Green",  "#22C55E"),
            ("Y", "Yellow", "#EAB308"),
            ("O", "Orange", "#F97316"),
            ("R", "Red",    "#EF4444"),
        ]
        for key, name, color in channels:
            row = tk.Frame(bar_frame, bg="#1A1A2E")
            row.pack(fill=tk.X, pady=2)

            lbl = tk.Label(row, text=name, font=("Helvetica", 12, "bold"),
                           fg=color, bg="#1A1A2E", width=7, anchor="w")
            lbl.pack(side=tk.LEFT)

            canvas = tk.Canvas(row, height=20, bg="#16213E", highlightthickness=0)
            canvas.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 5))

            val_lbl = tk.Label(row, text="0.0", font=("Helvetica", 11),
                               fg="#AAA", bg="#1A1A2E", width=7, anchor="e")
            val_lbl.pack(side=tk.RIGHT)

            self.bars[key] = {"canvas": canvas, "color": color, "value_label": val_lbl}

        # Divider
        tk.Label(root, text="SCANNED TOPPINGS", font=("Helvetica", 11, "bold"),
                 fg="#555", bg="#1A1A2E").pack(pady=(10, 5))

        # Ingredient log
        self.list_frame = tk.Frame(root, bg="#1A1A2E")
        self.list_frame.pack(padx=30, fill=tk.BOTH, expand=True)

        self.canvas_list = tk.Canvas(self.list_frame, bg="#1A1A2E", highlightthickness=0)
        self.canvas_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.inner_frame = tk.Frame(self.canvas_list, bg="#1A1A2E")
        self.canvas_list.create_window((0, 0), window=self.inner_frame, anchor="nw")
        self.inner_frame.bind("<Configure>",
                              lambda e: self.canvas_list.configure(scrollregion=self.canvas_list.bbox("all")))

        # Clear button
        tk.Button(root, text="Clear All", font=("Helvetica", 13, "bold"),
                  fg="white", bg="#E74C3C", activebackground="#C0392B",
                  relief=tk.FLAT, padx=20, pady=8, command=self.clear_list).pack(pady=(10, 20))

        self.ingredients = []
        self.count = 0
        self.last_ingredient = None
        self.max_bar_value = 100.0  # auto-scales

    def set_status(self, text, color="#27AE60"):
        self.status_label.config(text=text)
        self.status_dot.config(fg=color)

    def update_reading(self, values, ingredient):
        """values = dict with keys V,B,G,Y,O,R"""
        # Update current ingredient
        color = COLORS.get(ingredient, "#888")
        self.current_label.config(text=ingredient, fg=color)

        # Auto-scale bars
        max_val = max(values.values())
        if max_val > self.max_bar_value:
            self.max_bar_value = max_val
        scale = max(self.max_bar_value, 1.0)

        # Update bars
        for key, val in values.items():
            bar = self.bars[key]
            bar["value_label"].config(text=f"{val:.1f}")
            c = bar["canvas"]
            c.delete("all")
            c.update_idletasks()
            w = c.winfo_width()
            bar_w = max(1, int((val / scale) * w))
            c.create_rectangle(0, 0, bar_w, 20, fill=bar["color"], outline="")

        # Add to ingredient list if it changed and isn't Nothing
        if ingredient != "Nothing" and ingredient != self.last_ingredient:
            self.last_ingredient = ingredient
            self.count += 1

            row = tk.Frame(self.inner_frame, bg="#16213E", pady=6, padx=10)
            row.pack(fill=tk.X, pady=2)

            tk.Label(row, text=f"#{self.count}", font=("Helvetica", 11, "bold"),
                     fg="#555", bg="#16213E", width=4, anchor="w").pack(side=tk.LEFT)
            tk.Label(row, text=ingredient, font=("Helvetica", 15, "bold"),
                     fg=color, bg="#16213E").pack(side=tk.LEFT, padx=(5, 0))

            self.ingredients.append(row)
            self.canvas_list.update_idletasks()
            self.canvas_list.yview_moveto(1.0)

        if ingredient == "Nothing":
            self.last_ingredient = None

    def clear_list(self):
        for w in self.ingredients:
            w.destroy()
        self.ingredients.clear()
        self.count = 0
        self.last_ingredient = None


def serial_reader(app):
    while True:
        try:
            with serial.Serial(PORT, BAUD, timeout=1) as ser:
                app.root.after(0, app.set_status, "Connected", "#27AE60")
                while True:
                    line = ser.readline().decode("utf-8", errors="ignore").strip()
                    m = LINE_RE.search(line)
                    if m:
                        values = {
                            "V": float(m.group(1)),
                            "B": float(m.group(2)),
                            "G": float(m.group(3)),
                            "Y": float(m.group(4)),
                            "O": float(m.group(5)),
                            "R": float(m.group(6)),
                        }
                        ingredient = m.group(7)
                        app.root.after(0, app.update_reading, values, ingredient)
        except serial.SerialException:
            app.root.after(0, app.set_status, "Disconnected — retrying...", "#E74C3C")
            import time
            time.sleep(2)


def main():
    root = tk.Tk()
    app = ScannerApp(root)
    threading.Thread(target=serial_reader, args=(app,), daemon=True).start()
    root.mainloop()


if __name__ == "__main__":
    main()
