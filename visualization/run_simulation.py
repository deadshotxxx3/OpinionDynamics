import os
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

from visualize import (
    parse_graph,
    parse_log,
    run_graph_viewer,
    run_dynamics_only,
    build_states,
    MAX_VISUAL_VERTICES,
)


BINARY_PATH_DEFAULT = "../build/opinion_dynamics"


class SimulationForm:
    def __init__(self, root):
        self.root = root
        self.root.title("Opinion Dynamics — запуск симуляции")
        self.root.geometry("680x820")
        self.root.minsize(560, 480)

        self.fields = {}
        self.level_rows = []
        self.dynamic_edges_var = tk.BooleanVar()
        self.remove_edges_var = tk.BooleanVar()
        self.stubborn_mode_var = tk.StringVar(value="random")
        self.visualize_var = tk.BooleanVar(value=True)
        self.source_var = tk.StringVar(value="generate")
        self.load_graph_var = tk.StringVar(value="initial")

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

    def _wheel_windows(self, event):
        self.canvas.yview_scroll(int(-event.delta / 120), "units")

    def _wheel_linux(self, event):
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

    def build_generate_section(self):
        outer = ttk.Frame(self.scrollable_parent)

        levels_frame = ttk.LabelFrame(outer, text="Уровни")
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

        basics = ttk.LabelFrame(outer, text="Параметры мнений")
        basics.pack(fill="x", padx=10, pady=6)

        self.add_entry(basics, "gen_k1", "Порог k1", "0.05")
        self.add_entry(basics, "gen_k2", "Порог k2", "0.5")
        self.add_entry(basics, "gen_tmax", "Количество шагов", "500")
        self.add_entry(basics, "gen_seed", "Seed (пусто = случайный)", "")

        stubborn_frame = ttk.LabelFrame(outer, text="Упрямые вершины")
        stubborn_frame.pack(fill="x", padx=10, pady=6)

        self.add_entry(stubborn_frame, "stubborn", "Количество упрямых (0 = нет)", "5")

        mode_frame = ttk.Frame(stubborn_frame)
        mode_frame.pack(fill="x", padx=6, pady=2)
        ttk.Radiobutton(
            mode_frame, text="Случайно", variable=self.stubborn_mode_var, value="random"
        ).pack(side="left")
        ttk.Radiobutton(
            mode_frame, text="Вручную", variable=self.stubborn_mode_var, value="manual"
        ).pack(side="left")

        self.add_entry(
            stubborn_frame, "stubborn_targets", "Цели вручную (через запятую)", ""
        )

        self.generate_frame = outer

    def build_load_section(self):
        outer = ttk.Frame(self.scrollable_parent)

        load_frame = ttk.LabelFrame(outer, text="Загрузка из папки прогона")
        load_frame.pack(fill="x", padx=10, pady=6)

        row = ttk.Frame(load_frame)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Папка прогона", width=28).pack(side="left")
        entry = ttk.Entry(row)
        entry.insert(0, "../build/runs/run_42")
        entry.pack(side="left", fill="x", expand=True)
        self.fields["load_dir"] = entry
        ttk.Button(row, text="...", width=3, command=self.browse_load_dir).pack(
            side="left", padx=2
        )

        graph_choice = ttk.Frame(load_frame)
        graph_choice.pack(fill="x", padx=6, pady=2)
        ttk.Label(graph_choice, text="Какой граф", width=28).pack(side="left")
        ttk.Radiobutton(
            graph_choice, text="Начальный", variable=self.load_graph_var, value="initial"
        ).pack(side="left")
        ttk.Radiobutton(
            graph_choice, text="Конечный", variable=self.load_graph_var, value="final"
        ).pack(side="left")

        hint = ttk.Label(
            load_frame,
            text=(
                "Параметры берутся из секции PARAMS файла.\n"
                "Пустое поле ниже означает «взять из файла»."
            ),
            foreground="gray",
        )
        hint.pack(anchor="w", padx=6, pady=(4, 2))

        override_frame = ttk.LabelFrame(outer, text="Переопределить параметры")
        override_frame.pack(fill="x", padx=10, pady=6)

        self.add_entry(override_frame, "load_k1", "Порог k1", "")
        self.add_entry(override_frame, "load_k2", "Порог k2", "")
        self.add_entry(override_frame, "load_tmax", "Количество шагов", "")
        self.add_entry(override_frame, "load_seed", "Seed", "")

        self.load_frame = outer

    def build_dynamic_edges_section(self):
        frame = ttk.LabelFrame(self.scrollable_parent, text="Динамика рёбер")
        frame.pack(fill="x", padx=10, pady=6)

        ttk.Checkbutton(
            frame, text="Добавление рёбер", variable=self.dynamic_edges_var
        ).pack(anchor="w", padx=6)

        self.add_entry(frame, "p0", "p0 (добавление)", "0.05")
        self.add_entry(frame, "k", "k (добавление)", "0.001")

        ttk.Checkbutton(
            frame, text="Удаление рёбер", variable=self.remove_edges_var
        ).pack(anchor="w", padx=6, pady=(6, 0))

        self.add_entry(frame, "remove_p0", "p0 (удаление)", "0.0")
        self.add_entry(frame, "remove_k", "k (удаление)", "0.0")

        self.dynamic_frame = frame

    def build_binary_section(self):
        frame = ttk.LabelFrame(self.scrollable_parent, text="Бинарник")
        frame.pack(fill="x", padx=10, pady=6)

        row = ttk.Frame(frame)
        row.pack(fill="x", padx=6, pady=2)
        ttk.Label(row, text="Путь к opinion_dynamics", width=28).pack(side="left")
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
        ttk.Label(row, text=label, width=28).pack(side="left")
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
        chosen = filedialog.askdirectory(
            title="Выберите папку прогона",
            initialdir=initial_dir,
        )
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
        if self.generate_frame is not None:
            self.generate_frame.pack_forget()
        if self.load_frame is not None:
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
        for c in candidates:
            if os.path.isfile(c):
                return c

        raise ValueError(
            "Бинарник не найден. Проверено:\n  " + "\n  ".join(candidates)
        )

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
                args,
                check=True,
                cwd=project_root,
                capture_output=True,
                text=True,
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
                "Ошибка",
                f"Не удалось определить папку прогона.\n\nstdout:\n{result.stdout}",
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
        seed = None

        if self.source_var.get() == "generate":
            args += self.build_generate_args()
            seed = self.parse_seed_optional("gen_seed")
            if seed is not None:
                args += ["--seed", str(seed)]
        else:
            args += self.build_load_args()
            seed = self.parse_seed_optional("load_seed")
            if seed is not None:
                args += ["--seed", str(seed)]

        if self.dynamic_edges_var.get():
            args += ["--dynamic-edges"]
            p0 = self.get_value("p0")
            k = self.get_value("k")
            if p0:
                args += ["--p0", p0]
            if k:
                args += ["--k", k]

            if self.remove_edges_var.get():
                args += ["--remove-edges"]
                rp0 = self.get_value("remove_p0")
                rk = self.get_value("remove_k")
                if rp0:
                    args += ["--remove-p0", rp0]
                if rk:
                    args += ["--remove-k", rk]

        return args, seed

    def build_generate_args(self):
        args = []

        vertices_list = []
        probabilities_list = []

        for i, row in enumerate(self.level_rows):
            v = row["vertices"].get().strip()
            p = row["probability"].get().strip()
            if not v or not p:
                raise ValueError(f"Уровень {i+1}: заполните оба поля")
            try:
                int(v)
                float(p)
            except ValueError:
                raise ValueError(
                    f"Уровень {i+1}: вершин — целое число, вероятность — дробное"
                )
            vertices_list.append(v)
            probabilities_list.append(p)

        args += ["--level-vertices", ",".join(vertices_list)]
        args += ["--level-probabilities", ",".join(probabilities_list)]

        stubborn = self.get_value("stubborn")
        if stubborn:
            try:
                stubborn_count = int(stubborn)
            except ValueError:
                raise ValueError("Количество упрямых должно быть целым числом")
            if stubborn_count < 0:
                raise ValueError("Количество упрямых не может быть отрицательным")

            if stubborn_count > 0:
                mode = self.stubborn_mode_var.get()
                args += ["--stubborn", str(stubborn_count), "--stubborn-mode", mode]

                if mode == "manual":
                    targets_str = self.get_value("stubborn_targets")
                    if not targets_str:
                        raise ValueError(
                            "Указан ручной режим, но не заданы цели"
                        )
                    targets = [t.strip() for t in targets_str.split(",") if t.strip()]
                    if len(targets) != stubborn_count:
                        raise ValueError(
                            f"Указано {len(targets)} целей, а упрямых {stubborn_count}. "
                            "Количество должно совпадать."
                        )
                    for t in targets:
                        try:
                            int(t)
                        except ValueError:
                            raise ValueError(
                                "Цели должны быть целыми числами через запятую"
                            )
                    args += ["--stubborn-targets", ",".join(targets)]

        k1 = self.get_value("gen_k1")
        k2 = self.get_value("gen_k2")
        tmax = self.get_value("gen_tmax")
        if not k1 or not k2 or not tmax:
            raise ValueError("Заполните k1, k2 и количество шагов")

        args += ["--k1", k1, "--k2", k2, "--tmax", tmax]
        return args

    def build_load_args(self):
        args = []

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

        args += ["--load-dir", load_dir]
        args += ["--load-graph", self.load_graph_var.get()]

        k1 = self.get_value("load_k1")
        k2 = self.get_value("load_k2")
        tmax = self.get_value("load_tmax")

        if k1:
            args += ["--k1", k1]
        if k2:
            args += ["--k2", k2]
        if tmax:
            args += ["--tmax", tmax]

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

        if not os.path.isfile(initial_graph_path):
            messagebox.showerror(
                "Ошибка визуализации", f"Нет файла {initial_graph_path}"
            )
            return
        if not os.path.isfile(log_path):
            messagebox.showerror("Ошибка визуализации", f"Нет файла {log_path}")
            return

        try:
            num_vertices, initial_edges, stubborn = parse_graph(initial_graph_path)
            log = parse_log(log_path)

            if num_vertices > MAX_VISUAL_VERTICES:
                run_dynamics_only(log)
                return

            states = build_states(num_vertices, initial_edges, stubborn, log)
            run_graph_viewer(num_vertices, stubborn, states, log["t_max"])
        except Exception as e:
            messagebox.showerror("Ошибка визуализации", str(e))


def main():
    root = tk.Tk()
    SimulationForm(root)
    root.mainloop()


if __name__ == "__main__":
    main()