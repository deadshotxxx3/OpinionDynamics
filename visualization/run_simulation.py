import os
import sys
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox, filedialog


BINARY_PATH_DEFAULT = "../build/opinion_dynamics"
VISUALIZE_SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "visualize.py")
VISUALIZE_CHECK_DELAY_MS = 3000
LABEL_WIDTH = 28

EDGE_FUNCTIONS = [
    ("constant", "Константная", "p = a", "a", 1, "0.01"),
    ("linear", "Линейная", "p = a + b·t", "a, b", 2, "0.01, 0.0001"),
    ("polynomial", "Полиномиальная", "p = c0 + c1·t + c2·t² + ...", "c0, c1, c2, ...", -1,
     "0.001, 0, 0.000001"),
    ("exponential", "Экспоненциальная", "p = a·e^(b·t)", "a, b", 2, "0.001, 0.02"),
    ("saturation", "Насыщение", "p = pmax·(1 − e^(−r·t))", "pmax, r", 2, "0.02, 0.05"),
    ("logistic", "Логистическая", "p = pmax / (1 + e^(−r·(t − t0)))", "pmax, r, t0", 3,
     "0.02, 0.1, 50"),
    ("step", "Ступенчатая", "p = a при t < T, иначе b", "a, b, T", 3, "0.001, 0.02, 100"),
    ("periodic", "Периодическая", "p = a + b·sin(w·t)", "a, b, w", 3, "0.01, 0.005, 0.1"),
]

FUNCTION_BY_DISPLAY = {display: (name, hint, count, default)
                       for name, display, _, hint, count, default in EDGE_FUNCTIONS}

FORMULA_BY_DISPLAY = {display: formula for _, display, formula, _, _, _ in EDGE_FUNCTIONS}

COMBOBOX_WIDTH = max(len(display) for _, display, _, _, _, _ in EDGE_FUNCTIONS) + 2


