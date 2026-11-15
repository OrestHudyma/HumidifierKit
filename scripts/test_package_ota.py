import struct
import unittest
from package_ota import package, metadata

class PackageTests(unittest.TestCase):
    def app(self, chip=16, size=64):
        data = bytearray(size)
        data[0] = 0xE9
        struct.pack_into('<H', data, 12, chip)
        return bytes(data)

    def test_header_and_payload(self):
        app = self.app()
        result = package(app, metadata())
        self.assertEqual(result[:4], bytes.fromhex('1ef1ee0b'))
        self.assertEqual(struct.unpack_from('<H', result, 6)[0], 60)
        self.assertEqual(struct.unpack_from('<I', result, 52)[0], len(result))
        self.assertEqual(struct.unpack_from('<HH', result, 56), (1, 1))
        self.assertEqual(struct.unpack_from('<HI', result, 60), (0, len(app)))
        self.assertEqual(result[66:], app)

    def test_reject_wrong_chip_and_oversize(self):
        for app in (b'', b'not an image', self.app(chip=13), self.app(size=0x140001)):
            with self.subTest(size=len(app)), self.assertRaises(ValueError):
                package(app, metadata())

if __name__ == '__main__':
    unittest.main()
