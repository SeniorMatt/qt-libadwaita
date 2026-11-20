# SPDX-License-Identifier: GPL-2.0-or-later
"""The one place that says which token file is active.

data/current holds the file name (relative to data/) of the active libadwaita token JSON, e.g. "adwaita-1.10.json".
CMake, gen_tokens.py and gen_kde_colors.py all read it. Switching version = change that one line
(tools/update_libadwaita.py --apply does it)."""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data")


def active_name():
    """File name of the active token JSON, e.g. 'adwaita-1.10.json'."""
    with open(os.path.join(DATA, "current")) as f:
        return f.read().strip()


def active_path():
    return os.path.join(DATA, active_name())
