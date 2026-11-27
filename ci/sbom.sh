#!/usr/bin/env bash
# SBOM(CycloneDX JSON)을 생성한다. OEM 인도 증거(G3)용.
set -u
RESULT=0
REPORT_DIR="${REPORT_DIR:-reports}"
mkdir -p "$REPORT_DIR"

export SYFT_CHECK_FOR_APP_UPDATE=false
if ! syft scan dir:. \
        --exclude './build/**' --exclude './reports/**' --exclude './.git/**' \
        -o "cyclonedx-json=$REPORT_DIR/sbom.cdx.json" -q; then
    echo "[sbom] 생성 실패" >&2
    RESULT=1
elif [ ! -s "$REPORT_DIR/sbom.cdx.json" ]; then
    echo "[sbom] 결과 파일이 비어 있다." >&2
    RESULT=1
else
    echo "[sbom] $REPORT_DIR/sbom.cdx.json 생성"
fi

exit "$RESULT"
