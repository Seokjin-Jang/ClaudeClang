#!/usr/bin/env python3
"""reports/ 의 결과를 모아 Markdown 요약을 stdout 으로 출력한다. (GitHub Actions Job Summary 용)
사용법: summary.py [reports 디렉터리]
"""
import json
import os
import sys
import xml.etree.ElementTree as ET

SUITES = [("unit", "단위 테스트", "mcdc"), ("integration", "통합 테스트", "functions"), ("tdd", "TDD 테스트", None)]


def count_tests(path):
    tests = failures = 0
    if os.path.isfile(path):
        root = ET.parse(path).getroot()
        tests = int(root.get("tests", 0))
        failures = int(root.get("failures", 0)) + int(root.get("errors", 0))
    return tests, failures


def coverage_text(path, metric):
    text = "-"
    if metric and os.path.isfile(path):
        with open(path, encoding="utf-8") as handle:
            entry = json.load(handle)["data"][0]["totals"].get(metric)
        if entry:
            text = f"{metric} {entry['percent']:.2f}% ({entry['covered']}/{entry['count']})"
    return text


def count_cppcheck(path):
    count = 0
    if os.path.isfile(path):
        for err in ET.parse(path).getroot().iter("error"):
            loc = err.find("location")
            file_name = loc.get("file", "") if loc is not None else ""
            if file_name.replace("\\", "/").removeprefix("./").startswith("src/") and err.get("severity") != "information":
                count += 1
    return count


def count_lizard(path):
    count = 0
    if os.path.isfile(path):
        count = len(list(ET.parse(path).getroot().iter("error")))
    return count


def main(argv):
    reports = argv[1] if len(argv) > 1 else "reports"
    print("## CI 결과\n")
    print("| 테스트 | 실행 | 실패 | 커버리지(기준 지표) |")
    print("|---|---|---|---|")
    for suite, label, metric in SUITES:
        tests, failures = count_tests(os.path.join(reports, suite, "results.xml"))
        cov = coverage_text(os.path.join(reports, suite, "export.json"), metric)
        print(f"| {label} | {tests} | {failures} | {cov} |")
    print()
    print("| 정적 분석/메트릭 | 지적 |")
    print("|---|---|")
    print(f"| Cppcheck (src) | {count_cppcheck(os.path.join(reports, 'cppcheck.xml'))} |")
    print(f"| Lizard (CCN>10 또는 길이>80) | {count_lizard(os.path.join(reports, 'lizard.xml'))} |")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
