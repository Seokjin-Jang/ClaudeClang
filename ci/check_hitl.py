#!/usr/bin/env python3
"""Doxygen XML 에서 Human in the Loop 태그가 모든 파일과 함수에 있는지 검사한다.
필수: @ai_model, @ai_date, @ai_requester, @review_status
@review_status 가 Done 이면 @reviewer, @review_date 도 값이 있어야 한다.
사용법: check_hitl.py <doxygen xml 디렉터리>
"""
import glob
import os
import sys
import xml.etree.ElementTree as ET

REQUIRED = ["AI Model:", "AI Date:", "AI Requester:", "Review Status:"]
ON_DONE = ["Reviewer:", "Review Date:"]
EMPTY = {"", "-", "n/a", "N/A", "<요청자>"}


def collect(desc):
    """detaileddescription 요소에서 {제목: 값} 을 모은다."""
    tags = {}
    if desc is not None:
        for sect in desc.iter("simplesect"):
            if sect.get("kind") == "par":
                title = (sect.findtext("title") or "").strip()
                value = " ".join("".join(p.itertext()).strip() for p in sect.findall("para")).strip()
                tags[title] = value
    return tags


def problems(tags):
    found = []
    for name in REQUIRED:
        if tags.get(name, "") in EMPTY:
            found.append(f"{name.rstrip(':')} 누락")
    if tags.get("Review Status:", "").lower() == "done":
        for name in ON_DONE:
            if tags.get(name, "") in EMPTY:
                found.append(f"리뷰 완료인데 {name.rstrip(':')} 누락")
    return found


def main(argv):
    result = 0
    if len(argv) != 2:
        print(__doc__)
        result = 2
    else:
        count = 0
        for path in sorted(glob.glob(os.path.join(argv[1], "*.xml"))):
            root = ET.parse(path).getroot()
            for comp in root.iter("compounddef"):
                if comp.get("kind") != "file":
                    continue
                file_name = (comp.find("location").get("file") if comp.find("location") is not None else "")
                targets = [(f"파일 {file_name}", file_name, collect(comp.find("detaileddescription")))]
                for mem in comp.iter("memberdef"):
                    if mem.get("kind") == "function":
                        loc = mem.find("location")
                        line = loc.get("line") if loc is not None else "?"
                        name = mem.findtext("name")
                        targets.append((f"함수 {name}()", f"{file_name}:{line}", collect(mem.find("detaileddescription"))))
                for label, where, tags in targets:
                    for item in problems(tags):
                        count += 1
                        print(f"{where}: {label}: {item}")
        print(f"[hitl] 지적 {count}건")
        if count:
            result = 1
    return result


if __name__ == "__main__":
    sys.exit(main(sys.argv))
