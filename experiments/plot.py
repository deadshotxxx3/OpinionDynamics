"""Графики по results.csv: среднее метрики с 95% доверительным интервалом.

Использование:
    python plot.py results/stubborn_capture/results.csv --x stubborn_share --y captured \
        --group function --panel choice-rule [--out рисунок.png]

--x      столбец по горизонтали (числа);
--y      метрика, которая усредняется по seed;
--group  столбец, по которому строятся разные линии (необязательно);
--panel  столбец, по которому строятся соседние панели (необязательно).

Совпадающие линии видны по разным маркерам. Для метрик из нулей и единиц
(consensus, captured) интервал считается по Уилсону,
для остальных — как среднее ± 1.96 стандартной ошибки. Пустые значения (например,
consensus_step в прогонах без консенсуса) не учитываются, число учтённых прогонов
печатается в таблице.
"""

import argparse
import csv
import math
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


Z_95 = 1.96
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]
MARKER_SIZES = [9, 7, 5, 4]

LABELS = {
    "stubborn_share": "Доля упрямых вершин",
    "num_stubborn": "Число упрямых вершин",
    "stubborn-assign": "Число упрямых вершин",
    "captured": "Доля прогонов с захватом графа",
    "consensus": "Доля прогонов с консенсусом",
    "consensus_step": "Шаг консенсуса",
    "capture90_step": "Шаг, когда у 90% мнение упрямых",
    "stubborn_opinion_share": "Доля вершин с мнением упрямых",
    "winner_share": "Доля самого частого мнения",
    "distinct_final": "Мнений в конце",
    "num-opinions": "Число мнений",
    "choice-rule": "Правило",
    "function": "Функция",
    "proportional": "пропорционально",
    "max": "максимум",
}


def label(name):
    return LABELS.get(name, name)


def read_rows(path):
    with open(path, "r", encoding="utf-8", newline="") as f:
        return [row for row in csv.DictReader(f) if not row.get("error")]


def wilson(successes, n):
    if n == 0:
        return math.nan, math.nan, math.nan
    p = successes / n
    denominator = 1 + Z_95 ** 2 / n
    center = (p + Z_95 ** 2 / (2 * n)) / denominator
    half = Z_95 * math.sqrt(p * (1 - p) / n + Z_95 ** 2 / (4 * n * n)) / denominator
    return p, center - half, center + half


def mean_interval(values):
    n = len(values)
    if n == 0:
        return math.nan, math.nan, math.nan
    mean = sum(values) / n
    if n == 1:
        return mean, mean, mean
    variance = sum((v - mean) ** 2 for v in values) / (n - 1)
    half = Z_95 * math.sqrt(variance / n)
    return mean, mean - half, mean + half


def summarize(values):
    """Среднее, нижняя и верхняя граница интервала, число значений."""
    if values and all(v in (0.0, 1.0) for v in values):
        mean, low, high = wilson(sum(values), len(values))
    else:
        mean, low, high = mean_interval(values)
    return mean, low, high, len(values)


def ordered_unique(values):
    seen = []
    for value in values:
        if value not in seen:
            seen.append(value)
    return seen


def aggregate(rows, x, y, group, panel):
    """{панель: {линия: [(x, среднее, низ, верх, n), ...]}}."""
    buckets = {}
    for row in rows:
        key = (row[panel] if panel else "", row[group] if group else "", float(row[x]))
        buckets.setdefault(key, [])
        if row[y] != "":
            buckets[key].append(float(row[y]))

    panels = ordered_unique(row[panel] if panel else "" for row in rows)
    groups = ordered_unique(row[group] if group else "" for row in rows)
    result = {}
    for p in panels:
        result[p] = {}
        for g in groups:
            points = sorted(
                (xv, *summarize(vals)) for (pk, gk, xv), vals in buckets.items() if pk == p and gk == g
            )
            if points:
                result[p][g] = points
    return result


def print_table(data, x, y):
    for p, lines in data.items():
        for g, points in lines.items():
            title = " / ".join(v for v in (label(p), label(g)) if v)
            print(f"\n{title or y}")
            print(f"  {x:>16} {'среднее':>10} {'95% ниже':>10} {'95% выше':>10} {'n':>4}")
            for xv, mean, low, high, n in points:
                print(f"  {xv:>16g} {mean:>10.3f} {low:>10.3f} {high:>10.3f} {n:>4}")


def plot(data, x, y, group, panel, out_path):
    panels = list(data)
    fig, axes = plt.subplots(1, len(panels), figsize=(5.5 * len(panels), 4.2), sharex=True, sharey=True,
                             squeeze=False)
    for ax, p in zip(axes[0], panels):
        for index, (g, points) in enumerate(data[p].items()):
            xs = [pt[0] for pt in points]
            means = [pt[1] for pt in points]
            below = [max(pt[1] - pt[2], 0.0) for pt in points]
            above = [max(pt[3] - pt[1], 0.0) for pt in points]
            ax.errorbar(xs, means, yerr=[below, above], marker=MARKERS[index % len(MARKERS)],
                        markersize=MARKER_SIZES[index % len(MARKER_SIZES)], markerfacecolor="none",
                        linewidth=1.3, elinewidth=0.8, capsize=2, label=label(g) if group else None)
        if panel:
            ax.set_title(f"{label(panel)}: {label(p)}")
        ax.set_xlabel(label(x))
        ax.grid(alpha=0.3)
    axes[0][0].set_ylabel(label(y))
    if group:
        axes[0][-1].legend(title=label(group), fontsize=9)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"\nРисунок: {out_path}")


def main():
    parser = argparse.ArgumentParser(description="График метрики по результатам серии")
    parser.add_argument("csv", help="results.csv из sweep.py")
    parser.add_argument("--x", required=True, help="столбец по горизонтали")
    parser.add_argument("--y", required=True, help="метрика по вертикали")
    parser.add_argument("--group", help="столбец для разных линий")
    parser.add_argument("--panel", help="столбец для разных панелей")
    parser.add_argument("--out", help="файл рисунка (.png или .pdf), по умолчанию рядом с csv")
    args = parser.parse_args()

    rows = read_rows(args.csv)
    if not rows:
        raise SystemExit("В таблице нет прогонов без ошибок")
    for column in (args.x, args.y, args.group, args.panel):
        if column and column not in rows[0]:
            raise SystemExit(f"Нет столбца «{column}». Есть: {', '.join(rows[0])}")

    data = aggregate(rows, args.x, args.y, args.group, args.panel)
    print_table(data, args.x, args.y)
    out = args.out or os.path.join(os.path.dirname(os.path.abspath(args.csv)), f"{args.y}_vs_{args.x}.png")
    plot(data, args.x, args.y, args.group, args.panel, out)


if __name__ == "__main__":
    main()
