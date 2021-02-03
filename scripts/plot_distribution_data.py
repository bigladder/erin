import sys

import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd


def print_usage(args):
    print(f"python {args[0]} <datafile.csv> <(optional)units>\n" +
            "   - datafile.csv: path to a datafile\n" +
            "   - units: string, optional; defaults to seconds (supports second, minute, hour, day, week, year)\n" +
            "   This will plot a histogram of the given data and show a plot\n")
    print("Arguments:")
    for idx, a in enumerate(args):
        print(f"- arg[{idx}] = {a}")


seconds_per_hour = 3600
seconds_per_minute = 60
seconds_per_day = seconds_per_hour * 24
seconds_per_week = seconds_per_day * 7
seconds_per_year = seconds_per_hour * 8760


TIME_CONVERSIONS = {
        "hour": seconds_per_hour,
        "hours": seconds_per_hour,
        "hr": seconds_per_hour,
        "hrs": seconds_per_hour,
        "h": seconds_per_hour,
        "minute": seconds_per_minute,
        "minutes": seconds_per_minute,
        "min": seconds_per_minute,
        "mins": seconds_per_minute,
        "m": seconds_per_minute,
        "second": 1,
        "seconds": 1,
        "sec": 1,
        "secs": 1,
        "s": 1,
        "week": seconds_per_week,
        "weeks": seconds_per_week,
        "wk": seconds_per_week,
        "wks": seconds_per_week,
        "w": seconds_per_week,
        "day": seconds_per_day,
        "days": seconds_per_day,
        "d": seconds_per_day,
        "year": seconds_per_year,
        "years": seconds_per_year,
        "yr": seconds_per_year,
        "yrs": seconds_per_year,
        "y": seconds_per_year,
        }


if __name__ == "__main__":
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print_usage(sys.argv)
        sys.exit(1)
    path = sys.argv[1]
    units = "seconds"
    if len(sys.argv) == 3:
        units = sys.argv[2].lower().strip()
    data = pd.read_csv(path)
    if units not in TIME_CONVERSIONS:
        print(f"Unhandled time unit '{units}'")
    multiplier = 1.0 / TIME_CONVERSIONS[units]
    data['data'] = data['data'] * multiplier
    data['data'].hist(bins=100)
    ax = plt.gca()
    ax.set_xlabel(f"Time ({units})")
    ax.set_ylabel(f"Occurrences")
    plt.show()
    print("Done!")
