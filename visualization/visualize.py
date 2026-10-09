import sys
import os

import matplotlib
matplotlib.use("TkAgg")

import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button
import networkx as nx


MAX_VISUAL_VERTICES = 100


class TokenReader:
    def __init__(self, filepath):
        with open(filepath, "r", encoding="utf-8") as f:
            self._tokens = f.read().split()
        self._pos = 0

    def next(self):
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
    
    def peek(self):
        if self._pos >= len(self._tokens):
            return None
        return self._tokens[self._pos]


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

    return num_vertices, edges, stubborn


def parse_log_params(reader):
    reader.expect("SEED")
    seed = reader.next_int()
    reader.expect("T_MAX")
    t_max = reader.next_int()
    reader.expect("K1")
    k1 = reader.next_float()
    reader.expect("K2")
    k2 = reader.next_float()
    reader.expect("DYNAMIC_EDGES")
    dynamic_edges = reader.next_int()
    reader.expect("P0")
    p0 = reader.next_float()
    reader.expect("K")
    k = reader.next_float()
    reader.expect("REMOVE_EDGES")
    remove_edges = reader.next_int()
    reader.expect("REMOVE_P0")
    remove_p0 = reader.next_float()
    reader.expect("REMOVE_K")
    remove_k = reader.next_float()
    use_weights = 0
    if reader.peek() == "USE_WEIGHTS":
        reader.next()
        use_weights = reader.next_int()

    return {
        "seed": seed,
        "t_max": t_max,
        "k1": k1,
        "k2": k2,
        "dynamic_edges": dynamic_edges,
        "p0": p0,
        "k": k,
        "remove_edges": remove_edges,
        "remove_p0": remove_p0,
        "remove_k": remove_k,
        "use_weights": use_weights,
    }


def parse_log_history(reader):
    reader.expect("HISTORY")
    count = reader.next_int()
    history = []
    for _ in range(count):
        ones = reader.next_int()
        edges_now = reader.next_int()
        history.append((ones, edges_now))
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


def parse_log(filepath):
    reader = TokenReader(filepath)

    log = parse_log_params(reader)

    reader.expect("INITIAL_GRAPH_FILE")
    reader.next()
    reader.expect("FINAL_GRAPH_FILE")
    reader.next()

    log["history"] = parse_log_history(reader)
    log["opinion_changes"] = parse_log_opinion_changes(reader)
    log["edge_events"] = parse_log_edge_events(reader)
    log["final_opinions"] = parse_log_final_opinions(reader)

    return log


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


def build_states(num_vertices, initial_edges, stubborn, log):
    changes_by_step = group_opinion_changes_by_step(log["opinion_changes"])
    events_by_step = group_edge_events_by_step(log["edge_events"])

    opinions = [1 if v in stubborn else 0 for v in range(num_vertices)]
    edges = dict(initial_edges)

    states = [(list(opinions), dict(edges))]

    for t in range(1, log["t_max"] + 1):
        apply_edge_events(edges, events_by_step.get(t, []))
        apply_opinion_changes(opinions, changes_by_step.get(t, []))
        states.append((list(opinions), dict(edges)))

    return states


def collect_union_edges(states):
    union_edges = set()
    for _, edges in states:
        union_edges.update(edges.keys())
    return union_edges


def build_layout_graph(num_vertices, states):
    union_edges = collect_union_edges(states)
    graph = nx.Graph()
    graph.add_nodes_from(range(num_vertices))
    graph.add_edges_from(union_edges)
    return graph


def node_colors_for_step(num_vertices, opinions, stubborn):
    colors = []
    for v in range(num_vertices):
        if v in stubborn:
            colors.append("darkred")
        elif opinions[v] == 1:
            colors.append("red")
        else:
            colors.append("lightgray")
    return colors


