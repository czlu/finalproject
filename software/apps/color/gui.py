#!/usr/bin/env python3
"""
Pizza Builder Game
Scan toppings with the color sensor in the correct order to build a pizza.
Usage: python3 gui.py [serial_port]
"""

import sys
import re
import random
import threading
import serial
import tkinter as tk

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbmodem102"
BAUD = 38400

# Parse: V:20.2 B:33.6 G:51.5 Y:49.7 O:61.7 R:39.2 => Pepperoni
LINE_RE = re.compile(
    r"V:([\d.-]+)\s+B:([\d.-]+)\s+G:([\d.-]+)\s+Y:([\d.-]+)\s+O:([\d.-]+)\s+R:([\d.-]+)\s+=>\s+(\w+)"
)

# Pizza definitions: name -> ordered list of required toppings
# Toppings must always be scanned in this order: Cheese, Pepperoni, Veggies
PIZZAS = {
    "Pepperoni Pizza": ["Cheese", "Pepperoni"],
    "Veggie Pizza":    ["Cheese", "Veggies"],
    "Everything Pizza": ["Cheese", "Pepperoni", "Veggies"],
}

TOPPING_COLORS = {
    "Cheese":    "#F1C40F",
    "Pepperoni": "#E74C3C",
    "Veggies":   "#27AE60",
}

TOPPING_EMOJI = {
    "Cheese":    "\U0001F9C0",
    "Pepperoni": "\U0001F355",
    "Veggies":   "\U0001F96C",
}


