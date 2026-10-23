import sys
import os

import matplotlib
matplotlib.use("QtAgg")

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.widgets import Slider, Button
import networkx as nx


MAX_VISUAL_VERTICES = 100
MAX_COLORED_OPINIONS = 9

INT_PARAM_KEYS = {"SEED", "T_MAX", "USE_WEIGHTS", "DYNAMIC_EDGES", "REMOVE_EDGES", "NUM_OPINIONS"}
WORD_PARAM_KEYS = {"CHOICE_RULE", "INITIAL_REGULAR", "INITIAL_STUBBORN"}
FUNCTION_PARAM_KEYS = {"ADD_FUNCTION", "REMOVE_FUNCTION", "OPINION_FUNCTION"}

BINARY_COLORS = {0: "lightgray", 1: "red"}
OTHER_COLOR = "lightgray"
PALETTE = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#bcbd22", "#17becf", "#7f7f7f",
]
STUBBORN_EDGE_COLOR = "black"

CHOICE_RULE_NAMES = {"max": "максимум", "proportional": "пропорционально"}


class TokenReader:
    def __init__(self, filepath):
        with open(filepath, "r", encoding="utf-8") as f:
            self._tokens = f.read().split()
        self._pos = 0

    def peek(self):
        if self._pos >= len(self._tokens):
            return None
        return self._tokens[self._pos]

    def next(self):
        if self._pos >= len(self._tokens):
            raise ValueError("Неожиданный конец файла")
        token = self._tokens[self._pos]
        self._pos += 1
        return token

    def next_int(self):
        return int(self.next())

    def next_float(self):
        return float(self.next())

    def expect(self, expected):
        token = self.next()
        if token != expected:
            raise ValueError(f"Ожидался токен '{expected}', получен '{token}'")


def parse_graph(filepath):
    reader = TokenReader(filepath)

    reader.expect("VERTICES")
    num_vertices = reader.next_int()

    reader.expect("STUBBORN")
    stubborn_count = reader.next_int()
    stubborn = set(reader.next_int() for _ in range(stubborn_count))

    reader.expect("EDGES")
    edge_count = reader.next_int()
    edges = {}
    for _ in range(edge_count):
        u = reader.next_int()
        v = reader.next_int()
        w = reader.next_float()
        edges[(u, v)] = w

    opinions = None
    if reader.peek() == "OPINIONS":
        reader.next()
        count = reader.next_int()
        opinions = [reader.next_int() for _ in range(count)]

    return num_vertices, edges, stubborn, opinions


def read_function(reader):
    name = reader.next()
    count = reader.next_int()
    params = [reader.next_float() for _ in range(count)]
    return {"name": name, "params": params}


def parse_log_params(reader):
    params = {}
    while reader.peek() != "INITIAL_GRAPH_FILE":
        if reader.peek() is None:
            raise ValueError("В логе нет секции INITIAL_GRAPH_FILE")

        key = reader.next()
        if key in FUNCTION_PARAM_KEYS:
            params[key.lower()] = read_function(reader)
        elif key in INT_PARAM_KEYS:
            params[key.lower()] = reader.next_int()
        elif key in WORD_PARAM_KEYS:
            params[key.lower()] = reader.next()
        elif key == "POPULARITY":
            params["popularity"] = [reader.next_int() for _ in range(reader.next_int())]
        elif key == "INITIAL_MANUAL":
            count = reader.next_int()
            params["initial_manual"] = [(reader.next_int(), reader.next_int()) for _ in range(count)]
        else:
            params[key.lower()] = reader.next_float()

    params.setdefault("use_weights", 0)
    params.setdefault("num_opinions", 2)
    params.setdefault("choice_rule", "proportional")
    return params


def format_number(value):
    return f"{value:g}"


def describe_opinion_function(log):
    function = log.get("opinion_function")
    if function is None and ("k1" in log or "k2" in log):
        function = {"name": "threshold", "params": [log.get("k1", 0.0), log.get("k2", 1.0)]}
    if function is None:
        return ""
    params = ", ".join(format_number(p) for p in function["params"])
    rule = CHOICE_RULE_NAMES.get(log["choice_rule"], log["choice_rule"])
    return (
        f"Функция мнений: {function['name']}({params})    "
        f"правило: {rule}    мнений: {log['num_opinions']}"
    )


