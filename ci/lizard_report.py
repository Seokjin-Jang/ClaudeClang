#!/usr/bin/env python3
"""Lizard --warnings_only 출력(stdin)을 Warnings NG 가 읽는 checkstyle XML(stdout)로 변환한다.
Lizard 의 --checkstyle 은 임계값과 무관하게 모든 함수를 보고하므로, 임계값을 넘은 함수만 담기 위해 사용한다.
사용법: lizard <경로> -C 10 -L 80 --warnings_only | lizard_report.py > lizard.xml
"""
import re
import sys
from xml.sax.saxutils import quoteattr

WARNING = re.compile(r"^(?P<file>.+?):(?P<line>\d+): warning: (?P<msg>.*)$")


def main():
    files = {}
    for raw in sys.stdin:
        found = WARNING.match(raw.strip())
        if found:
            files.setdefault(found["file"], []).append((found["line"], found["msg"]))

    print('<?xml version="1.0" encoding="UTF-8"?>')
    print('<checkstyle version="4.3">')
    for name, items in files.items():
        print(f"  <file name={quoteattr(name)}>")
        for line, msg in items:
            print(f'    <error line="{line}" column="0" severity="error" '
                  f'message={quoteattr(msg)} source="lizard"/>')
        print("  </file>")
    print("</checkstyle>")
    return 0


if __name__ == "__main__":
    sys.exit(main())
