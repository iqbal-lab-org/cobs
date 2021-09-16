import argparse
from pathlib import Path
import string
import random
import os
import logging
import sys
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)


def get_args():
    parser = argparse.ArgumentParser(description='Reorder genomes WRT to an ordering.')
    parser.add_argument('--input_dir', type=str, help='Path to a dir where each file is a genome', required=True)
    parser.add_argument('--order', type=str, help='Path to a file with ordering', required=True)
    parser.add_argument('--output_dir', type=str, help='Where to save the reordered genomes', required=True)
    args = parser.parse_args()
    return args


def main():
    args = get_args()
    input_dir = Path(args.input_dir)
    with open(args.order) as order_fh:
        order = [line.strip() for line in order_fh.readlines()]
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=False)
    reorder_genomes(input_dir, order, output_dir)


def get_order_to_path(file, order):
    order_to_path = {}
    for order_id, order_str in enumerate(order):
        if order_str in file.name:
            order_to_path[order_id] = file
    file_was_matched_to_at_most_one_entry = len(order_to_path) <= 1
    if not file_was_matched_to_at_most_one_entry:
        raise Exception(f"Error for {file} - it was matched to several entries: {order_to_path}")
    return order_to_path


def get_random_name(length=40):
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for _ in range(length))


def get_n_random_sorted_names(n):
    random_names = set()
    while len(random_names) < n:
        random_names.add(get_random_name())
    return sorted(list(random_names))


def reorder_genomes(input_dir, order, output_dir):
    order_to_path = {}
    for file in input_dir.iterdir():
        logging.info(f"Getting order for file {file}")
        order_to_path_for_this_file = get_order_to_path(file, order)
        order_to_path.update(order_to_path_for_this_file)

    random_names = get_n_random_sorted_names(len(order_to_path))
    for index in range(len(order_to_path)):
        source = order_to_path[index].resolve()
        source_suffixes = "".join(source.suffixes)
        dest = output_dir.resolve() / f"{random_names[index]}{source_suffixes}"
        logging.info(f"{source} -> {dest}")
        os.symlink(str(source), str(dest))


if __name__ == "__main__":
    main()
