#!/usr/bin/env python3
"""요구사항 추적성과 시스템 테스트 요구사항 커버리지를 검사한다.

검사 항목:
  1. 모든 요구사항에 출처(OEM-* 또는 가정 A-nn)가 있고, A-nn 은 assumptions.md 에 존재한다.
  2. trace.csv 의 OEM 출처와 oem_trace.csv 의 SWR 연결이 양방향으로 일치한다.
  3. 요구사항 문서(safety-requirements.md, software-requirements.md)의 출처·ASIL 열이 trace.csv 와 같고,
     본문에서 인용한 A-nn 이 모두 출처 열에 있다.
  4. 안전 요구사항(type 기능(안전))은 ASIL B 이며 safety-requirements.md 에만, 나머지는 software-requirements.md 에만 있다.
  5. 모든 요구사항에 정의된 컴포넌트(README 표)가 연결되어 있다.
  6. 문서의 검증 열의 단위/통합/시스템과 trace.csv 의 시험 연결(예약 포함)이 양방향으로 일치한다.
     머리말의 안전 요구사항 ID 목록이 안전 문서 표와 같고, 본문의 이유/경고코드가 SRS 부록 A 에 있다.
  7. status 가 implemented 이상이면 모듈이 연결되어 있고 연결된 시험 ID(@test)가 tests/ 에 실제로 있다.
  8. tests/ 의 모든 @test 는 trace.csv 에 연결되어 있고, 그 시험의 @requirement 태그와 trace.csv 가 일치한다.
  9. 요구사항 커버리지: verified 요구사항의 100% 가 시스템 시험에 연결되어 있다. (시스템 시험 요구사항 커버리지 게이트)
사용법: check_traceability.py [저장소 루트]
"""
import csv
import os
import re
import sys

TEST_TAG = re.compile(r"@test\s+([A-Z0-9_\-]+)")
REQ_TAG = re.compile(r"@requirement\s+([A-Za-z0-9_\-;, ]+)")
TEST_FILES = (".c", ".cpp", ".h", ".hpp")
BUILT = ("implemented", "verified")
LEVELS = (("단위", "unit_tests"), ("통합", "integration_tests"), ("시스템", "system_tests"))


def split(cell):
    return [item.strip() for item in re.split(r"[;,]", cell or "") if item.strip()]


