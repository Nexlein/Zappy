#!/usr/bin/env python3
import os
import sys
import json
from collections import defaultdict
import glob
import matplotlib.pyplot as plt
import networkx as nx  # type: ignore


def save_bar_chart(x_data, y_data, xlabel, ylabel, title, filepath, color):
    plt.figure(figsize=(10, 6))
    plt.bar(x_data, y_data, color=color)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(filepath)
    plt.close()


def generate_charts(run_folder):
    metrics_dir = os.path.join(run_folder, "metrics")
    charts_dir = os.path.join(run_folder, "charts")
    os.makedirs(charts_dir, exist_ok=True)

    # Load metrics
    metrics_files = glob.glob(os.path.join(metrics_dir, "*.json"))
    metrics_data = []
    for mf in metrics_files:
        with open(mf, "r") as f:
            metrics_data.append(json.load(f))

    if not metrics_data:
        print(f"No metrics found in {metrics_dir}")
        return

    metrics_data.sort(key=lambda x: x.get("pid", 0))

    # Data lists
    pids = [str(m.get("pid", "")) for m in metrics_data]
    survival_times = [m.get("survival_time_sec", 0) for m in metrics_data]
    levels = [m.get("highest_level", 1) for m in metrics_data]

    # --- 1. Survival Time Chart ---
    survival_path = os.path.join(charts_dir, "survival_time.png")
    save_bar_chart(
        pids,
        survival_times,
        "Drone PID",
        "Survival Time (s)",
        "AI Survival Time",
        survival_path,
        "green",
    )

    # --- 2. Highest Level Chart ---
    level_path = os.path.join(charts_dir, "highest_level.png")
    save_bar_chart(
        pids,
        levels,
        "Drone PID",
        "Highest Level Reached",
        "AI Highest Level",
        level_path,
        "blue",
    )

    # --- 3. Fork Tree Chart ---
    G = nx.DiGraph()
    labels = {}

    for m in metrics_data:
        pid = m.get("pid")
        ppid = m.get("parent_pid")
        G.add_node(pid)
        labels[pid] = f"PID {pid}\nLvl {m.get('highest_level', 1)}"
        if ppid and ppid in [x["pid"] for x in metrics_data]:
            G.add_edge(ppid, pid)

    plt.figure(figsize=(12, 8))
    try:
        # Try tree layout if pydot is installed or just use spring
        pos = nx.spring_layout(G, k=0.5, iterations=50)
    except Exception:
        pos = nx.spring_layout(G)

    nx.draw(
        G,
        pos,
        with_labels=True,
        labels=labels,
        node_size=3000,
        node_color="lightblue",
        font_size=8,
        font_weight="bold",
    )
    plt.title("Fork Tree")
    tree_path = os.path.join(charts_dir, "fork_tree.png")
    plt.savefig(tree_path)
    plt.close()

    # --- 4. Command Frequency Chart ---
    # Parse network/*.jsonl for command counts
    command_counts = defaultdict(int)
    net_logs = glob.glob(os.path.join(run_folder, "network", "*.jsonl"))
    for log_file in net_logs:
        with open(log_file, "r") as f:
            for line in f:
                if not line.strip():
                    continue
                try:
                    data = json.loads(line)
                    cmd = data.get("command")
                    if cmd:
                        # group 'Take <resource>' into 'Take'
                        base_cmd = cmd.split(" ")[0]
                        command_counts[base_cmd] += 1
                except json.JSONDecodeError:
                    continue

    if command_counts:
        cmds = list(command_counts.keys())
        counts = list(command_counts.values())

        # Sort by count descending
        sorted_cmds = [x for _, x in sorted(zip(counts, cmds), reverse=True)]
        sorted_counts = sorted(counts, reverse=True)

        cmd_path = os.path.join(charts_dir, "command_frequency.png")
        save_bar_chart(
            sorted_cmds,
            sorted_counts,
            "Commands",
            "Frequency",
            "Total Command Usage Across Swarm",
            cmd_path,
            "orange",
        )
    else:
        cmd_path = "No commands found"

    print(f"Charts saved in {charts_dir}/")
    print(f" - {survival_path}")
    print(f" - {level_path}")
    print(f" - {tree_path}")
    print(f" - {cmd_path}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: generate_charts.py <run_folder>")
        sys.exit(1)
    generate_charts(sys.argv[1])
