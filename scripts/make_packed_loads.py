"""
This module parses a TOML input file and creates a "fake" packed-loads file
for it. This is mainly for testing purposes.
"""

import tomllib
import csv


def main(input_toml_path, output_csv_path):
    """
    Create a packed-loads file from TOML input.
    """
    load_tags = []
    with open(input_toml_path, "rb") as fp:
        data = tomllib.load(fp)
    for comp_tag in data["components"].keys():
        comp = data["components"][comp_tag]
        if comp["type"] == "load":
            load_tags.append(comp_tag)
    with open(output_csv_path, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([item for tag in load_tags for item in [tag, 2]])
        writer.writerow([item for tag in load_tags
                        for item in ["hours", "kW"]])
        writer.writerow([item for tag in load_tags for item in [0, 10]])
        writer.writerow([item for tag in load_tags for item in [4, 0]])


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(
        prog="make_packed_loads",
        description="Create a packed loads file",
    )
    parser.add_argument("source_toml")
    parser.add_argument("-o", "--output", default="packed-loads.csv")
    args = parser.parse_args()
    main(args.source_toml, args.output)