def parse_log_history(reader, num_vertices):
    """История шагов: список (словарь мнение -> число вершин, число рёбер).

    Старые логи (секция HISTORY) хранят только число вершин с мнением 1.
    """
    section = reader.next()
    count = reader.next_int()
    history = []
    if section == "HISTORY":
        for _ in range(count):
            ones = reader.next_int()
            edges_now = reader.next_int()
            counts = {opinion: c for opinion, c in ((0, num_vertices - ones), (1, ones)) if c > 0}
            history.append((counts, edges_now))
        return history

    if section != "OPINION_HISTORY":
        raise ValueError(f"Ожидалась секция OPINION_HISTORY, получена '{section}'")
    for _ in range(count):
        edges_now = reader.next_int()
        pairs = reader.next_int()
        counts = {}
        for _ in range(pairs):
            opinion = reader.next_int()
            counts[opinion] = reader.next_int()
        history.append((counts, edges_now))
    return history


def parse_log_opinion_changes(reader):
    reader.expect("OPINION_CHANGES")
    count = reader.next_int()
    changes = []
    for _ in range(count):
        step = reader.next_int()
        vertex = reader.next_int()
        frm = reader.next_int()
        to = reader.next_int()
        changes.append((step, vertex, frm, to))
    return changes


def parse_log_edge_events(reader):
    reader.expect("EDGE_EVENTS")
    count = reader.next_int()
    events = []
    for _ in range(count):
        step = reader.next_int()
        kind = reader.next()
        u = reader.next_int()
        v = reader.next_int()
        w = reader.next_float()
        events.append((step, kind == "ADD", u, v, w))
    return events


def parse_log_final_opinions(reader):
    reader.expect("FINAL_OPINIONS")
    count = reader.next_int()
    return [reader.next_int() for _ in range(count)]


def parse_log(filepath, num_vertices):
    reader = TokenReader(filepath)

    log = parse_log_params(reader)

    reader.expect("INITIAL_GRAPH_FILE")
    reader.next()
    reader.expect("FINAL_GRAPH_FILE")
    reader.next()

    log["history"] = parse_log_history(reader, num_vertices)
    log["opinion_changes"] = parse_log_opinion_changes(reader)
    log["edge_events"] = parse_log_edge_events(reader)
    log["final_opinions"] = parse_log_final_opinions(reader)

    return log


def initial_opinions(num_vertices, stubborn, saved):
    """Начальные мнения из файла графа, а для старых файлов — упрямые 1, остальные 0."""
    if saved is not None:
        return list(saved)
    return [1 if v in stubborn else 0 for v in range(num_vertices)]


def count_opinions(opinions):
    counts = {}
    for opinion in opinions:
        counts[opinion] = counts.get(opinion, 0) + 1
    return counts


def group_opinion_changes_by_step(opinion_changes):
    changes_by_step = {}
    for step, vertex, frm, to in opinion_changes:
        changes_by_step.setdefault(step, []).append((vertex, to))
    return changes_by_step


def group_edge_events_by_step(edge_events):
    events_by_step = {}
    for step, added, u, v, w in edge_events:
        events_by_step.setdefault(step, []).append((added, u, v, w))
    return events_by_step


def apply_edge_events(edges, events):
    for added, u, v, w in events:
        key = (min(u, v), max(u, v))
        if added:
            edges[key] = w
        else:
            edges.pop(key, None)


def apply_opinion_changes(opinions, changes):
    for vertex, to in changes:
        opinions[vertex] = to


def build_states(num_vertices, initial_edges, start_opinions, log):
    changes_by_step = group_opinion_changes_by_step(log["opinion_changes"])
    events_by_step = group_edge_events_by_step(log["edge_events"])

    opinions = list(start_opinions)
    edges = dict(initial_edges)

    states = [(list(opinions), dict(edges))]

    for t in range(1, log["t_max"] + 1):
        apply_edge_events(edges, events_by_step.get(t, []))
        apply_opinion_changes(opinions, changes_by_step.get(t, []))
        states.append((list(opinions), dict(edges)))

    return states


def counts_timeline(start_opinions, log):
    """Число вершин по мнениям на шагах 0..T: шаг 0 из начальных мнений, остальные из лога."""
    return [count_opinions(start_opinions)] + [counts for counts, _ in log["history"]]


