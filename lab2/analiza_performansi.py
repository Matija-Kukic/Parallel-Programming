import os
import matplotlib.pyplot as plt
import numpy as np

# 1. Automatsko parsiranje rezultata iz rezultati.txt
results_file = "results.txt"

# Rječnik u koji ćemo spremiti nizove vremena za svaku vrijednost parametra 'a'
data_dict = {1: [], 2: [], 3: []}

if not os.path.exists(results_file):
    raise FileNotFoundError(
        f"Datoteka '{results_file}' nije pronađena! Provjeri je li u istoj mapi kao i skripta."
    )

current_a = None

with open(results_file, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue

        # Detekcija parametra 'a'
        if line.startswith("a="):
            current_a = int(line.split("=")[1])

        # Izvlačenje vremena za procesore (n=...)
        elif line.startswith("n=") and current_a in data_dict:
            # Linija izgleda npr. "n=1 : 13.272" -> uzimamo dio nakon dvotočke
            vrijeme = float(line.split(":")[1].strip())
            data_dict[current_a].append(vrijeme)

# Pretvaranje u numpy nizove
processors = np.array([1, 2, 3, 4, 5, 6, 7, 8])
t_s1 = np.array(data_dict[1])
t_s2 = np.array(data_dict[2])
t_s3 = np.array(data_dict[3])

# Provjera jesu li svi podaci uspješno učitani
for a_val, arr in [
    (1, t_s1),
    (2, t_s2),
    (3, t_s3),
]:  # Popravljeno: uklonjen suvišni zarez unutar niza provjere
    if len(arr) != 8:
        print(
            f"[UPOZORENJE] Za a={a_val} učitano je {len(arr)}/8 vrijednosti. Grafovi možda neće biti točni."
        )

# 2. Izračun metrika (Ubrzanje S = T1 / Tp  i  Učinkovitost E = S / P)
s1_speedup = t_s1[0] / t_s1
s1_efficiency = s1_speedup / processors

s2_speedup = t_s2[0] / t_s2
s2_efficiency = s2_speedup / processors

s3_speedup = t_s3[0] / t_s3
s3_efficiency = s3_speedup / processors

output_dir = "plots"
os.makedirs(output_dir, exist_ok=True)


def plot_individual_speedup(scenarij_num, speedup_data, params_title, filename):
    plt.figure(figsize=(7, 5))
    plt.plot(processors, processors, "k--", label="Idealno ubrzanje")
    plt.plot(
        processors,
        speedup_data,
        "o-",
        color="tab:blue",
        linewidth=2,
        label="Izmjereno ubrzanje",
    )
    plt.title(
        f"Ubrzanje: Postava {scenarij_num}\n({params_title})",
        fontsize=11,
        fontweight="bold",
    )
    plt.xlabel("Broj procesora (P)")
    plt.ylabel("Ubrzanje (S)")
    plt.xlim(1, 8)
    plt.ylim(0, 9)
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename), dpi=300)
    plt.close()


def plot_individual_efficiency(scenarij_num, efficiency_data, params_title, filename):
    plt.figure(figsize=(7, 5))
    plt.axhline(y=1.0, color="k", linestyle="--", label="Idealna učinkovitost (100%)")
    plt.plot(
        processors,
        efficiency_data,
        "s-",
        color="tab:orange",
        linewidth=2,
        label="Izmjerena učinkovitost",
    )
    plt.title(
        f"Učinkovitost: Postava {scenarij_num}\n({params_title})",
        fontsize=11,
        fontweight="bold",
    )
    plt.xlabel("Broj procesora (P)")
    plt.ylabel("Učinkovitost (E)")
    plt.xlim(1, 8)
    plt.ylim(0, 1.2)
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, filename), dpi=300)
    plt.close()


# --- CRTANJE GRAFOVA ---

# Scenarij 1
plot_individual_speedup(
    scenarij_num=1,
    speedup_data=s1_speedup,
    params_title="a=1, d=9, 7 zadataka",
    filename="scenarij1_ubrzanje.png",
)
plot_individual_efficiency(
    scenarij_num=1,
    efficiency_data=s1_efficiency,
    params_title="a=1, d=9, 7 zadataka",
    filename="scenarij1_ucinkovitost.png",
)

# Scenarij 2
plot_individual_speedup(
    scenarij_num=2,
    speedup_data=s2_speedup,
    params_title="a=2, d=9, 49 zadataka",
    filename="scenarij2_ubrzanje.png",
)
plot_individual_efficiency(
    scenarij_num=2,
    efficiency_data=s2_efficiency,
    params_title="a=2, d=9, 49 zadataka",
    filename="scenarij2_ucinkovitost.png",
)

# Scenarij 3 (Promijenjeno d=10 u d=9 prema stvarnim podacima iz rezultata)
plot_individual_speedup(
    scenarij_num=3,
    speedup_data=s3_speedup,
    params_title="a=3, d=9, 343 zadatka",
    filename="scenarij3_ubrzanje.png",
)
plot_individual_efficiency(
    scenarij_num=3,
    efficiency_data=s3_efficiency,
    params_title="a=3, d=9, 343 zadatka",
    filename="scenarij3_ucinkovitost.png",
)

print(
    f"[INFO] Uspješno pročitani podaci i generirano 6 grafova unutar '{output_dir}/':"
)
print("  - scenarij1_ubrzanje.png")
print("  - scenarij1_ucinkovitost.png")
print("  - scenarij2_ubrzanje.png")
print("  - scenarij2_ucinkovitost.png")
print("  - scenarij3_ubrzanje.png")
print("  - scenarij3_ucinkovitost.png")
