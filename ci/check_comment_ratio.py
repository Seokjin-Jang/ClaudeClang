#!/usr/bin/env python3
"""CLOC JSON 의 주석률(주석 / (주석 + 코드))이 기준 이상인지 확인한다.
사용법: check_comment_ratio.py <cloc.json> <최소 퍼센트>
"""
import json
import os
import sys


def main(argv):
    result = 0
    if len(argv) != 3:
        print(__doc__)
        result = 2
    elif not os.path.isfile(argv[1]):
        print("[comment] cloc 결과가 없다. 건너뜀.")
    else:
        with open(argv[1], encoding="utf-8") as handle:
            total = json.load(handle).get("SUM")
        if not total:
            print("[comment] 측정 대상 소스가 없다. 건너뜀.")
        else:
            lines = total["comment"] + total["code"]
            ratio = 100.0 * total["comment"] / lines if lines else 0.0
            minimum = float(argv[2])
            print(f"[comment] 주석률 {ratio:.1f}% (기준 {minimum:.0f}% 이상)")
            if ratio < minimum:
                result = 1
    return result


if __name__ == "__main__":
    sys.exit(main(sys.argv))
