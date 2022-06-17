import argparse
from pathlib import Path


def get_args():
    parser = argparse.ArgumentParser(
        description='Goes through a ordering and remove samples that are not high quality.')
    parser.add_argument('--high_quality', type=str, help='Path to a file with high quality genomes', required=True)
    parser.add_argument('--order_dir', type=str, help='Path to a dir with all the orderings', required=True)
    parser.add_argument('--output_dir', type=str, help='Path to a dir with the updated orderings', required=True)
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    with open(args.high_quality) as high_quality_fh:
        high_quality_genomes = set([line.strip() for line in high_quality_fh])

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    for file in Path(args.order_dir).iterdir():
        if file.suffix == ".txt":
            with open(file) as input_filehandler, open(output_dir / file.name, "w") as output_filehandler:
                for genome in input_filehandler:
                    genome = genome.strip()
                    if genome in high_quality_genomes:
                        print(genome, file=output_filehandler)


if __name__ == "__main__":
    main()
