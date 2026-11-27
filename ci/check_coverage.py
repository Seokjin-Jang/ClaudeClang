#!/usr/bin/env python3
"""llvm-cov export JSON 의 합계가 기준 이상인지 확인한다.
사용법: check_coverage.py <export.json> <metric> <threshold>
metric: functions | lines | regions | branches | mcdc
"""
import json
import sys


def main(argv):
    result = 0
    if len(argv) != 4:
        print(__doc__)
        result = 2
    else:
        path, metric, threshold = argv[1], argv[2], float(argv[3])
        with open(path, encoding="utf-8") as handle:
            totals = json.load(handle)["data"][0]["totals"]
        if metric not in totals:
            print(f"[coverage] 지표 '{metric}' 가 결과에 없다. (가능: {', '.join(totals)})")
            result = 1
        else:
            entry = totals[metric]
            print(f"[coverage] {metric}: {entry['covered']}/{entry['count']} = "
                  f"{entry['percent']:.2f}% (기준 {threshold:.0f}%)")
            if entry["count"] == 0:
                print(f"[coverage] 측정 대상 {metric} 이 0건이다.")
                result = 1
            elif entry["percent"] < threshold:
                result = 1
    return result


if __name__ == "__main__":
    sys.exit(main(sys.argv))