class PizzaGame:
    def __init__(self, root):
        self.root = root
        self.root.title("Pizza Builder")
        self.root.configure(bg="#1A1A2E")
        self.root.geometry("520x780")

        # Title
        tk.Label(root, text="\U0001F355 Pizza Builder", font=("Helvetica", 28, "bold"),
                 fg="white", bg="#1A1A2E").pack(pady=(20, 5))

        # Status
        sf = tk.Frame(root, bg="#1A1A2E")
        sf.pack(pady=(0, 10))
        self.status_dot = tk.Label(sf, text="\u25CF", font=("Helvetica", 14),
                                   fg="#555", bg="#1A1A2E")
        self.status_dot.pack(side=tk.LEFT, padx=(0, 6))
        self.status_label = tk.Label(sf, text="Connecting...",
                                     font=("Helvetica", 13), fg="#888", bg="#1A1A2E")
        self.status_label.pack(side=tk.LEFT)

        # Current scan readout
        self.scan_label = tk.Label(root, text="Sensor: ---", font=("Helvetica", 13),
                                   fg="#666", bg="#1A1A2E")
        self.scan_label.pack(pady=(0, 10))

        # Pizza order card
        self.order_frame = tk.Frame(root, bg="#16213E", highlightbackground="#333",
                                    highlightthickness=1)
        self.order_frame.pack(padx=30, pady=(0, 10), fill=tk.X)

        self.order_title = tk.Label(self.order_frame, text="", font=("Helvetica", 22, "bold"),
                                    fg="white", bg="#16213E", pady=10)
        self.order_title.pack()

        self.order_details = tk.Label(self.order_frame, text="", font=("Helvetica", 15),
                                      fg="#AAA", bg="#16213E", pady=5)
        self.order_details.pack()

        # Progress: topping slots
        self.progress_frame = tk.Frame(root, bg="#1A1A2E")
        self.progress_frame.pack(padx=30, pady=(5, 10), fill=tk.X)

        self.slot_labels = []  # will be rebuilt each round

        # Message area (try again / success)
        self.message_label = tk.Label(root, text="", font=("Helvetica", 20, "bold"),
                                      fg="#888", bg="#1A1A2E")
        self.message_label.pack(pady=(5, 10))

        # Score
        self.score_frame = tk.Frame(root, bg="#1A1A2E")
        self.score_frame.pack(pady=(0, 5))
        self.score = 0
        self.score_label = tk.Label(self.score_frame, text="Score: 0",
                                    font=("Helvetica", 16, "bold"), fg="white", bg="#1A1A2E")
        self.score_label.pack()

        # New pizza button
        tk.Button(root, text="New Pizza", font=("Helvetica", 14, "bold"),
                  fg="white", bg="#3B82F6", activebackground="#2563EB",
                  relief=tk.FLAT, padx=20, pady=8, command=self.new_round).pack(pady=(10, 15))

        # Channel bars
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
            row.pack(fill=tk.X, pady=1)
            tk.Label(row, text=name, font=("Helvetica", 10), fg=color, bg="#1A1A2E",
                     width=7, anchor="w").pack(side=tk.LEFT)
            canvas = tk.Canvas(row, height=14, bg="#16213E", highlightthickness=0)
            canvas.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 5))
            val_lbl = tk.Label(row, text="0.0", font=("Helvetica", 10),
                               fg="#AAA", bg="#1A1A2E", width=6, anchor="e")
            val_lbl.pack(side=tk.RIGHT)
            self.bars[key] = {"canvas": canvas, "color": color, "value_label": val_lbl}

        # Game state
        self.pizza_name = ""
        self.required = []
        self.step = 0
        self.done = False
        self.last_scan = "Nothing"
        self.max_bar = 100.0

        self.new_round()

    def set_status(self, text, color="#27AE60"):
        self.status_label.config(text=text)
        self.status_dot.config(fg=color)

    def new_round(self):
        self.pizza_name = random.choice(list(PIZZAS.keys()))
        self.required = PIZZAS[self.pizza_name][:]
        self.step = 0
        self.done = False
        self.last_scan = "Nothing"
        self.message_label.config(text="Scan toppings in order!", fg="#888")

        # Update order card
        self.order_title.config(text=f"\U0001F4CB  {self.pizza_name}")
        topping_text = "  \u2192  ".join(
            f"{TOPPING_EMOJI.get(t, '')} {t}" for t in self.required
        )
        self.order_details.config(text=topping_text)

        # Rebuild progress slots
        for w in self.slot_labels:
            w.destroy()
        self.slot_labels = []

        for i, topping in enumerate(self.required):
            slot = tk.Frame(self.progress_frame, bg="#16213E", padx=15, pady=10,
                            highlightbackground="#333", highlightthickness=1)
            slot.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)

            num = tk.Label(slot, text=f"Step {i+1}", font=("Helvetica", 10),
                           fg="#555", bg="#16213E")
            num.pack()

            name = tk.Label(slot, text=f"{TOPPING_EMOJI.get(topping, '')} {topping}",
                            font=("Helvetica", 14, "bold"), fg="#555", bg="#16213E")
            name.pack()

            self.slot_labels.append(slot)

    def mark_slot(self, index, success):
        if index >= len(self.slot_labels):
            return
        slot = self.slot_labels[index]
        topping = self.required[index]
        color = TOPPING_COLORS.get(topping, "#27AE60") if success else "#E74C3C"

        for widget in slot.winfo_children():
            if isinstance(widget, tk.Label):
                if widget.cget("font").startswith("Helvetica 14"):
                    widget.config(fg=color)

        slot.config(highlightbackground=color)

    def handle_scan(self, ingredient):
        # Ignore Nothing and repeats
        if ingredient == "Nothing" or self.done:
            return
        if ingredient == self.last_scan:
            return
        self.last_scan = ingredient

        if self.step < len(self.required):
            expected = self.required[self.step]
            if ingredient == expected:
                # Correct topping
                self.mark_slot(self.step, True)
                self.step += 1

                if self.step >= len(self.required):
                    # Pizza complete!
                    self.done = True
                    self.score += 1
                    self.score_label.config(text=f"Score: {self.score}")
                    self.message_label.config(
                        text=f"\u2705  {self.pizza_name} complete!",
                        fg="#27AE60"
                    )
                else:
                    next_t = self.required[self.step]
                    self.message_label.config(
                        text=f"\u2705  {ingredient}! Now scan {next_t}",
                        fg="#27AE60"
                    )
            else:
                # Wrong topping
                self.message_label.config(
                    text=f"\u274C  Wrong! Expected {expected}, got {ingredient}. Try again!",
                    fg="#E74C3C"
                )

    def update_bars(self, values):
        max_val = max(values.values())
        if max_val > self.max_bar:
            self.max_bar = max_val
        scale = max(self.max_bar, 1.0)

        for key, val in values.items():
            bar = self.bars[key]
            bar["value_label"].config(text=f"{val:.1f}")
            c = bar["canvas"]
            c.delete("all")
            c.update_idletasks()
            w = c.winfo_width()
            bar_w = max(1, int((val / scale) * w))
            c.create_rectangle(0, 0, bar_w, 14, fill=bar["color"], outline="")

    def update_reading(self, values, ingredient):
        self.scan_label.config(
            text=f"Sensor: {ingredient}",
            fg=TOPPING_COLORS.get(ingredient, "#888")
        )
        self.update_bars(values)
        self.handle_scan(ingredient)


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
    app = PizzaGame(root)
    threading.Thread(target=serial_reader, args=(app,), daemon=True).start()
    root.mainloop()


if __name__ == "__main__":
    main()
