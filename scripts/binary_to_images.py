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
    parser.add_argument('--width', type=int, default=4000, help='Width of the image (should equal nb of samples)')
    args = parser.parse_args()
    return args


def compress_core(functor, name):
    logging.info(f"Compressing using {name}...")
    functor()
    logging.info("Done!")


def compress_image(image, output, input_filename, part):
    # compress image using several algorithms
    compress_core(lambda: image.save(str(output / f"{input_filename}.{part}.png"), 'PNG', optimize=True), "PNG")
    tiff_compression_methods = [
        "lzma", "packbits", "tiff_adobe_deflate", "tiff_lzw"]
    for tiff_compression_method in tiff_compression_methods:
        compress_core(lambda: image.save(str(output / f"{input_filename}.{part}.{tiff_compression_method}.tiff"), "TIFF",
                                         compression=tiff_compression_method), f"TIFF {tiff_compression_method}")


def compress(input, output, width):
    input_filename = input.name

    # first copy the original index
    logging.info("Copying original index...")
    shutil.copy(str(input), output)

    # read the binary object into a numpy array
    logging.info("Reading binary object...")
    input_fh = open(input, "rb")
    data = input_fh.read()
    data = np.frombuffer(data, np.uint8)

    height = math.ceil(data.size/width)
    data = np.pad(data, (0, height*width - data.size))
    data = data.reshape((height, width))

    logging.info(f"Data size = {data.size}")
    logging.info(f"Width = {width}")
    logging.info(f"Height = {height}")

    # serialise the numpy array with no compression
    logging.info("Serialising numpy object...")
    with open(output / f"{input_filename}.npy", "wb") as data_npy_fh:
        np.save(data_npy_fh, data)

    # create image from numpy array
    image = Image.fromarray(data, '1')

    compress_image(image, output, input_filename, "all")

    # split and compress
    height_of_each_sub_image = 25_000
    for index in range(0, math.ceil(height/height_of_each_sub_image)):
        image_row_from = index*height_of_each_sub_image
        image_row_to = min(image_row_from+height_of_each_sub_image, height)
        logging.info(f"Cropping image: {(0, image_row_from, width, image_row_to)}")
        sub_image = image.crop((0, image_row_from, width, image_row_to))
        compress_image(sub_image, output, input_filename, f"part_{index}")


def main():
    args = get_args()
    input = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=False)
    compress(input, output_dir, args.width)

main()