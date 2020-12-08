import textwrap
import functools

def bit(x, i):
    return (x >> i) & 1


def bitmask(n):
    return (1 << n) - 1


def bitslice(x, j, i):
    return (x >> i) & bitmask(1 + j - i)

def wrap(s, prefix):
    n = len(prefix)
    lines = s.splitlines()
    lines = [l2.strip() + "." for l1 in lines for l2 in l1.split(".") if l2.strip()]
    words = "\n".join(lines)
    return textwrap.indent(words, prefix=" " * n)[n:]

def integer(i):
    funcs = {
        '0x': functools.partial(int, base=16),
        '0o': functools.partial(int, base=8),
        '0b': functools.partial(int, base=2),
    }

    for k, f in funcs.items():
        if i.startswith(k):
            return f(i)

    return int(i)

def human_number(n, bitlen):
    nbits = bitlen
    nhex = (3 + nbits) // 4
    ndec = int(round(0.5 + nbits * 0.301))
    nums = []

    if bitlen == 1:
        nums.append(f"{n}")
    elif bitlen > 1:
        nums.append(f"0b{n:>0{nbits}b}")
        nums.append(f"0x{n:>0{nhex}x}")
        nums.append(f"0d{n:>0{ndec}}")
    return " / ".join(nums)