class OpinionColors:
    """Цвета мнений, одинаковые на всех шагах.

    Два мнения — серый (0) и красный (1), как раньше. До 10 мнений — у каждого свой цвет.
    Больше 10 — свой цвет получают 9 мнений, которых за прогон было больше всего,
    остальные рисуются серым как «прочие».
    """

    def __init__(self, num_opinions, timeline):
        if num_opinions == 2:
            self.colors = dict(BINARY_COLORS)
            self.has_others = False
        elif num_opinions <= len(PALETTE):
            self.colors = {k: PALETTE[k] for k in range(num_opinions)}
            self.has_others = False
        else:
            totals = {}
            for counts in timeline:
                for opinion, count in counts.items():
                    totals[opinion] = totals.get(opinion, 0) + count
            top = sorted(totals, key=lambda k: (-totals[k], k))[:MAX_COLORED_OPINIONS]
            self.colors = {opinion: PALETTE[i] for i, opinion in enumerate(top)}
            self.has_others = len(totals) > len(top)

    def color(self, opinion):
        return self.colors.get(opinion, OTHER_COLOR)

    def shown_opinions(self):
        return list(self.colors)

    def legend_handles(self, with_stubborn):
        handles = [Patch(color=self.colors[k], label=f"мнение {k}") for k in self.shown_opinions()]
        if self.has_others:
            handles.append(Patch(color=OTHER_COLOR, label="прочие"))
        if with_stubborn:
            handles.append(Line2D([], [], marker="o", color="none", markerfacecolor="white",
                                  markeredgecolor=STUBBORN_EDGE_COLOR, markeredgewidth=2,
                                  markersize=9, label="упрямая"))
        return handles


def plot_shares(ax, timeline, colors, num_vertices):
    steps = list(range(len(timeline)))
    for opinion in colors.shown_opinions():
        shares = [counts.get(opinion, 0) / num_vertices for counts in timeline]
        line_color = "dimgray" if colors.color(opinion) == "lightgray" else colors.color(opinion)
        ax.plot(steps, shares, color=line_color, linewidth=1.5, label=f"мнение {opinion}")
    if colors.has_others:
        shown = set(colors.shown_opinions())
        others = [sum(c for k, c in counts.items() if k not in shown) / num_vertices for counts in timeline]
        ax.plot(steps, others, color="darkgray", linestyle="--", linewidth=1.2, label="прочие")
    ax.set_ylim(-0.02, 1.02)
    ax.set_ylabel("Доля вершин")


def collect_union_edges(states):
    union_edges = set()
    for _, edges in states:
        union_edges.update(edges.keys())
    return union_edges


def build_layout_graph(num_vertices, states):
    graph = nx.Graph()
    graph.add_nodes_from(range(num_vertices))
    graph.add_edges_from(collect_union_edges(states))
    return graph


class GraphViewer:
    def __init__(self, num_vertices, stubborn, states, timeline, colors, t_max, subtitle=""):
        self.num_vertices = num_vertices
        self.stubborn = stubborn
        self.states = states
        self.timeline = timeline
        self.colors = colors
        self.t_max = t_max
        self.playing = False

        self.layout_graph = build_layout_graph(num_vertices, states)
        self.pos = nx.spring_layout(self.layout_graph, seed=42)
        self.regular_nodes = [v for v in range(num_vertices) if v not in stubborn]
        self.stubborn_nodes = sorted(stubborn)

        self.fig = plt.figure(figsize=(9, 8.5))
        grid = self.fig.add_gridspec(2, 1, height_ratios=[4, 1.3], left=0.1, right=0.78,
                                     top=0.92, bottom=0.17, hspace=0.25)
        self.ax = self.fig.add_subplot(grid[0])
        self.ax_shares = self.fig.add_subplot(grid[1])
        if subtitle:
            self.fig.suptitle(subtitle, fontsize=10, color="dimgray")

        plot_shares(self.ax_shares, timeline, colors, num_vertices)
        self.ax_shares.set_xlim(0, max(t_max, 1))
        self.ax_shares.set_xlabel("Шаг")
        self.step_marker = self.ax_shares.axvline(0, color="black", linewidth=1)

        self.fig.legend(handles=colors.legend_handles(bool(stubborn)), loc="upper left",
                        bbox_to_anchor=(0.79, 0.9), fontsize=9, frameon=False)

        slider_ax = self.fig.add_axes([0.12, 0.05, 0.55, 0.03])
        self.slider = Slider(slider_ax, "Шаг", 0, t_max, valinit=0, valstep=1)
        self.slider.on_changed(self.on_slider_changed)

        button_ax = self.fig.add_axes([0.72, 0.04, 0.1, 0.05])
        self.play_button = Button(button_ax, "Play")
        self.play_button.on_clicked(self.toggle_play)

        self.timer = self.fig.canvas.new_timer(interval=150)
        self.timer.add_callback(self.on_timer)
        self.timer.start()

        self.fig.canvas.mpl_connect("close_event", self.on_close)

    def draw_nodes(self, nodes, opinions, outlined):
        if not nodes:
            return
        nx.draw_networkx_nodes(
            self.layout_graph, self.pos, nodelist=nodes, ax=self.ax, node_size=120,
            node_color=[self.colors.color(opinions[v]) for v in nodes],
            edgecolors=STUBBORN_EDGE_COLOR if outlined else "none",
            linewidths=2.0 if outlined else 0.0,
        )

    def draw_step(self, t):
        self.ax.clear()
        opinions, edges = self.states[t]

        nx.draw_networkx_edges(
            self.layout_graph, self.pos, edgelist=list(edges.keys()), ax=self.ax, alpha=0.3
        )
        self.draw_nodes(self.regular_nodes, opinions, outlined=False)
        self.draw_nodes(self.stubborn_nodes, opinions, outlined=True)

        counts = self.timeline[t]
        leader = max(counts, key=lambda k: (counts[k], -k)) if counts else None
        leader_text = f"лидер: {leader} ({counts[leader]} из {self.num_vertices})" if counts else ""
        self.ax.set_title(
            f"Шаг {t} / {self.t_max}    мнений: {len(counts)}    {leader_text}    рёбер: {len(edges)}",
            fontsize=10,
        )
        self.ax.axis("off")
        self.step_marker.set_xdata([t, t])
        self.fig.canvas.draw_idle()

    def on_slider_changed(self, val):
        self.draw_step(int(val))

    def on_timer(self):
        if not self.playing:
            return
        t = int(self.slider.val)
        next_t = 0 if t >= self.t_max else t + 1
        self.slider.set_val(next_t)

    def on_close(self, event):
        self.playing = False
        self.timer.stop()

    def toggle_play(self, event):
        self.playing = not self.playing
        self.play_button.label.set_text("Пауза" if self.playing else "Play")

    def show(self):
        self.draw_step(0)
        plt.show()


