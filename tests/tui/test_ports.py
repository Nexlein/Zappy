##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test port allocator
##

import os
import socket
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.ports import PortAllocator, PortError


class TestPortAllocator(unittest.TestCase):
    def test_allocates_distinct_ports(self):
        alloc = PortAllocator(8000, 9000)
        a = alloc.allocate()
        b = alloc.allocate()
        self.assertNotEqual(a, b)
        self.assertTrue(8000 <= a < 9000)
        self.assertTrue(8000 <= b < 9000)

    def test_reserved_port_is_skipped(self):
        alloc = PortAllocator(8000, 9000)
        first = alloc.allocate()
        # next call must not hand back a port already reserved.
        self.assertNotEqual(alloc.allocate(), first)

    def test_release_allows_reuse(self):
        # single-port range: the only port must come back after release.
        alloc = PortAllocator(8123, 8124)
        port = alloc.allocate()
        with self.assertRaises(PortError):
            alloc.allocate()
        alloc.release(port)
        self.assertEqual(alloc.allocate(), port)

    def test_release_unknown_port_is_noop(self):
        alloc = PortAllocator(8000, 9000)
        alloc.release(9999)  # never allocated — must not raise

    def test_occupied_port_is_skipped(self):
        # bind a real socket, then build an allocator over only that port:
        # the live probe must see it taken and exhaust the range.
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as taken:
            taken.bind(("", 0))
            taken.listen()
            port = taken.getsockname()[1]
            alloc = PortAllocator(port, port + 1)
            with self.assertRaises(PortError):
                alloc.allocate()

    def test_exhausted_range_raises(self):
        alloc = PortAllocator(8200, 8202)  # two ports
        alloc.allocate()
        alloc.allocate()
        with self.assertRaises(PortError):
            alloc.allocate()

    def test_invalid_range_raises(self):
        with self.assertRaises(ValueError):
            PortAllocator(0, 100)
        with self.assertRaises(ValueError):
            PortAllocator(9000, 8000)
        with self.assertRaises(ValueError):
            PortAllocator(8000, 70000)  # above 65535


if __name__ == "__main__":
    unittest.main()