class GraphViewer:
    def __init__(self, num_vertices, stubborn, states, t_max):
        self.num_vertices = num_vertices
        self.stubborn = stubborn
        self.states = states
        self.t_max = t_max
        self.playing = False

        self.layout_graph = build_layout_graph(num_vertices, states)
        self.pos = nx.spring_layout(self.layout_graph, seed=42)

        self.fig, self.ax = plt.subplots(figsize=(8, 7))
        plt.subplots_adjust(bottom=0.2)

        slider_ax = plt.axes([0.15, 0.08, 0.6, 0.03])
        self.slider = Slider(slider_ax, "Шаг", 0, t_max, valinit=0, valstep=1)
        self.slider.on_changed(self.on_slider_changed)

        button_ax = plt.axes([0.8, 0.07, 0.1, 0.05])
        self.play_button = Button(button_ax, "Play")
        self.play_button.on_clicked(self.toggle_play)

        self.timer = self.fig.canvas.new_timer(interval=150)
        self.timer.add_callback(self.on_timer)
        self.timer.start()

    def draw_step(self, t):
        self.ax.clear()
        opinions, edges = self.states[t]

        edge_list = list(edges.keys())
        nx.draw_networkx_edges(
            self.layout_graph, self.pos, edgelist=edge_list, ax=self.ax, alpha=0.3
        )

        colors = node_colors_for_step(self.num_vertices, opinions, self.stubborn)
        nx.draw_networkx_nodes(
            self.layout_graph, self.pos, node_color=colors, ax=self.ax, node_size=120
        )

        ones = sum(opinions)
        self.ax.set_title(
            f"Шаг {t} / {self.t_max}    "
            f"мнение=1: {ones} из {self.num_vertices}    "
            f"рёбер: {len(edges)}"
        )
        self.ax.axis("off")
        self.fig.canvas.draw_idle()

    def on_slider_changed(self, val):
        self.draw_step(int(val))

    def on_timer(self):
        if not self.playing:
            return
        t = int(self.slider.val)
        next_t = 0 if t >= self.t_max else t + 1
        self.slider.set_val(next_t)

    def toggle_play(self, event):
        self.playing = not self.playing
        self.play_button.label.set_text("Пауза" if self.playing else "Play")

    def show(self):
        self.draw_step(0)
        plt.show()


def run_graph_viewer(num_vertices, stubborn, states, t_max):
    viewer = GraphViewer(num_vertices, stubborn, states, t_max)
    viewer.show()


def run_dynamics_only(log):
    steps = list(range(1, len(log["history"]) + 1))
    ones_series = [h[0] for h in log["history"]]
    edges_series = [h[1] for h in log["history"]]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 7), sharex=True)

    ax1.plot(steps, ones_series, color="red")
    ax1.set_ylabel("Мнение = 1")
    ax1.set_title("Граф слишком большой для визуализации структуры - показана только динамика")

    ax2.plot(steps, edges_series, color="blue")
    ax2.set_ylabel("Число рёбер")
    ax2.set_xlabel("Шаг")

    plt.tight_layout()
    plt.show()


def resolve_run_paths(run_dir):
    initial_graph_path = os.path.join(run_dir, "graph_initial.txt")
    log_path = os.path.join(run_dir, "simulation_log.txt")
    return initial_graph_path, log_path


def check_files_exist(initial_graph_path, log_path):
    if not os.path.isfile(initial_graph_path):
        print(f"Не найден файл: {initial_graph_path}")
        sys.exit(1)
    if not os.path.isfile(log_path):
        print(f"Не найден файл: {log_path}")
        sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Использование: python visualize.py <путь_к_папке_прогона>")
        sys.exit(1)

    run_dir = sys.argv[1]
    initial_graph_path, log_path = resolve_run_paths(run_dir)
    check_files_exist(initial_graph_path, log_path)

    num_vertices, initial_edges, stubborn = parse_graph(initial_graph_path)
    log = parse_log(log_path)

    if num_vertices > MAX_VISUAL_VERTICES:
        print(
            f"Вершин {num_vertices} > {MAX_VISUAL_VERTICES} - "
            "показываю только графики динамики, без структуры графа."
        )
        run_dynamics_only(log)
        return

    states = build_states(num_vertices, initial_edges, stubborn, log)
    run_graph_viewer(num_vertices, stubborn, states, log["t_max"])


if __name__ == "__main__":
    main()