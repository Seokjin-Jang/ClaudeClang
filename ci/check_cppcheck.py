#!/usr/bin/env python3
"""Cppcheck XML(version 2)에서 src/ 아래 지적 건수를 세어, 1건이라도 있으면 실패한다.
사용법: check_cppcheck.py <cppcheck.xml>

severity 가 information 인 메시지(missingIncludeSystem, toomanyconfigs 등)는 코드 규칙 위반이 아니라
도구 진단 정보이므로 세지 않는다. error/warning/style/performance/portability 는 모두 센다.
특정 룰 ID 를 제외하는 예외 처리는 두지 않는다.
"""
import sys
import xml.etree.ElementTree as ET


def main(argv):
    result = 0
    if len(argv) != 2:
        print(__doc__)
        result = 2
    else:
        count = 0
        for err in ET.parse(argv[1]).getroot().iter("error"):
            loc = err.find("location")
            path = loc.get("file", "") if loc is not None else ""
            in_src = path.replace("\\", "/").removeprefix("./").startswith("src/")
            if in_src and err.get("severity") != "information":
                count += 1
                print(f"{path}:{loc.get('line')}: [{err.get('severity')}] {err.get('id')}: {err.get('msg')}")
        print(f"[cppcheck] src 지적 {count}건")
        if count:
            result = 1
    return result


if __name__ == "__main__":
    sys.exit(main(sys.argv))
