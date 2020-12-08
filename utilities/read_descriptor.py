#!/usr/bin/env python
import sys
import argparse
import textwrap
from utils import bit, bitmask, bitslice, integer, human_number

parser = argparse.ArgumentParser()
parser.add_argument("descriptor", type=integer)
parser.add_argument("--level", type=integer, required=False, default=3)
args = parser.parse_args()

desc = args.descriptor

level3_fields = {
    (63, 50): {
        "name": "UPPER ATTRS",
    },
    (49, 48): {
        "name": "res0",
    },
    (47, 12): {
        "name": "OA",
    },
    (11, 2): {
        "name": "LOWER ATTRS",
    },
    (1, 1): {
        "name": "B",
    },
    (0, 0): {
        "name": "V",
    }
}

def print_fields(value, fields, split_breakdown=False):
    out  = []
    out2 = []
    labels = []

    for k, v in sorted(fields.items(), reverse=True, key=lambda it: it[0]):
        v["value"] = bitslice(value, *k)
        sz = k[0] - k[1] + 1
        out.append(f"{v['value']:>0{sz}b}")
        label = v["name"]

        if not split_breakdown:
            if len(label) > sz and label == "res0":
                label = "0"

            if len(label) > sz:
                label = label[:sz-2]+'..'
                if len(label) > sz:
                    label = '?'*sz

            rem = (sz - len(label))
            rhs = rem // 2
            lhs = rem - rhs
            out2.append(label.rjust(lhs+len(label), ' ') + " "*rhs)
        else:
            out2.append(label)

    print(f"    0b{'|'.join(out)}")

    if split_breakdown:
        maxlen = max(len(v) for v in out2)
        for o1, o2 in zip(out, out2):
            print(f"       {o2:>{maxlen}}: 0b{o1}")
    else:
        print(f"      {'|'.join(out2)}")

print_fields(desc, level3_fields)

if not bit(desc, 0):
    print()
    print("<invalid>")
else:
    print()
    oa = level3_fields[(47,12)]["value"] << 12
    print(f"OA = {human_number(oa, 47-12+1)}")
    # upper
    upper_attrs_fields = {
        (63-50, 63-50): {"name": "IGNORED"},
        (62-50, 59-50): {"name": "PBHA"},
        (58-50, 55-50): {"name": "IGNORED"},
        (54-50, 54-50): {"name": "UXN"},
        (53-50, 53-50): {"name": "PXN"},
        (52-50, 52-50): {"name": "Contiguous"},
        (51-50, 51-50): {"name": "DBM"},
        (50-50, 50-50): {"name": "GP"},
    }
    # lower
    lower_attr_fields = {
        (16, 16): {"name": "nT"},
        (15, 12): {"name": "OA"},
        (11, 11): {"name": "nG"},
        (10, 10): {"name": "AF"},
        (9, 8): {"name": "SH"},
        (7, 6): {"name": "AP"},
        (5, 5): {"name": "NS"},
        (4, 2): {"name": "AttrIndx"},
    }

    print()
    print('UPPER ATTRS:')
    print_fields(desc>>50, upper_attrs_fields, split_breakdown=True)

    print()
    print('LOWER ATTRS:')
    print_fields(desc, lower_attr_fields, split_breakdown=True)