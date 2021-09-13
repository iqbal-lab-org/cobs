from pathlib import Path
from PIL import Image
import numpy as np
import argparse
import shutil
import logging
import sys
import math
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S',
                    format='%(asctime)s %(levelname)-8s %(message)s')


def get_args():
    parser = argparse.ArgumentParser(description='Convert a binary object to a compressed image.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--input', type=str, help='Binary object to be compressed', required=True)
    parser.add_argument('--output_dir', type=str, help='Where to save the diferent compressions', required=True)
    parser.add_argument('--width', type=int, default=4000, help='Width of the image (should equal nb of samples)')
    parser.add_argument('--number_of_rows_per_batch', type=int, default=50000,
                        help='Number of rows to have in each batch')
    args = parser.parse_args()
    return args


def compress_core(functor, name):
    logging.info(f"Compressing using {name}...")
    functor()
    logging.info("Done!")


def compress_image(image, output, input_filename, part, use_tiff_methods=False):
    # compress image using several algorithms
    compress_core(lambda: image.save(str(output / f"{input_filename}.{part}.png"), 'PNG', optimize=True), "PNG")
    tiff_compression_methods = ["lzma", "packbits", "tiff_adobe_deflate", "tiff_lzw"] if use_tiff_methods else []
    for tiff_compression_method in tiff_compression_methods:
        compress_core(lambda: image.save(str(output / f"{input_filename}.{part}.{tiff_compression_method}.tiff"),
                                         "TIFF",
                                         compression=tiff_compression_method), f"TIFF {tiff_compression_method}")


def compress(input_index, output, width, number_of_rows_per_batch):
    input_filename = input_index.name

    logging.info("Copying original index...")
    shutil.copy(str(input_index), output)

    logging.info("Reading binary object...")
    input_fh = open(input_index, "rb")
    data = input_fh.read()

    logging.info("Extracting and writing header...")
    first_classic_index_pos = data.index(b"CLASSIC_INDEX")
    second_classic_index_pos = data.index(b"CLASSIC_INDEX", first_classic_index_pos+1)
    bf_matrix_start_pos = second_classic_index_pos + len(b"CLASSIC_INDEX")
    cobs_header = data[:bf_matrix_start_pos]
    with open(output / f"{input_filename}.cobs_header.bin", "wb") as cobs_header_fh:
        cobs_header_fh.write(cobs_header)

    logging.info("Compressing bloom filter data...")
    bf_data = data[bf_matrix_start_pos:]
    bf_data = np.frombuffer(bf_data, np.uint8)
    height = math.ceil(bf_data.size/width)
    bf_data = np.pad(bf_data, (0, height*width - bf_data.size))
    bf_data = bf_data.reshape((height, width))
    logging.info(f"Bloom filter size = {bf_data.size}")
    logging.info(f"Width = {width}")
    logging.info(f"Height = {height}")

    logging.info("Serialising numpy object...")
    with open(output / f"{input_filename}.npy", "wb") as data_npy_fh:
        np.save(data_npy_fh, bf_data)

    logging.info("Creating image from numpy matrix...")
    image = Image.fromarray(bf_data, '1')

    compress_image(image, output, input_filename, "all")

    # split and compress
    for index in range(0, math.ceil(height / number_of_rows_per_batch)):
        image_row_from = index * number_of_rows_per_batch
        image_row_to = min(image_row_from + number_of_rows_per_batch, height)
        logging.info(f"Cropping image: {(0, image_row_from, width, image_row_to)}")
        sub_image = image.crop((0, image_row_from, width, image_row_to))
        compress_image(sub_image, output, input_filename, f"part_{index}")


def main():
    args = get_args()
    input_index = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=False)
    compress(input_index, output_dir, args.width, args.number_of_rows_per_batch)


main()
