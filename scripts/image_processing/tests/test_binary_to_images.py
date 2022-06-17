import unittest
from scripts.binary_to_images import compress_bloom_filter_data, DataNotAlignable, split_and_compress, create_transposed_bf_matrix
import numpy as np
import filecmp

class TestBinaryToImage(unittest.TestCase):
    def setUp(self):
        self.bf_data = np.array([0, 10, 20, 30, 40, 50, 60, 70, 200, 210, 0, 0, 0, 0, 0, 255], dtype=np.uint8)

    def test___compress_bloom_filter_data___no_padding(self):
        compress_bloom_filter_data(self.bf_data, 8, "test_full_no_padding.out")
        self.assertTrue(filecmp.cmp("test_full_no_padding.out.png",
                                    "test_full_no_padding.png"))

    def test___compress_bloom_filter_data___with_padding(self):
        compress_bloom_filter_data(self.bf_data, 80, "test_full_with_padding.out")
        self.assertTrue(filecmp.cmp("test_full_with_padding.out.png",
                                    "test_full_with_padding.png"))

    def test___compress_bloom_filter_data___width_is_not_alignable(self):
        with self.assertRaises(DataNotAlignable):
            compress_bloom_filter_data(self.bf_data, 10, "test_full_with_padding.out")

    def test___split_and_compress(self):
        _, image, height, _ = compress_bloom_filter_data(self.bf_data, 8, "test_full_no_padding.temp")
        split_and_compress(image, 8, height, 3, "test_full_no_padding.parts.out")
        for i in range(6):
            self.assertTrue(filecmp.cmp(f"test_full_no_padding.parts.part_{i}.png",
                                        f"test_full_no_padding.parts.out.part_{i}.png"))

    def test___create_transposed_bf_matrix(self):
        bf_data, _, _, _ = compress_bloom_filter_data(self.bf_data, 8, "test_full_no_padding.out")
        with open("test_create_transposed_bf_matrix.out", "wb") as fout:
            create_transposed_bf_matrix(bf_data, fout)

        with open("test_create_transposed_bf_matrix.out", "rb") as fin:
            transposed_bf_data = fin.read()
        transposed_bf_data = np.frombuffer(transposed_bf_data, np.uint8)
        transposed_bf_data = np.unpackbits(transposed_bf_data)
        transposed_bf_data = transposed_bf_data.astype(bool)
        transposed_bf_data = transposed_bf_data.reshape((8, 16))
        untransposed_bf_data = np.transpose(transposed_bf_data)
        untransposed_bf_data = np.packbits(untransposed_bf_data)

        self.assertTrue(np.array_equal(self.bf_data, untransposed_bf_data))
