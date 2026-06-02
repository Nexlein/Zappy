##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## test_broadcast_protocol
##

import sys
import os
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from BroadcastProtocol import BroadcastProtocol, DecodedBroadcast, MessageType


class TestEncode(unittest.TestCase):
    def test_format(self):
        # encode assemble team|type|level avec la valeur texte de l'enum
        self.assertEqual(
            BroadcastProtocol.encode("team5", MessageType.RALLY, 3), "team5|RALLY|3"
        )

    def test_uses_enum_value(self):
        self.assertEqual(
            BroadcastProtocol.encode("team5", MessageType.INCANT, 1), "team5|INCANT|1"
        )


class TestDecode(unittest.TestCase):
    def test_valid(self):
        decoded = BroadcastProtocol.decode("team5|RALLY|3")
        self.assertEqual(decoded.team_name, "team5")
        self.assertIs(decoded.msg_type, MessageType.RALLY)
        self.assertEqual(decoded.level, 3)

    def test_returns_dataclass(self):
        self.assertIsInstance(
            BroadcastProtocol.decode("team5|ABORT|2"), DecodedBroadcast
        )

    def test_unknown_type_raises(self):
        # un type hors enum (autre équipe / corrompu) est rejeté
        with self.assertRaises(ValueError):
            BroadcastProtocol.decode("team5|XXX|3")

    def test_non_int_level_raises(self):
        with self.assertRaises(ValueError):
            BroadcastProtocol.decode("team5|RALLY|abc")

    def test_too_few_fields_raises(self):
        with self.assertRaises(ValueError):
            BroadcastProtocol.decode("team5|RALLY")

    def test_too_many_fields_raises(self):
        with self.assertRaises(ValueError):
            BroadcastProtocol.decode("team5|RALLY|3|extra")

    def test_empty_raises(self):
        with self.assertRaises(ValueError):
            BroadcastProtocol.decode("")


class TestRoundTrip(unittest.TestCase):
    def test_encode_then_decode(self):
        # symétrie: decode(encode(x)) == x, pour chaque type connu
        for msg_type in MessageType:
            decoded = BroadcastProtocol.decode(
                BroadcastProtocol.encode("team5", msg_type, 7)
            )
            self.assertEqual(decoded.team_name, "team5")
            self.assertIs(decoded.msg_type, msg_type)
            self.assertEqual(decoded.level, 7)


if __name__ == "__main__":
    unittest.main()
