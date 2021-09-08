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
    parser = argparse.ArgumentParser(description='Convert a binary object to a compressed image.')
    parser.add_argument('--input', type=str, help='Binary object to be compressed', required=True)
    parser.add_argument('--output_dir', type=str, help='Where to save the diferent compressions', required=True)
    args = parser.parse_args()
    return args

def compress(input, output):
    input_filename = input.name

    # first copy the original index
    logging.info("Copying original index...")
    shutil.copy(str(input), output)

    # read the binary object into a numpy array
    logging.info("Reading binary object...")
    input_fh = open(input, "rb")
    data = input_fh.read()
    data = np.frombuffer(data, np.uint8)
    dimension = math.ceil(math.sqrt(data.size))
    data = np.pad(data, (0, dimension*dimension - data.size))
    data = data.reshape((dimension, dimension))

    # serialise the numpy array with no compression
    logging.info("Serialising numpy object...")
    with open(output / f"{input_filename}.npy", "wb") as data_npy_fh:
        np.save(data_npy_fh, data)

    # create image from numpy array
    image = Image.fromarray(data, 'L')

    # compress image using several algorithms
    logging.info("Compressing using PNG...")
    image.save(str(output / f"{input_filename}.png"), 'PNG', optimize=True)
    logging.info("Done!")

    # Note: removed as the BMP format is not suited for very large files:
    # It errors out with ValueError: File size is too large for the BMP format
    # logging.info("Compressing using BMP RLE...")
    # image.save(str(output / f"{input_filename}.bmp_rle.bmp"), 'BMP', compression="bmp_rle")
    # logging.info("Done!")

    tiff_compression_methods = [
        "lzma", "packbits", "tiff_adobe_deflate", "tiff_lzw"]
    for tiff_compression_method in tiff_compression_methods:
        logging.info(f"Compressing using TIFF {tiff_compression_method}...")
        image.save(str(output / f"{input_filename}.{tiff_compression_method}.tiff"), "TIFF", compression=tiff_compression_method)
        logging.info("Done!")


def main():
    args = get_args()
    input = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=False)
    compress(input, output_dir)

main()