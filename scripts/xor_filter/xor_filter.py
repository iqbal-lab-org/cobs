import argparse
import numpy as np

def get_args():
    parser = argparse.ArgumentParser(description='Apply a xor filter to a binary file.')
    parser.add_argument('--input_file', type=str, required=True)
    parser.add_argument('--output_file', type=str, required=True)
    args = parser.parse_args()
    return args

def main():
    args = get_args()

    with open(args.input_file, "rb") as fin:
        data = fin.read()

    data = np.frombuffer(data, np.uint8)
    data = np.unpackbits(data)
    original_data = data
    shifted_data = data.copy()
    shifted_data = np.concatenate((shifted_data[1:], shifted_data[0:1]))

    xored_data = np.logical_xor(original_data, shifted_data)
    xored_data = xored_data[:-1]
    xored_data_packed = bytes(np.packbits(xored_data))

    with open(args.output_file, "wb") as fout:
        fout.write(xored_data_packed)

if __name__ == '__main__':
    main()
