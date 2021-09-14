import unittest
from scripts.binary_to_images import compress_bloom_filter_data, DataNotAlignable, split_and_compress
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
        image, height, _ = compress_bloom_filter_data(self.bf_data, 8, "test_full_no_padding.temp")
        split_and_compress(image, 8, height, 3, "test_full_no_padding.parts.out")
        for i in range(6):
            self.assertTrue(filecmp.cmp(f"test_full_no_padding.parts.part_{i}.png",
                                        f"test_full_no_padding.parts.out.part_{i}.png"))
