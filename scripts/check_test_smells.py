#!/usr/bin/env python3
"""
check_test_smells.py — 테스트 자체의 결함성 정적 검사.

규칙 (모두 warn 등급, 빌드/ctest 차단 X):
  R1: TEST_CASE 안에 단언 0개 (REQUIRE/CHECK/SUCCEED 모두 0)
  R2: SUCCEED-only 스모크 (body의 유일한 단언이 SUCCEED)
  R3: tag 누락 (TEST_CASE 두 번째 인자 빈 문자열 또는 누락)
  R4: disabled/skipped tag ([.] / [!hide] / [!shouldfail])

사용:
  python3 scripts/check_test_smells.py test/

근거: doc/testplan/2026-05-07-gl-state-and-test-quality-design.md §6.1
한계: 단일 라인 TEST_CASE 만 처리 (다중 라인은 Phase 2). 정규식 기반 — AST 정확도 X.
"""

import re
import sys
from pathlib import Path

# TEST_CASE("name", "[tag]") 또는 TEST_CASE("name") — 단일 라인 헤더 가정
TEST_CASE_RE = re.compile(
    r'TEST_CASE\s*\(\s*"(?P<name>[^"]*)"\s*(?:,\s*"(?P<tags>[^"]*)")?\s*\)\s*\{'
)
ASSERT_RE = re.compile(
    r'\b(REQUIRE|CHECK|REQUIRE_FALSE|CHECK_FALSE|REQUIRE_THROWS(?:_AS|_WITH|_MATCHES)?|'
    r'CHECK_THROWS(?:_AS|_WITH|_MATCHES)?|SUCCEED|REQUIRE_THAT|CHECK_THAT|'
    r'STATIC_REQUIRE(?:_FALSE)?|STATIC_CHECK(?:_FALSE)?)\s*\('
)
SUCCEED_RE = re.compile(r'\bSUCCEED\s*\(')

DISABLED_TAGS = ('[.]', '[!hide]', '[!shouldfail]')


def find_test_bodies(text):
    """단순 brace-counting으로 TEST_CASE body 추출. 단일 라인 헤더만."""
    out = []
    for m in TEST_CASE_RE.finditer(text):
        name = m.group('name')
        tags = m.group('tags') or ''
        # body 시작 = '{' 위치
        i = m.end() - 1  # '{'
        depth = 1
        j = i + 1
        while j < len(text) and depth > 0:
            c = text[j]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
            j += 1
        body = text[i + 1:j - 1] if depth == 0 else text[i + 1:]
        line_no = text[:m.start()].count('\n') + 1
        out.append((line_no, name, tags, body))
    return out


def check_file(path):
    findings = []
    text = path.read_text(encoding='utf-8')
    for line_no, name, tags, body in find_test_bodies(text):
        asserts = ASSERT_RE.findall(body)
        succeeds = SUCCEED_RE.findall(body)

        # R1: 단언 0개
        if not asserts:
            findings.append((path, line_no, 'R1',
                             f'TEST_CASE has no assertions: "{name}"'))

        # R2: SUCCEED-only (asserts에 SUCCEED만 있음)
        elif succeeds and len(asserts) == len(succeeds):
            findings.append((path, line_no, 'R2',
                             f'SUCCEED-only test smell: "{name}" — '
                             f'consider SpdlogCapture or behavioral assertion'))

        # R3: tag 누락
        if not tags.strip():
            findings.append((path, line_no, 'R3',
                             f'TEST_CASE missing tag: "{name}" — '
                             f'cannot filter via ctest -R'))

        # R4: disabled
        for d in DISABLED_TAGS:
            if d in tags:
                findings.append((path, line_no, 'R4',
                                 f'TEST_CASE has disabled tag {d}: "{name}"'))
                break

    return findings


def main(argv):
    if len(argv) < 2:
        print('usage: check_test_smells.py <test_dir>', file=sys.stderr)
        return 2

    test_dir = Path(argv[1])
    if not test_dir.exists():
        print(f'error: {test_dir} not found', file=sys.stderr)
        return 2

    cpp_files = sorted(test_dir.rglob('test_*.cpp'))
    all_findings = []
    for p in cpp_files:
        all_findings.extend(check_file(p))

    if not all_findings:
        print(f'check_test_smells: 0 warnings across {len(cpp_files)} files')
        return 0

    for path, line, rule, msg in all_findings:
        print(f'{path}:{line}: warning [{rule}]: {msg}')

    print(f'\ncheck_test_smells: {len(all_findings)} warnings '
          f'across {len(cpp_files)} files (all warn-level, build not blocked)')
    return 0  # warn-only — 0 exit (R1-R4 모두 warn 정책)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
