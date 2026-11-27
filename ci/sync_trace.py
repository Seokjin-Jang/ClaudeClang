#!/usr/bin/env python3
"""소스·테스트의 태그에서 trace.csv 의 modules 와 시험 열을 도출해 갱신한다.

- 모듈: src/ 함수 주석의 @requirement 와 파일 머리말의 @module
- 시험: tests/unit, tests/integration, tests/system 의 @test 와 @requirement
status 가 planned 이고 도출된 시험이 있는 요구사항은 --status 로 지정한 상태로 바꾼다.
사용법: sync_trace.py [--status implemented|verified] [저장소 루트]
"""
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_traceability as trace  # noqa: E402


def main(argv):
    args = argv[1:]
    status = None
    if args[:1] == ["--status"] and len(args) >= 2:
        status, args = args[1], args[2:]
    root = args[0] if args else "."
    path = os.path.join(root, "docs/traceability/trace.csv")
    rows = trace.read_csv(path)
    by_req, _problems, file_modules = trace.derive_modules(root)
    tests = trace.derive_tests(root)
    code_modules = {mod for name, mods in file_modules.items() if name.endswith(".c") for mod in mods}
    for row in rows:
        req = row["req_id"]
        derived_tests = tests.get(req, {})
        modules = by_req.get(req, set())
        if not modules and row["category"].startswith("제약사항") and row["iteration"] == "1":
            modules = code_modules
        if not modules and not derived_tests:
            continue
        row["modules"] = ";".join(sorted(modules))
        for _label, column in trace.LEVELS:
            row[column] = ";".join(sorted(derived_tests.get(column, set())))
        if status and row["status"] == "planned":
            row["status"] = status
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"[sync_trace] {len(rows)}건 갱신")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