def read_csv(path):
    with open(path, encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def read_text(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def doc_rows(text):
    """마크다운 표에서 첫 열이 SWR-nnn 인 행을 {ID: 셀 목록} 으로 읽는다."""
    rows = {}
    for line in text.splitlines():
        if re.match(r"^\|\s*SWR-\d+\s*\|", line):
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            rows[cells[0]] = cells
    return rows


def scan_tests(root):
    """tests/ 아래 @test 블록에서 {시험ID: {요구사항 ID}} 를 구한다."""
    found = {}
    for base, _dirs, files in os.walk(os.path.join(root, "tests")):
        for name in files:
            if name.endswith(TEST_FILES):
                text = read_text(os.path.join(base, name))
                for block in re.findall(r"/\*\*.*?\*/", text, re.S):
                    test = TEST_TAG.search(block)
                    if test:
                        reqs = set()
                        for match in REQ_TAG.finditer(block):
                            reqs.update(split(match.group(1)))
                        found[test.group(1)] = reqs
    return found


def check_sources(rows, oem_rows, assumptions, errors):
    ids = {row["req_id"] for row in rows}
    oem_map = {row["oem_id"]: set(split(row["swr_ids"])) for row in oem_rows}
    for oem_id, swrs in oem_map.items():
        if not swrs:
            errors.append(f"{oem_id}: 연결된 SWR 없음")
        for swr in swrs - ids:
            errors.append(f"{oem_id}: 존재하지 않는 {swr}")
    for row in rows:
        req = row["req_id"]
        sources = split(row["source"])
        if not sources:
            errors.append(f"{req}: 출처 없음")
        for src in sources:
            if src.startswith("A-"):
                if not re.search(rf"^\|\s*{re.escape(src)}\s*\|", assumptions, re.M):
                    errors.append(f"{req}: 가정 {src} 가 assumptions.md 에 없음")
            elif src.startswith("OEM-"):
                if src not in oem_map:
                    errors.append(f"{req}: 출처 {src} 가 oem_trace.csv 에 없음")
                elif req not in oem_map[src]:
                    errors.append(f"{req}: 출처 {src} 인데 oem_trace.csv 의 {src} 가 {req} 를 연결하지 않음")
            else:
                errors.append(f"{req}: 출처 {src} 는 OEM-* 또는 A-nn 이어야 함")
    for oem_id, swrs in oem_map.items():
        for row in rows:
            if row["req_id"] in swrs and oem_id not in split(row["source"]):
                errors.append(f"{row['req_id']}: oem_trace.csv 가 {oem_id} 를 연결하지만 trace.csv 출처에 없음")


def check_docs(rows, safety, srs, errors):
    """요구사항 문서와 trace.csv 의 일치, 안전/일반 분리를 검사한다."""
    ids = {row["req_id"] for row in rows}
    for row in rows:
        req = row["req_id"]
        in_safety, in_srs = req in safety, req in srs
        if row["type"] == "기능(안전)":
            if row["asil"] != "B":
                errors.append(f"{req}: 안전 요구사항인데 ASIL 이 B 가 아님")
            if not in_safety:
                errors.append(f"{req}: 안전 요구사항이 safety-requirements.md 에 없음")
            if in_srs:
                errors.append(f"{req}: 안전 요구사항이 software-requirements.md 에도 있음")
        else:
            if not in_srs:
                errors.append(f"{req}: software-requirements.md 에 없음")
            if in_safety:
                errors.append(f"{req}: 안전 문서에도 있는데 type 이 기능(안전) 이 아님")
        cells = safety.get(req) or srs.get(req)
        if cells:
            if set(split(cells[1])) != set(split(row["source"])):
                errors.append(f"{req}: 문서 출처 {split(cells[1])} 와 trace.csv {split(row['source'])} 불일치")
            if cells[2] != row["asil"]:
                errors.append(f"{req}: 문서 ASIL {cells[2]} 와 trace.csv {row['asil']} 불일치")
            cited = set(re.findall(r"\bA-\d+\b", " ".join(cells[2:])))
            missing = cited - set(split(cells[1]))
            if missing:
                errors.append(f"{req}: 본문이 인용한 {sorted(missing)} 가 출처 열에 없음")
    for req in (set(safety) | set(srs)) - ids:
        errors.append(f"{req}: 문서에는 있으나 trace.csv 에 없음")


def header_ids(text, marker):
    """머리말 줄에서 marker 뒤의 SWR 번호 목록(범위 120~126 포함)을 집합으로 읽는다."""
    found = set()
    for line in text.splitlines():
        if marker in line:
            tail = line.split(marker, 1)[1]
            for lo, hi in re.findall(r"(\d+)~(\d+)", tail):
                found.update(f"SWR-{n:03d}" for n in range(int(lo), int(hi) + 1))
            tail = re.sub(r"\d+~\d+", "", tail)
            found.update(f"SWR-{int(n):03d}" for n in re.findall(r"(?<![\w-])(?:SWR-)?(\d{3})(?!\d)", tail))
    return found


def check_headers(safety_text, srs_text, safety, errors):
    table = set(safety)
    for label, text, marker in (("safety-requirements.md", safety_text, "이 문서의 요구사항 ID:"),
                                ("software-requirements.md", srs_text, "ASIL B 요구(")):
        listed = header_ids(text, marker)
        if not listed:
            errors.append(f"{label}: 머리말에 안전 요구사항 ID 목록이 없음")
        for req in sorted(table - listed):
            errors.append(f"{label}: 머리말 ID 목록에 {req} 가 없음 (안전 문서 표에는 있음)")
        for req in sorted(listed - table):
            errors.append(f"{label}: 머리말 ID 목록의 {req} 가 안전 문서 표에 없음")


# 본문에 쓰이는 대문자 용어 중 코드 집합이 아닌 것(값 이름, 약어, 표준 이름). 새 용어는 여기에 명시적으로 추가한다.
GENERIC_TERMS = {"LOCK", "RELEASE", "CONFIRMED", "PENDING", "TRUE", "FALSE",
                 "SW", "SWR", "SRS", "OEM", "ISO", "IF", "SHA", "QM", "C17", "C_STANDARD",
                 "ASIL", "BL", "CLAUDE", "DC", "MC", "FR", "NFR", "SR", "ID", "ISOFIX"}
# 백틱 안에서 쓰는 C 언어 키워드·함수·수식 기호 (식별자 정의표에 넣지 않는 것)
CODE_WORDS = {"malloc", "calloc", "realloc", "free", "return", "void", "double", "us", "llround", "t", "t_s", "md"}
TICK = re.compile(r"`([^`]+)`")


def appendix_sets(srs_text):
    """SRS 부록 A 에서 집합과 매핑 표를, 부록 C 에서 식별자·열거값 정의를 읽는다."""
    appendix = srs_text.split("## 부록 A", 1)[-1].split("## 부록 B", 1)[0]
    tick = re.compile(r"`([A-Z][A-Z0-9_]*)`")
    sets, mapping = {}, []
    for line in appendix.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 2 and line.startswith("|"):
            for key, name in (("상태", "state"), ("입력 유효성", "validity"), ("이유코드", "reason"),
                              ("경고코드", "warning"), ("우선순위 사유", "priority")):
                if cells[0].startswith(key):
                    sets[name] = set(tick.findall(cells[1]))
            if len(cells) >= 3 and re.fullmatch(r"`[A-Z_]+`", cells[0]):
                mapping.append((tick.findall(cells[0]), tick.findall(cells[1]), cells[2]))
    defs = {"ids": set(), "enums": set(), "members": set()}
    for line in srs_text.split("## 부록 C", 1)[-1].splitlines() if "## 부록 C" in srs_text else []:
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 3 and line.startswith("|") and cells[0].startswith("`"):
            defs["ids"].update(TICK.findall(cells[0]))
            for value in TICK.findall(cells[1]):
                if "/" in value:
                    defs["enums"].add(value)
                    defs["members"].update(value.split("/"))
    return sets, mapping, defs


def assumption_rows(text):
    """가정 문서 표에서 첫 열이 A-nn 인 행을 {ID: 셀 목록} 으로 읽는다."""
    rows = {}
    for line in text.splitlines():
        if re.match(r"^\|\s*A-\d+\s*\|", line):
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            rows[cells[0]] = cells
    return rows


def check_mapping(mapping, sets, trace_ids, docs, errors):
    """부록 A 매핑 표: 집합과의 일치, 근거 요구사항의 실재와 이유코드 사용 여부."""
    reasons = sets.get("reason", set())
    priorities = sets.get("priority", set())
    mapped = {item for _, right, _ in mapping for item in right}
    for token in sorted(mapped - reasons):
        errors.append(f"SRS 부록 A 매핑 표: reason_code `{token}` 가 이유코드 집합에 없음")
    for token in sorted(reasons - mapped):
        errors.append(f"SRS 부록 A 매핑 표: 이유코드 `{token}` 가 매핑 표에 없음")
    left = {item for lefts, _, _ in mapping for item in lefts}
    for token in sorted(left - priorities):
        errors.append(f"SRS 부록 A 매핑 표: priority_reason `{token}` 가 우선순위 사유 집합에 없음")
    for token in sorted(priorities - left):
        errors.append(f"SRS 부록 A 매핑 표: 우선순위 사유 `{token}` 가 매핑 표에 없음")
    for lefts, rights, basis in mapping:
        for swr in re.findall(r"SWR-\d+", basis):
            if swr not in trace_ids:
                errors.append(f"SRS 부록 A 매핑 표: 근거 {swr} 가 trace.csv 에 없음 ({lefts})")
            elif not any(code in " ".join(docs[swr][3:]) for code in rights):
                errors.append(f"SRS 부록 A 매핑 표: 근거 {swr} 의 본문에 이유코드 {rights} 가 없음")


def check_tokens(label, cells, allowed_upper, defs, errors):
    """한 행의 본문에서 대문자 용어, 백틱 식별자, 열거값이 정의된 것인지 검사한다."""
    text = " ".join(cells)
    for token in sorted(set(re.findall(r"(?<![A-Za-z0-9_])[A-Z][A-Z0-9_]+(?![A-Za-z0-9_])", text))):
        if token not in allowed_upper:
            errors.append(f"{label}: 본문의 대문자 용어 `{token}` 가 부록 A 의 집합에도 허용 용어에도 없음")
    known = defs["ids"] | defs["members"] | CODE_WORDS
    for tick in sorted(set(TICK.findall(text))):
        if re.fullmatch(r"[A-Za-z_]+(?:/[A-Za-z_]+)+", tick):
            if tick not in defs["enums"]:
                errors.append(f"{label}: 열거값 `{tick}` 가 SRS 부록 C 에 정의되지 않음")
            continue
        if tick.startswith("-") or tick == "C_STANDARD 17" or re.fullmatch(r"[A-Z0-9_ ]+", tick) or tick in defs["ids"]:
            continue
        for word in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", tick):
            if word not in known and not re.fullmatch(r"[A-Z0-9_]+", word):
                errors.append(f"{label}: 식별자 `{word}` 가 SRS 부록 C 에 정의되지 않음")


def check_codes(srs_text, safety, srs, assumptions, trace_ids, errors):
    """부록 A, C 의 정의와 요구사항·가정 문서 본문의 용어가 일치하는지 검사한다."""
    sets, mapping, defs = appendix_sets(srs_text)
    for name in ("state", "validity", "reason", "warning", "priority"):
        if not sets.get(name):
            errors.append(f"SRS 부록 A: {name} 집합 행을 찾을 수 없음")
    if not defs["ids"]:
        errors.append("SRS 부록 C: 식별자 정의표를 찾을 수 없음")
    check_mapping(mapping, sets, trace_ids, {**safety, **srs}, errors)
    allowed = GENERIC_TERMS | sets.get("priority", set()) | sets.get("reason", set()) | sets.get("state", set()) \
        | sets.get("validity", set()) | sets.get("warning", set())
    for req, cells in {**safety, **srs}.items():
        check_tokens(req, cells[3:], allowed, defs, errors)
    for ident, cells in assumptions.items():
        check_tokens(ident, cells[1:], allowed, defs, errors)


def check_links(rows, docs, components, tests, errors):
    mapped = {}
    for row in rows:
        req = row["req_id"]
        built = row["status"] in BUILT
        comps = split(row["components"])
        if not comps:
            errors.append(f"{req}: components 연결 없음")
        for comp in comps:
            if comp not in components:
                errors.append(f"{req}: 컴포넌트 {comp} 가 README 의 컴포넌트 표에 정의되지 않음")
        verification = docs.get(req, [""])[-1]
        for label, column in LEVELS:
            if label in verification and not split(row[column]):
                errors.append(f"{req}: 검증에 {label} 이 있는데 {column} 연결 없음")
            if label not in verification and split(row[column]):
                errors.append(f"{req}: 검증에 {label} 이 없는데 {column} 에 시험이 연결됨")
        if built and not split(row["modules"]):
            errors.append(f"{req}: modules 연결 없음")
        for _label, column in LEVELS:
            for test in split(row[column]):
                mapped.setdefault(test, set()).add(req)
                if built and test not in tests:
                    errors.append(f"{req}: {column} 의 {test} 가 tests/ 에 없음")
    for test, reqs in tests.items():
        if test not in mapped:
            errors.append(f"{test}: tests/ 에 있으나 trace.csv 에 연결되지 않음")
        elif not reqs:
            errors.append(f"{test}: @requirement 태그 없음")
        elif reqs != mapped[test]:
            errors.append(f"{test}: @requirement {sorted(reqs)} 와 trace.csv {sorted(mapped[test])} 불일치")


FUNC_DOC = re.compile(r"/\*\*((?:(?!\*/).)*)\*/\s*(?!typedef\b)((?:static\s+)?[A-Za-z_][\w\s\*]*?\b(\w+)\s*\()", re.S)
FUNC_DEF = re.compile(r"^[A-Za-z_][\w \*]*?\b(eclp_\w+)\s*\([^;{]*\)\s*\n\{", re.M)
MODULE_TAG = re.compile(r"@module\s+(M-[A-Z]+(?:-\d+)?)")
SWR_ID = re.compile(r"SWR-\d+")


def design_modules(root):
    """상세 설계서 2장 표에서 모듈 ID 를 읽는다."""
    text = read_text(os.path.join(root, "docs/design/detailed-design.md"))
    return set(re.findall(r"^\|\s*(M-[A-Z]+-\d+)\s*\|", text, re.M))


def scan_src(root):
    """src/ 에서 함수별 요구사항 태그와 파일별 모듈 태그를 읽는다."""
    funcs, modules, defs = {}, {}, {}
    src_dir = os.path.join(root, "src")
    for name in sorted(os.listdir(src_dir)) if os.path.isdir(src_dir) else []:
        if not name.endswith((".c", ".h")):
            continue
        text = read_text(os.path.join(src_dir, name))
        file_block = re.match(r"\s*/\*\*(.*?)\*/", text, re.S)
        modules[name] = set(MODULE_TAG.findall(file_block.group(1))) if file_block else set()
        if name.endswith(".c"):
            for func in FUNC_DEF.findall(text):
                defs[func] = name
        for block, _sig, func in FUNC_DOC.findall(text):
            funcs.setdefault(func, []).append((name, set(SWR_ID.findall(block))))
    return funcs, modules, defs


def derive_modules(root):
    """{요구사항: {모듈}} 과 함수 태그 오류를 도출한다."""
    funcs, file_modules, defs = scan_src(root)
    problems, result = [], {}
    for func, entries in funcs.items():
        for file_name, reqs in entries:
            if not reqs:
                problems.append(f"src/{file_name}: 함수 {func} 에 @requirement 태그가 없음")
            owner = file_name if file_name.endswith(".c") else defs.get(func)
            owners = file_modules.get(owner, set()) if owner else set()
            if len(owners) != 1:
                problems.append(f"src/{file_name}: 함수 {func} 의 모듈을 하나로 정할 수 없음 ({sorted(owners)})")
            for req in reqs:
                result.setdefault(req, set()).update(owners)
    return result, problems, file_modules


def derive_tests(root):
    """{요구사항: {수준: {시험 ID}}} 를 tests/ 의 태그에서 도출한다."""
    result = {}
    for level, column in (("unit", "unit_tests"), ("integration", "integration_tests"), ("system", "system_tests")):
        level_dir = os.path.join(root, "tests", level)
        for base, _dirs, files in os.walk(level_dir):
            for name in files:
                if not name.endswith(TEST_FILES):
                    continue
                for block in re.findall(r"/\*\*.*?\*/", read_text(os.path.join(base, name)), re.S):
                    test = TEST_TAG.search(block)
                    if test:
                        for match in REQ_TAG.finditer(block):
                            for req in split(match.group(1)):
                                result.setdefault(req, {}).setdefault(column, set()).add(test.group(1))
    return result


def check_derived(root, rows, errors):
    """BUILT 상태 요구사항의 모듈·시험 열이 소스와 테스트의 태그에서 도출한 값과 같은지 검사한다."""
    known = design_modules(root)
    by_req, problems, file_modules = derive_modules(root)
    errors.extend(problems)
    tests = derive_tests(root)
    ids = {row["req_id"] for row in rows}
    for name, mods in sorted(file_modules.items()):
        if not mods:
            errors.append(f"src/{name}: 파일 머리말에 @module 태그가 없음")
        for mod in sorted(mods - known):
            errors.append(f"src/{name}: 모듈 {mod} 가 상세 설계서 표에 정의되지 않음")
    for req in sorted(set(by_req) - ids):
        errors.append(f"{req}: 소스 태그에는 있으나 trace.csv 에 없음")
    code_modules = {mod for name, mods in file_modules.items() if name.endswith(".c") for mod in mods}
    for row in rows:
        if row["status"] not in BUILT:
            continue
        req = row["req_id"]
        expected_modules = by_req.get(req, set())
        if not expected_modules and row["category"].startswith("제약사항"):
            expected_modules = code_modules
        if set(split(row["modules"])) != expected_modules:
            errors.append(f"{req}: modules {split(row['modules'])} 와 소스 태그에서 도출한 {sorted(expected_modules)} 불일치")
        for _label, column in LEVELS:
            derived = tests.get(req, {}).get(column, set())
            if set(split(row[column])) != derived:
                errors.append(f"{req}: {column} {split(row[column])} 와 테스트 태그에서 도출한 {sorted(derived)} 불일치")


def check(root):
    errors = []
    rows = read_csv(os.path.join(root, "docs/traceability/trace.csv"))
    oem_rows = read_csv(os.path.join(root, "docs/traceability/oem_trace.csv"))
    assumptions = read_text(os.path.join(root, "docs/requirements/assumptions.md"))
    safety_text = read_text(os.path.join(root, "docs/requirements/safety-requirements.md"))
    srs_text = read_text(os.path.join(root, "docs/requirements/software-requirements.md"))
    safety = doc_rows(safety_text)
    srs = doc_rows(srs_text)
    components = set(re.findall(r"^\|\s*(C-[A-Z]+)\s*\|", read_text(os.path.join(root, "docs/traceability/README.md")), re.M))
    tests = scan_tests(root)

    check_sources(rows, oem_rows, assumptions, errors)
    check_docs(rows, safety, srs, errors)
    check_headers(safety_text, srs_text, safety, errors)
    check_codes(srs_text, safety, srs, assumption_rows(assumptions), {row["req_id"] for row in rows}, errors)
    check_links(rows, {**safety, **srs}, components, tests, errors)
    check_derived(root, rows, errors)

    # 요구사항 커버리지: 시스템 시험 대상(기능, 품질 요구사항) 중 시스템 시험에 연결된 비율
    verified = [row for row in rows if row["status"] == "verified"
                and (row["type"].startswith("기능") or row["category"].startswith("품질"))]
    covered = [row for row in verified if split(row["system_tests"])]
    return errors, len(verified), len(covered), len(rows)


def main(argv):
    root = argv[1] if len(argv) > 1 else "."
    errors, verified, covered, total = check(root)
    for message in errors:
        print(message)
    print(f"[traceability] 요구사항 {total}건, verified {verified}건, 시스템 시험 연결 {covered}/{verified}, 지적 {len(errors)}건")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