class SimulationForm:
    def __init__(self, root):
        self.root = root
        self.root.title("Opinion Dynamics — запуск симуляции")
        self.root.geometry("680x820")
        self.root.minsize(560, 480)

        self.fields = {}
        self.level_rows = []
        self.stubborn_mode_vars = {}
        self.function_vars = {}
        self.function_hints = {}

        self.dynamic_edges_var = tk.BooleanVar()
        self.remove_edges_var = tk.BooleanVar()
        self.visualize_var = tk.BooleanVar(value=True)
        self.source_var = tk.StringVar(value="generate")
        self.load_graph_var = tk.StringVar(value="initial")
        self.use_weights_var = tk.BooleanVar(value=False)
        self.load_weights_var = tk.StringVar(value="file")

        self.generate_frame = None
        self.load_frame = None
        self.dynamic_frame = None

        self.build_bottom_bar()
        self.build_scrollable_area()
        self.build_source_section()
        self.build_generate_section()
        self.build_load_section()
        self.build_dynamic_edges_section()
        self.build_binary_section()

        self.on_source_changed()

        self.root.bind("<Control-Return>", lambda e: self.on_run_clicked())
        self.root.bind("<Escape>", lambda e: self.root.destroy())

    def build_bottom_bar(self):
        bar = ttk.Frame(self.root)
        bar.pack(side="bottom", fill="x")

        ttk.Separator(bar, orient="horizontal").pack(fill="x")

        inner = ttk.Frame(bar)
        inner.pack(fill="x", padx=10, pady=8)

        ttk.Checkbutton(
            inner,
            text="Показать визуализацию после запуска",
            variable=self.visualize_var,
        ).pack(side="left")

        ttk.Button(inner, text="Запустить (Ctrl+Enter)", command=self.on_run_clicked).pack(
            side="right"
        )

    def build_scrollable_area(self):
        container = ttk.Frame(self.root)
        container.pack(side="top", fill="both", expand=True)

        self.canvas = tk.Canvas(container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(container, orient="vertical", command=self.canvas.yview)
        self.scrollable_parent = ttk.Frame(self.canvas)

        self.canvas_window = self.canvas.create_window(
            (0, 0), window=self.scrollable_parent, anchor="nw"
        )

        self.scrollable_parent.bind(
            "<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")),
        )
        self.canvas.bind(
            "<Configure>",
            lambda e: self.canvas.itemconfig(self.canvas_window, width=e.width),
        )

        self.canvas.configure(yscrollcommand=scrollbar.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        self.canvas.bind_all("<MouseWheel>", self._wheel_windows)
        self.canvas.bind_all("<Button-4>", self._wheel_linux)
        self.canvas.bind_all("<Button-5>", self._wheel_linux)

    def _is_inside_dropdown(self, event):
        widget = event.widget
        if isinstance(widget, str):
            return True
        return widget.winfo_class() in ("Listbox", "TCombobox", "ComboboxPopdown")

    def _wheel_windows(self, event):
        if self._is_inside_dropdown(event):
            return
        self.canvas.yview_scroll(int(-event.delta / 120), "units")

    def _wheel_linux(self, event):
        if self._is_inside_dropdown(event):
            return
        if event.num == 4:
            self.canvas.yview_scroll(-1, "units")
        elif event.num == 5:
            self.canvas.yview_scroll(1, "units")

    def build_source_section(self):
        frame = ttk.LabelFrame(self.scrollable_parent, text="Источник графа")
        frame.pack(fill="x", padx=10, pady=6)

        ttk.Radiobutton(
            frame,
            text="Сгенерировать новый граф",
            variable=self.source_var,
            value="generate",
            command=self.on_source_changed,
        ).pack(anchor="w", padx=6, pady=1)

        ttk.Radiobutton(
            frame,
            text="Загрузить из папки прогона",
            variable=self.source_var,
            value="load",
            command=self.on_source_changed,
        ).pack(anchor="w", padx=6, pady=1)

    def build_levels_frame(self, parent):
        levels_frame = ttk.LabelFrame(parent, text="Уровни")
        levels_frame.pack(fill="x", padx=10, pady=6)

        header = ttk.Frame(levels_frame)
        header.pack(fill="x", padx=6, pady=(4, 2))
        ttk.Label(header, text="№", width=4).pack(side="left")
        ttk.Label(header, text="Вершин", width=12).pack(side="left", padx=2)
        ttk.Label(header, text="Вероятность", width=14).pack(side="left", padx=2)

        self.levels_container = ttk.Frame(levels_frame)
        self.levels_container.pack(fill="x", padx=6, pady=2)

        buttons = ttk.Frame(levels_frame)
        buttons.pack(fill="x", padx=6, pady=(2, 6))
        ttk.Button(
            buttons, text="+ Добавить уровень", command=self.add_level_row
        ).pack(side="left")

        self.add_level_row(50, "0.3")

    def build_opinion_frame(self, parent):
        basics = ttk.LabelFrame(parent, text="Параметры мнений")
        basics.pack(fill="x", padx=10, pady=6)

        self.add_entry(basics, "gen_k1", "Порог k1", "0.05")
        self.add_entry(basics, "gen_k2", "Порог k2", "0.5")
        self.add_entry(basics, "gen_tmax", "Количество шагов", "500")
        self.add_entry(basics, "gen_seed", "Seed (пусто = случайный)", "")

        ttk.Checkbutton(
            basics, text="Учитывать веса рёбер", variable=self.use_weights_var
        ).pack(anchor="w", padx=6, pady=2)

    def build_stubborn_frame(self, parent):
        stubborn_frame = ttk.LabelFrame(parent, text="Упрямые вершины")
        stubborn_frame.pack(fill="x", padx=10, pady=6)

        self.build_stubborn_group(stubborn_frame, "attach", "Прикрепить новые (степень 1)", "5")
        self.build_stubborn_group(stubborn_frame, "assign", "Назначить существующие", "0")

    def build_stubborn_group(self, parent, prefix, title, default_count):
        frame = ttk.LabelFrame(parent, text=title)
        frame.pack(fill="x", padx=6, pady=4)

        self.add_entry(frame, f"{prefix}_count", "Количество (0 = нет)", default_count)

        mode_var = tk.StringVar(value="random")
        self.stubborn_mode_vars[prefix] = mode_var

        row = ttk.Frame(frame)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Выбор вершин", width=LABEL_WIDTH).pack(side="left")
        ttk.Radiobutton(row, text="Случайно", variable=mode_var, value="random").pack(side="left")
        ttk.Radiobutton(row, text="Вручную", variable=mode_var, value="manual").pack(
            side="left", padx=(8, 0)
        )

        self.add_entry(frame, f"{prefix}_targets", "Вершины вручную (через запятую)", "")

    def build_generate_section(self):
        outer = ttk.Frame(self.scrollable_parent)

        self.build_levels_frame(outer)
        self.build_opinion_frame(outer)
        self.build_stubborn_frame(outer)

        self.generate_frame = outer

    def build_load_section(self):
        outer = ttk.Frame(self.scrollable_parent)

        load_frame = ttk.LabelFrame(outer, text="Загрузка из папки прогона")
        load_frame.pack(fill="x", padx=10, pady=6)

        row = ttk.Frame(load_frame)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Папка прогона", width=LABEL_WIDTH).pack(side="left")
        entry = ttk.Entry(row)
        entry.insert(0, "../build/runs/run_42")
        entry.pack(side="left", fill="x", expand=True)
        self.fields["load_dir"] = entry
        ttk.Button(row, text="...", width=3, command=self.browse_load_dir).pack(
            side="left", padx=2
        )

        graph_choice = ttk.Frame(load_frame)
        graph_choice.pack(fill="x", padx=6, pady=2)
        ttk.Label(graph_choice, text="Какой граф", width=LABEL_WIDTH).pack(side="left")
        ttk.Radiobutton(
            graph_choice, text="Начальный", variable=self.load_graph_var, value="initial"
        ).pack(side="left")
        ttk.Radiobutton(
            graph_choice, text="Конечный", variable=self.load_graph_var, value="final"
        ).pack(side="left")

        ttk.Label(
            load_frame,
            text=(
                "Параметры берутся из секции PARAMS файла.\n"
                "Пустое поле ниже означает «взять из файла»."
            ),
            foreground="gray",
        ).pack(anchor="w", padx=6, pady=(4, 2))

        override_frame = ttk.LabelFrame(outer, text="Переопределить параметры")
        override_frame.pack(fill="x", padx=10, pady=6)

        self.add_entry(override_frame, "load_k1", "Порог k1", "")
        self.add_entry(override_frame, "load_k2", "Порог k2", "")
        self.add_entry(override_frame, "load_tmax", "Количество шагов", "")
        self.add_entry(override_frame, "load_seed", "Seed", "")

        weights_row = ttk.Frame(override_frame)
        weights_row.pack(fill="x", padx=6, pady=2)
        ttk.Label(weights_row, text="Веса рёбер", width=LABEL_WIDTH).pack(side="left")
        ttk.Radiobutton(
            weights_row, text="Из файла", variable=self.load_weights_var, value="file"
        ).pack(side="left")
        ttk.Radiobutton(
            weights_row, text="Учитывать", variable=self.load_weights_var, value="on"
        ).pack(side="left")
        ttk.Radiobutton(
            weights_row, text="Не учитывать", variable=self.load_weights_var, value="off"
        ).pack(side="left")

        self.load_frame = outer

    def build_dynamic_edges_section(self):
        frame = ttk.LabelFrame(self.scrollable_parent, text="Динамика рёбер")
        frame.pack(fill="x", padx=10, pady=6)

        self.build_function_group(
            frame, "add", "Появление рёбер", self.dynamic_edges_var, default_name="linear")
        self.build_function_group(
            frame, "remove", "Удаление рёбер", self.remove_edges_var, default_name="constant")

        self.dynamic_frame = frame

    def build_function_group(self, parent, prefix, title, enabled_var, default_name):
        group = ttk.LabelFrame(parent, text=title)
        group.pack(fill="x", padx=6, pady=4)

        ttk.Checkbutton(group, text="Включить", variable=enabled_var).pack(anchor="w", padx=6)

        displays = [display for _, display, _, _, _, _ in EDGE_FUNCTIONS]
        default_display = next(d for n, d, _, _, _, _ in EDGE_FUNCTIONS if n == default_name)

        row = ttk.Frame(group)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Функция", width=LABEL_WIDTH).pack(side="left")
        function_var = tk.StringVar(value=default_display)
        combo = ttk.Combobox(
            row,
            textvariable=function_var,
            values=displays,
            state="readonly",
            width=COMBOBOX_WIDTH,
            height=len(displays),
        )
        combo.pack(side="left")
        self.function_vars[prefix] = function_var

        for sequence in ("<MouseWheel>", "<Button-4>", "<Button-5>"):
            combo.bind(sequence, lambda e: "break")

        formula_row = ttk.Frame(group)
        formula_row.pack(fill="x", padx=6, pady=2)
        ttk.Label(formula_row, text="Формула", width=LABEL_WIDTH).pack(side="left")
        hint = ttk.Label(formula_row, text="")
        hint.pack(side="left", anchor="w")
        self.function_hints[prefix] = hint

        self.add_entry(group, f"{prefix}_params", "Параметры (через запятую)", "")

        combo.bind("<<ComboboxSelected>>", lambda e, p=prefix: self.on_function_changed(p))
        self.on_function_changed(prefix)

    def on_function_changed(self, prefix):
        display = self.function_vars[prefix].get()
        _, hint, _, default = FUNCTION_BY_DISPLAY[display]
        self.function_hints[prefix].config(
            text=f"{FORMULA_BY_DISPLAY[display]}    (параметры: {hint})"
        )
        entry = self.fields[f"{prefix}_params"]
        entry.delete(0, "end")
        entry.insert(0, default)

    def build_binary_section(self):
        frame = ttk.LabelFrame(self.scrollable_parent, text="Бинарник")
        frame.pack(fill="x", padx=10, pady=6)

        row = ttk.Frame(frame)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Путь к opinion_dynamics", width=LABEL_WIDTH).pack(side="left")
        entry = ttk.Entry(row)
        entry.insert(0, BINARY_PATH_DEFAULT)
        entry.pack(side="left", fill="x", expand=True)
        self.fields["binary_path"] = entry
        ttk.Button(row, text="...", width=3, command=self.browse_binary).pack(
            side="left", padx=2
        )

    def add_entry(self, parent, key, label, default_value):
        row = ttk.Frame(parent)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text=label, width=LABEL_WIDTH).pack(side="left")
        entry = ttk.Entry(row)
        entry.insert(0, default_value)
        entry.pack(side="left", fill="x", expand=True)
        self.fields[key] = entry

    def add_level_row(self, default_vertices="", default_probability=""):
        row = ttk.Frame(self.levels_container)
        row.pack(fill="x", pady=1)

        index_label = ttk.Label(row, text=str(len(self.level_rows) + 1), width=4)
        index_label.pack(side="left")

        vertices_entry = ttk.Entry(row, width=12)
        vertices_entry.insert(0, str(default_vertices))
        vertices_entry.pack(side="left", padx=2)

        probability_entry = ttk.Entry(row, width=14)
        probability_entry.insert(0, str(default_probability))
        probability_entry.pack(side="left", padx=2)

        ttk.Button(
            row, text="X", width=3,
            command=lambda r=row: self.remove_level_row(r),
        ).pack(side="left", padx=2)

        self.level_rows.append({
            "frame": row,
            "index_label": index_label,
            "vertices": vertices_entry,
            "probability": probability_entry,
        })

    def remove_level_row(self, row_frame):
        if len(self.level_rows) <= 1:
            messagebox.showwarning("Нельзя удалить", "Должен остаться хотя бы один уровень")
            return
        for entry in self.level_rows:
            if entry["frame"] == row_frame:
                entry["frame"].destroy()
                self.level_rows.remove(entry)
                break
        self.renumber_levels()

    def renumber_levels(self):
        for i, entry in enumerate(self.level_rows):
            entry["index_label"].config(text=str(i + 1))

    def browse_load_dir(self):
        current = self.get_value("load_dir")
        initial_dir = current if os.path.isdir(current) else "."
        chosen = filedialog.askdirectory(title="Выберите папку прогона", initialdir=initial_dir)
        if chosen:
            self.fields["load_dir"].delete(0, "end")
            self.fields["load_dir"].insert(0, chosen)

    def browse_binary(self):
        chosen = filedialog.askopenfilename(
            title="Выберите бинарник opinion_dynamics",
            initialdir=os.path.dirname(self.get_value("binary_path")) or ".",
        )
        if chosen:
            self.fields["binary_path"].delete(0, "end")
            self.fields["binary_path"].insert(0, chosen)

    def on_source_changed(self):
        self.generate_frame.pack_forget()
        self.load_frame.pack_forget()

        if self.source_var.get() == "generate":
            self.generate_frame.pack(fill="x", before=self.dynamic_frame)
        else:
            self.load_frame.pack(fill="x", before=self.dynamic_frame)

        self.canvas.yview_moveto(0.0)

    def get_value(self, key):
        entry = self.fields.get(key)
        if entry is None:
            return ""
        return entry.get().strip()

    def resolve_binary_path(self):
        raw = self.get_value("binary_path")
        if not raw:
            raise ValueError("Путь к бинарнику не может быть пустым")

        if os.path.isabs(raw):
            if os.path.isfile(raw):
                return os.path.abspath(raw)
            raise ValueError(f"Бинарник не найден: {raw}")

        script_dir = os.path.dirname(os.path.abspath(__file__))
        candidates = [
            os.path.abspath(os.path.join(script_dir, raw)),
            os.path.abspath(raw),
        ]
        for candidate in candidates:
            if os.path.isfile(candidate):
                return candidate

        raise ValueError("Бинарник не найден. Проверено:\n  " + "\n  ".join(candidates))

    def on_run_clicked(self):
        try:
            binary_abs = self.resolve_binary_path()
            args, seed = self.build_command(binary_abs)
        except ValueError as e:
            messagebox.showerror("Ошибка ввода", str(e))
            return

        project_root = os.path.dirname(binary_abs)

        try:
            result = subprocess.run(
                args, check=True, cwd=project_root, capture_output=True, text=True
            )
        except subprocess.CalledProcessError as e:
            messagebox.showerror(
                "Ошибка выполнения",
                f"Код возврата: {e.returncode}\n\nstdout:\n{e.stdout}\n\nstderr:\n{e.stderr}",
            )
            return
        except FileNotFoundError:
            messagebox.showerror("Ошибка", f"Бинарник не найден: {binary_abs}")
            return

        run_dir = self.detect_run_dir(project_root, seed, result.stdout)
        if run_dir is None:
            messagebox.showerror(
                "Ошибка", f"Не удалось определить папку прогона.\n\nstdout:\n{result.stdout}"
            )
            return

        messagebox.showinfo("Готово", f"Симуляция завершена.\nРезультат: {run_dir}")

        if self.visualize_var.get():
            self.show_visualization(run_dir)

    def detect_run_dir(self, project_root, seed, stdout):
        if seed is not None:
            candidate = os.path.join(project_root, "runs", f"run_{seed}")
            if os.path.isdir(candidate):
                return candidate

        for line in stdout.splitlines():
            line = line.strip()
            if line.startswith("Папка прогона:"):
                path = line.split(":", 1)[1].strip()
                if not os.path.isabs(path):
                    path = os.path.join(project_root, path)
                if os.path.isdir(path):
                    return os.path.abspath(path)

        runs_dir = os.path.join(project_root, "runs")
        if os.path.isdir(runs_dir):
            subdirs = [d for d in os.listdir(runs_dir) if d.startswith("run_")]
            if subdirs:
                return max(
                    (os.path.join(runs_dir, d) for d in subdirs),
                    key=os.path.getmtime,
                )
        return None

    def build_command(self, binary_abs):
        args = [binary_abs, "--batch"]

        if self.source_var.get() == "generate":
            args += self.build_generate_args()
            seed = self.parse_seed_optional("gen_seed")
        else:
            args += self.build_load_args()
            seed = self.parse_seed_optional("load_seed")

        if seed is not None:
            args += ["--seed", str(seed)]

        args += self.build_dynamic_edges_args()
        return args, seed

    def parse_function_params(self, prefix, title):
        name, hint, expected, _ = FUNCTION_BY_DISPLAY[self.function_vars[prefix].get()]
        raw = self.get_value(f"{prefix}_params")
        values = [v.strip() for v in raw.split(",") if v.strip()]

        for v in values:
            try:
                float(v)
            except ValueError:
                raise ValueError(f"{title}: параметры должны быть числами через запятую")

        if expected == -1 and len(values) < 1:
            raise ValueError(f"{title}: нужен хотя бы один коэффициент ({hint})")
        if expected != -1 and len(values) != expected:
            raise ValueError(
                f"{title}: нужно параметров {expected} ({hint}), указано {len(values)}"
            )

        return name, ",".join(values)

    def build_dynamic_edges_args(self):
        if not self.dynamic_edges_var.get():
            return []

        name, params = self.parse_function_params("add", "Появление рёбер")
        args = ["--dynamic-edges", "--add-func", name, "--add-params", params]

        if self.remove_edges_var.get():
            name, params = self.parse_function_params("remove", "Удаление рёбер")
            args += ["--remove-edges", "--remove-func", name, "--remove-params", params]

        return args

    def build_levels_args(self):
        vertices_list = []
        probabilities_list = []

        for i, row in enumerate(self.level_rows):
            v = row["vertices"].get().strip()
            p = row["probability"].get().strip()
            if not v or not p:
                raise ValueError(f"Уровень {i + 1}: заполните оба поля")
            try:
                int(v)
                float(p)
            except ValueError:
                raise ValueError(f"Уровень {i + 1}: вершин — целое число, вероятность — дробное")
            vertices_list.append(v)
            probabilities_list.append(p)

        return [
            "--level-vertices", ",".join(vertices_list),
            "--level-probabilities", ",".join(probabilities_list),
        ]

    def total_level_vertices(self):
        total = 0
        for row in self.level_rows:
            value = row["vertices"].get().strip()
            if value:
                total += int(value)
        return total

    def parse_stubborn_targets(self, prefix, title, count, unique, total_vertices):
        targets_str = self.get_value(f"{prefix}_targets")
        if not targets_str:
            raise ValueError(f"{title}: выбран ручной режим, но не заданы вершины")

        targets = [t.strip() for t in targets_str.split(",") if t.strip()]
        if len(targets) != count:
            raise ValueError(f"{title}: указано {len(targets)} вершин, а количество {count}")

        for t in targets:
            try:
                index = int(t)
            except ValueError:
                raise ValueError(f"{title}: вершины должны быть целыми числами через запятую")
            if index < 0 or index >= total_vertices:
                raise ValueError(
                    f"{title}: вершины {index} нет в графе. "
                    f"Допустимые номера: 0..{total_vertices - 1}"
                )

        if unique and len(set(targets)) != len(targets):
            raise ValueError(f"{title}: вершины не должны повторяться")

        return targets

    def build_stubborn_group_args(self, prefix, title, unique, total_vertices):
        count_str = self.get_value(f"{prefix}_count")
        targets_str = self.get_value(f"{prefix}_targets")
        mode = self.stubborn_mode_vars[prefix].get()

        try:
            count = int(count_str) if count_str else 0
        except ValueError:
            raise ValueError(f"{title}: количество должно быть целым числом")
        if count < 0:
            raise ValueError(f"{title}: количество не может быть отрицательным")

        if count == 0:
            if mode == "manual" and targets_str:
                raise ValueError(
                    f"{title}: указаны вершины, но количество 0. "
                    "Укажите количество, равное числу вершин в списке"
                )
            return []

        args = [f"--stubborn-{prefix}", str(count), f"--stubborn-{prefix}-mode", mode]

        if mode == "manual":
            targets = self.parse_stubborn_targets(prefix, title, count, unique, total_vertices)
            args += [f"--stubborn-{prefix}-targets", ",".join(targets)]

        return args

    def build_generate_args(self):
        args = self.build_levels_args()
        total_vertices = self.total_level_vertices()

        args += self.build_stubborn_group_args(
            "attach", "Прикрепить новые", unique=False, total_vertices=total_vertices)
        args += self.build_stubborn_group_args(
            "assign", "Назначить существующие", unique=True, total_vertices=total_vertices)

        k1 = self.get_value("gen_k1")
        k2 = self.get_value("gen_k2")
        tmax = self.get_value("gen_tmax")
        if not k1 or not k2 or not tmax:
            raise ValueError("Заполните k1, k2 и количество шагов")

        args += ["--k1", k1, "--k2", k2, "--tmax", tmax]

        if self.use_weights_var.get():
            args.append("--use-weights")

        return args

    def resolve_load_dir(self):
        load_dir = self.get_value("load_dir")
        if not load_dir:
            raise ValueError("Укажите папку прогона")

        if not os.path.isabs(load_dir):
            script_dir = os.path.dirname(os.path.abspath(__file__))
            candidate = os.path.abspath(os.path.join(script_dir, load_dir))
            if not os.path.isdir(candidate):
                candidate = os.path.abspath(load_dir)
            load_dir = candidate

        if not os.path.isdir(load_dir):
            raise ValueError(f"Папка не найдена: {load_dir}")

        return load_dir

    def build_load_args(self):
        args = ["--load-dir", self.resolve_load_dir(), "--load-graph", self.load_graph_var.get()]

        for key, flag in (("load_k1", "--k1"), ("load_k2", "--k2"), ("load_tmax", "--tmax")):
            value = self.get_value(key)
            if value:
                args += [flag, value]

        weights_mode = self.load_weights_var.get()
        if weights_mode == "on":
            args.append("--use-weights")
        elif weights_mode == "off":
            args.append("--no-weights")

        return args

    def parse_seed_optional(self, key):
        seed_str = self.get_value(key)
        if not seed_str:
            return None
        try:
            return int(seed_str)
        except ValueError:
            raise ValueError("Seed должен быть целым числом")

    def show_visualization(self, run_dir):
        initial_graph_path = os.path.join(run_dir, "graph_initial.txt")
        log_path = os.path.join(run_dir, "simulation_log.txt")

        for path in (initial_graph_path, log_path):
            if not os.path.isfile(path):
                messagebox.showerror("Ошибка визуализации", f"Нет файла {path}")
                return

        try:
            process = subprocess.Popen(
                [sys.executable, VISUALIZE_SCRIPT, run_dir],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
        except OSError as e:
            messagebox.showerror("Ошибка визуализации", f"Не удалось запустить visualize.py:\n{e}")
            return

        self.root.after(VISUALIZE_CHECK_DELAY_MS, lambda: self.check_visualization(process))

    def check_visualization(self, process):
        if process.poll() is None or process.returncode == 0:
            return

        output, _ = process.communicate()
        lines = output.strip().splitlines()
        message = "\n".join(lines[-15:]) if lines else f"Код возврата: {process.returncode}"
        messagebox.showerror("Ошибка визуализации", message)


def main():
    root = tk.Tk()
    SimulationForm(root)
    root.mainloop()


if __name__ == "__main__":
    main()