def run_graph_viewer(num_vertices, stubborn, states, timeline, colors, t_max, subtitle=""):
    viewer = GraphViewer(num_vertices, stubborn, states, timeline, colors, t_max, subtitle)
    viewer.show()


def build_dynamics_figure(log, timeline, colors, num_vertices):
    steps = list(range(len(timeline)))
    edges_series = [None] + [edges for _, edges in log["history"]]

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(9, 8), sharex=True,
                                        gridspec_kw={"height_ratios": [2.2, 1, 1]})

    plot_shares(ax1, timeline, colors, num_vertices)
    ax1.set_title("Граф слишком большой для визуализации структуры - показана только динамика",
                  fontsize=10)
    ax1.legend(fontsize=8, loc="upper left", bbox_to_anchor=(1.01, 1.0), frameon=False)
    subtitle = describe_opinion_function(log)
    if subtitle:
        fig.suptitle(subtitle, fontsize=10, color="dimgray")

    ax2.plot(steps, [len(counts) for counts in timeline], color="black")
    ax2.set_ylabel("Различных\nмнений")

    ax3.plot(steps[1:], edges_series[1:], color="blue")
    ax3.set_ylabel("Число рёбер")
    ax3.set_xlabel("Шаг")

    fig.tight_layout()
    return fig


def run_dynamics_only(log, timeline, colors, num_vertices):
    build_dynamics_figure(log, timeline, colors, num_vertices)
    plt.show()


def resolve_run_paths(run_dir):
    initial_graph_path = os.path.join(run_dir, "graph_initial.txt")
    log_path = os.path.join(run_dir, "simulation_log.txt")
    return initial_graph_path, log_path


def check_files_exist(*paths):
    for path in paths:
        if not os.path.isfile(path):
            print(f"Не найден файл: {path}")
            sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Использование: python visualize.py <путь_к_папке_прогона>")
        sys.exit(1)

    initial_graph_path, log_path = resolve_run_paths(sys.argv[1])
    check_files_exist(initial_graph_path, log_path)

    num_vertices, initial_edges, stubborn, saved_opinions = parse_graph(initial_graph_path)
    log = parse_log(log_path, num_vertices)
    start = initial_opinions(num_vertices, stubborn, saved_opinions)
    timeline = counts_timeline(start, log)
    colors = OpinionColors(log["num_opinions"], timeline)

    if num_vertices > MAX_VISUAL_VERTICES:
        print(
            f"Вершин {num_vertices} > {MAX_VISUAL_VERTICES} - "
            "показываю только графики динамики, без структуры графа."
        )
        run_dynamics_only(log, timeline, colors, num_vertices)
        return

    states = build_states(num_vertices, initial_edges, start, log)
    run_graph_viewer(num_vertices, stubborn, states, timeline, colors, log["t_max"],
                     describe_opinion_function(log))


if __name__ == "__main__":
    main()
