"""Метрики одного прогона по файлам его папки (graph_initial.txt и simulation_log.txt)."""

import os


METRIC_COLUMNS = [
    "num_vertices",
    "num_stubborn",
    "stubborn_share",
    "steps",
    "distinct_initial",
    "distinct_final",
    "consensus",
    "consensus_step",
    "winner",
    "winner_share",
    "stubborn_opinion",
    "stubborn_opinion_share",
    "captured",
    "capture90_step",
]


def read_graph_file(path):
    """Число вершин, множество упрямых и начальные мнения (None, если секции OPINIONS нет)."""
    with open(path, "r", encoding="utf-8") as f:
        tokens = f.read().split()

    pos = 0

    def take():
        nonlocal pos
        pos += 1
        return tokens[pos - 1]

    if take() != "VERTICES":
        raise ValueError(f"{path}: файл должен начинаться с VERTICES")
    num_vertices = int(take())
    if take() != "STUBBORN":
        raise ValueError(f"{path}: нет секции STUBBORN")
    stubborn = {int(take()) for _ in range(int(take()))}
    if take() != "EDGES":
        raise ValueError(f"{path}: нет секции EDGES")
    pos += 3 * int(take())

    opinions = None
    if pos < len(tokens) and tokens[pos] == "OPINIONS":
        pos += 1
        opinions = [int(take()) for _ in range(int(take()))]
    return num_vertices, stubborn, opinions


def read_history(path, num_vertices):
    """Число вершин по мнениям после каждого шага: список словарей «мнение -> вершин»."""
    history = []
    with open(path, "r", encoding="utf-8") as f:
        lines = iter(f)
        for line in lines:
            parts = line.split()
            if not parts or parts[0] not in ("OPINION_HISTORY", "HISTORY"):
                continue
            section, count = parts[0], int(parts[1])
            for _ in range(count):
                values = [int(v) for v in next(lines).split()]
                if section == "HISTORY":
                    ones = values[0]
                    counts = {0: num_vertices - ones, 1: ones}
                else:
                    pairs = values[2:]
                    counts = {pairs[i]: pairs[i + 1] for i in range(0, len(pairs), 2)}
                history.append({k: c for k, c in counts.items() if c > 0})
            return history
    raise ValueError(f"{path}: нет секции OPINION_HISTORY")


def first_step(history, condition):
    """Первый шаг (шаги считаются с 1), на котором выполнено условие, или пустая строка."""
    for step, counts in enumerate(history, start=1):
        if condition(counts):
            return step
    return ""


def run_metrics(run_dir):
    """Словарь метрик прогона, ключи — `METRIC_COLUMNS`.

    - consensus — у всех вершин в конце одно мнение (1/0), consensus_step — первый такой шаг;
    - winner — самое частое мнение в конце, winner_share — его доля;
    - stubborn_opinion — мнение упрямых вершин, если оно у всех упрямых одинаковое;
    - stubborn_opinion_share — доля вершин с этим мнением в конце;
    - captured — это мнение в конце у всех вершин (1/0), capture90_step — первый шаг, когда
      оно у 90% вершин. Без упрямых вершин захвата нет: captured = 0, доля 0.
    """
    num_vertices, stubborn, start = read_graph_file(os.path.join(run_dir, "graph_initial.txt"))
    if start is None:
        start = [1 if v in stubborn else 0 for v in range(num_vertices)]
    history = read_history(os.path.join(run_dir, "simulation_log.txt"), num_vertices)
    if not history:
        raise ValueError(f"{run_dir}: в логе нет ни одного шага")

    final = history[-1]
    winner = max(final, key=lambda k: (final[k], -k))
    stubborn_opinions = {start[v] for v in stubborn}
    target = next(iter(stubborn_opinions)) if len(stubborn_opinions) == 1 else None

    metrics = {
        "num_vertices": num_vertices,
        "num_stubborn": len(stubborn),
        "stubborn_share": round(len(stubborn) / num_vertices, 6),
        "steps": len(history),
        "distinct_initial": len(set(start)),
        "distinct_final": len(final),
        "consensus": int(len(final) == 1),
        "consensus_step": first_step(history, lambda c: len(c) == 1),
        "winner": winner,
        "winner_share": round(final[winner] / num_vertices, 6),
        "stubborn_opinion": "" if target is None else target,
        "stubborn_opinion_share": "",
        "captured": "",
        "capture90_step": "",
    }
    if not stubborn:
        metrics["stubborn_opinion_share"] = 0
        metrics["captured"] = 0
    elif target is not None:
        share = final.get(target, 0) / num_vertices
        metrics["stubborn_opinion_share"] = round(share, 6)
        metrics["captured"] = int(final.get(target, 0) == num_vertices)
        metrics["capture90_step"] = first_step(
            history, lambda c: c.get(target, 0) >= 0.9 * num_vertices)
    return metrics
