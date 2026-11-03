"""The dice a toy rolls, from its seed (Playbox v0.12's dice.py, verbatim).

A generator is a two-item list, [state, seed], so it can be handed round
and stepped without making new objects. The sum is a 28-bit linear
congruential generator worked in two 14-bit halves, so every number stays
a small integer on the Pico.
"""
import random

_A = 12829                  # multiplier; with an odd step the period is 2^28
_M = 0xFFFFFFF


def start(seed):
    """A generator at the beginning of this seed's run."""
    s = (seed * 9973 + 4057) & _M
    for _ in range(3):                  # stir, so neighbouring seeds part company
        s = (((s >> 14) * _A & 0x3FFF) << 14) + (s & 0x3FFF) * _A + 12345 & _M
    return [s, seed]


_box = start(random.getrandbits(13))    # until a toy is running


def use(box):
    """Roll this generator from now on."""
    global _box
    _box = box


def rand():
    """0 <= r < 1"""
    b = _box
    s = b[0]
    s = (((s >> 14) * _A & 0x3FFF) << 14) + (s & 0x3FFF) * _A + 12345 & _M
    b[0] = s
    return (s >> 12) / 65536


def pick(n):
    """A whole number from 0 to n - 1."""
    b = _box
    s = b[0]
    s = (((s >> 14) * _A & 0x3FFF) << 14) + (s & 0x3FFF) * _A + 12345 & _M
    b[0] = s
    return (s >> 12) * n >> 16


def seed():
    """The seed of the generator in use."""
    return _box[1]


def fresh():
    """A new seed, from the Pico's own (unrepeatable) random numbers."""
    return random.getrandbits(16) * 10000 >> 16


def aside(seed, salt=1):
    """A second generator for the same seed, for choices that must not
    disturb the run of the first (Random picking new knob values)."""
    return start((seed + 10007 * salt) & 0xFFFF)
