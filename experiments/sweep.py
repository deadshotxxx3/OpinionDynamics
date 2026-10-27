"""Серийные прогоны: перебор параметров по сетке, несколько seed на точку, метрики в results.csv.

Использование:
    python sweep.py configs/stubborn_capture.json [--jobs N] [--out DIR] [--keep-runs]

Формат конфига (JSON):
    {
      "name": "stubborn_capture",             имя серии, по нему называется папка результатов
      "binary": "../build/opinion_dynamics",  путь к программе относительно папки experiments
      "seeds": 20,                            сколько seed на каждую точку сетки
      "seed_start": 1,                        первый seed (необязательно, по умолчанию 1)
      "fixed": {"vertices": 50, "tmax": 500}, флаги, одинаковые для всех прогонов
      "vary": {                               флаги, которые перебираются (все сочетания)
        "stubborn-assign": [0, 5, 10],
        "choice-rule": ["max", "proportional"],
        "function": [                         вариант из нескольких флагов сразу, в таблицу
          {"label": "s", "opinion-func": "polynomial", "opinion-params": "1,0"}   пишется label
        ]
      }
    }

Ключ — имя флага без `--`. Значение true — флаг без значения (например "use-weights": true),
false или null — флаг не передаётся.
"""

import argparse
import concurrent.futures
import csv
import itertools
import json
import os
import shutil
import subprocess
import sys
import time

from metrics import METRIC_COLUMNS, run_metrics


EXPERIMENTS_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(EXPERIMENTS_DIR, "results")
RUN_DIR_PREFIX = "Папка прогона:"
PROGRESS_STEPS = 20


def flag_args(name, value):
    if value is True:
        return [f"--{name}"]
    if value is False or value is None:
        return []
    return [f"--{name}", str(value)]


def variant_args(name, value):
    """Флаги одного значения из vary и подпись для таблицы."""
    if isinstance(value, dict):
        args = []
        for key, item in value.items():
            if key != "label":
                args += flag_args(key, item)
        label = value.get("label", " ".join(args))
        return args, label
    return flag_args(name, value), value


def build_points(config):
    """Все сочетания значений из vary: список (подписи по ключам, флаги)."""
    vary = config.get("vary", {})
    names = list(vary)
    points = []
    for combo in itertools.product(*(vary[name] for name in names)):
        labels = {}
        args = []
        for name, value in zip(names, combo):
            extra, label = variant_args(name, value)
            labels[name] = label
            args += extra
        points.append((labels, args))
    return names, points


def resolve_binary(config, config_path):
    raw = config.get("binary", "../build/opinion_dynamics")
    if os.path.isabs(raw):
        candidates = [raw]
    else:
        candidates = [
            os.path.join(EXPERIMENTS_DIR, raw),
            os.path.join(os.path.dirname(os.path.abspath(config_path)), raw),
            os.path.abspath(raw),
        ]
    for candidate in candidates:
        if os.path.isfile(candidate):
            return os.path.abspath(candidate)
    sys.exit("Не найдена программа. Проверено:\n  " + "\n  ".join(candidates)
             + "\nСоберите проект или укажите путь в поле \"binary\" конфига.")


def unique_dir(path):
    candidate = path
    copy = 2
    while os.path.exists(candidate):
        candidate = f"{path}_{copy}"
        copy += 1
    return candidate


def run_one(binary, base_args, point_index, seed, runs_root, keep_runs):
    """Один прогон: запуск программы и подсчёт метрик. Возвращает (метрики, ошибка)."""
    point_dir = os.path.join(runs_root, f"p{point_index:04d}")
    command = [binary, "--batch"] + base_args + ["--seed", str(seed), "--quiet", "--runs-dir", point_dir]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        lines = (result.stdout + result.stderr).strip().splitlines()
        return None, lines[0] if lines else f"код возврата {result.returncode}"

    run_dir = None
    for line in result.stdout.splitlines():
        if line.startswith(RUN_DIR_PREFIX):
            run_dir = line[len(RUN_DIR_PREFIX):].strip()
    if run_dir is None:
        return None, "программа не напечатала папку прогона"

    try:
        metrics = run_metrics(run_dir)
    except (OSError, ValueError) as e:
        return None, f"не удалось посчитать метрики: {e}"
    if not keep_runs:
        shutil.rmtree(run_dir, ignore_errors=True)
    return metrics, ""


def main():
    parser = argparse.ArgumentParser(description="Серийные прогоны opinion_dynamics с метриками")
    parser.add_argument("config", help="JSON-файл с описанием серии")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1,
                        help="сколько прогонов запускать одновременно (по умолчанию число ядер)")
    parser.add_argument("--out", help="папка результатов (по умолчанию results/<name>)")
    parser.add_argument("--keep-runs", action="store_true",
                        help="не удалять папки прогонов после подсчёта метрик")
    args = parser.parse_args()

    with open(args.config, "r", encoding="utf-8") as f:
        config = json.load(f)

    name = config.get("name", os.path.splitext(os.path.basename(args.config))[0])
    binary = resolve_binary(config, args.config)
    seeds = int(config.get("seeds", 10))
    seed_start = int(config.get("seed_start", 1))
    base = []
    for key, value in config.get("fixed", {}).items():
        base += flag_args(key, value)

    vary_names, points = build_points(config)
    out_dir = unique_dir(os.path.abspath(args.out or os.path.join(RESULTS_DIR, name)))
    runs_root = os.path.join(out_dir, "runs")
    os.makedirs(runs_root)
    with open(os.path.join(out_dir, "config.json"), "w", encoding="utf-8") as f:
        json.dump(config, f, ensure_ascii=False, indent=2)

    tasks = [(i, seed) for i in range(len(points)) for seed in range(seed_start, seed_start + seeds)]
    print(f"Серия «{name}»: {len(points)} точек x {seeds} seed = {len(tasks)} прогонов, "
          f"потоков {args.jobs}")
    print(f"Результаты: {out_dir}")

    rows = []
    errors = []
    started = time.time()
    report_every = max(1, len(tasks) // PROGRESS_STEPS)
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {
            pool.submit(run_one, binary, base + points[i][1], i, seed, runs_root, args.keep_runs): (i, seed)
            for i, seed in tasks
        }
        for done, future in enumerate(concurrent.futures.as_completed(futures), start=1):
            i, seed = futures[future]
            metrics, error = future.result()
            row = {"point": i, **points[i][0], "seed": seed, "error": error}
            row.update(metrics or {})
            rows.append(row)
            if error:
                errors.append((i, seed, error))
            if done % report_every == 0 or done == len(tasks):
                print(f"  готово {done}/{len(tasks)}, {time.time() - started:.0f} с")

    rows.sort(key=lambda r: (r["point"], r["seed"]))
    csv_path = os.path.join(out_dir, "results.csv")
    columns = ["point"] + vary_names + ["seed"] + METRIC_COLUMNS + ["error"]
    with open(csv_path, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)

    if not args.keep_runs:
        shutil.rmtree(runs_root, ignore_errors=True)

    print(f"Таблица: {csv_path}")
    if errors:
        print(f"Ошибок: {len(errors)} из {len(tasks)}. Первая: точка {errors[0][0]}, "
              f"seed {errors[0][1]}: {errors[0][2]}")
        if len(errors) == len(tasks):
            sys.exit(1)


if __name__ == "__main__":
    main()
