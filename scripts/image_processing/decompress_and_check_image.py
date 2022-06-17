import PIL.Image
from PIL import Image
import numpy as np
import argparse
import logging
import sys
import filecmp

# avoids error:
# PIL.Image.DecompressionBombError: Image size (200000000 pixels) exceeds limit of 178956970 pixels, could be decompression bomb DOS attack.
# TODO: should we really avoid this error?
Image.MAX_IMAGE_PIXELS = None  # avoids

logging.basicConfig(stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S',
                    format='%(asctime)s %(levelname)-8s %(message)s')


def get_args():
    parser = argparse.ArgumentParser(description='Decompress an image and check if it is identical to a COBS index.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--image', type=str, help='Compressed image', required=True)
    parser.add_argument('--header', type=str, help='.cobs_header.bin file gerated with compression', required=True)
    parser.add_argument('--metadata', type=str, help='.metadata file gerated with compression', required=True)
    parser.add_argument('--cobs_index', type=str, help='Original uncompressed COBS index', required=True)
    args = parser.parse_args()
    return args


def get_bf_size(metadata_fh):
    for line in metadata_fh:
        key, equals, value = line.strip().split()
        if key == "BF_size":
            return int(value)
    raise RuntimeError("BF_size key not found")


def decompress(image_fh, bf_size):
    # convert image to numpy array, from https://www.pluralsight.com/guides/importing-image-data-into-numpy-arrays
    data = np.asarray(image_fh)
    data = data.flatten()
    data = data[:bf_size]
    data = np.packbits(data) # from bools to array of np_uint8
    data = bytes(data)
    return data


def are_COBS_indexes_identical(compressed_image_filepath, header_filepath, metadata_filepath, cobs_index_filepath):
    with open(metadata_filepath) as metadata_fh:
        bf_size = get_bf_size(metadata_fh)

    with PIL.Image.open(compressed_image_filepath) as image_fh:
        decompressed_image = decompress(image_fh, bf_size)

    with open(header_filepath, 'rb') as header_fh:
        header = header_fh.read()

    with open(f"{cobs_index_filepath}.decompressed", 'wb') as decompressed_index_fh:
        decompressed_index_fh.write(header)
        decompressed_index_fh.write(decompressed_image)

    cobs_indexes_are_identical = filecmp.cmp(cobs_index_filepath, f"{cobs_index_filepath}.decompressed")
    return cobs_indexes_are_identical


def main():
    args = get_args()
    cobs_indexes_are_identical = are_COBS_indexes_identical(args.image, args.header, args.metadata, args.cobs_index)
    if cobs_indexes_are_identical:
        print("COBS indexes are identical!")
    else:
        print("COBS indexes are >>>NOT<<< identical!")


if __name__ == "__main__":
    main()
