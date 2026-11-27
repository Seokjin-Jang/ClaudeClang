// 내부 Jenkins: main/develop 을 매일 밤 주기적으로 빌드하고 결과를 플러그인(Warnings NG, Coverage 등)에서 본다.
// PR 의 필수 CI 는 GitHub Actions(.github/workflows/ci.yml)이며, 검사 내용은 같은 ci/*.sh 스크립트를 공유한다.
// 개발 빌드와 같은 이미지(claudeclangbuild) 안에서 모든 검사를 수행한다.
// 검사 하나가 실패해도 나머지 검사와 리포트 수집은 계속하고, 마지막 Verdict 단계에서 종합해 실패 처리한다.

import groovy.transform.Field

@Field List<String> failedGates = []

def runGate(String name, String cmd) {
    catchError(buildResult: 'SUCCESS', stageResult: 'FAILURE') {
        int rc = sh(script: cmd, returnStatus: true)
        if (rc != 0) {
            failedGates << name
            error("${name} 실패 (exit ${rc})")
        }
    }
}

pipeline {
    agent any
    triggers {
        // 매일 02시대(H: 분산)에 main/develop 만 주기 실행한다. 다른 브랜치는 빈 문자열이라 주기 실행이 없다.
        cron(env.BRANCH_NAME in ['main', 'develop'] ? 'H 2 * * *' : '')
    }
    options {
        timestamps()
        timeout(time: 60, unit: 'MINUTES')
        buildDiscarder(logRotator(numToKeepStr: '20'))
    }
    stages {
        stage('Build Image') {
            steps {
                sh 'docker build -t claudeclangbuild docker/build'
            }
        }
        stage('Build') {
            steps {
                sh 'ci/in-docker.sh ci/build.sh'
            }
        }
        stage('Static Analysis (Cppcheck)') {
            steps {
                script { runGate('Cppcheck', 'ci/in-docker.sh ci/static_analysis.sh') }
            }
        }
        stage('Metrics (Lizard, CLOC)') {
            steps {
                script { runGate('Lizard', 'ci/in-docker.sh ci/metrics.sh') }
            }
        }
        stage('Unit Test (MC/DC 100%)') {
            steps {
                script { runGate('Unit Test', 'ci/in-docker.sh ci/test.sh unit') }
            }
            post {
                always {
                    junit allowEmptyResults: true, testResults: 'reports/unit/results.xml'
                }
            }
        }
        stage('Integration Test (Function 100%)') {
            steps {
                script { runGate('Integration Test', 'ci/in-docker.sh ci/test.sh integration') }
            }
            post {
                always {
                    junit allowEmptyResults: true, testResults: 'reports/integration/results.xml'
                }
            }
        }
        stage('TDD Test') {
            steps {
                script { runGate('TDD Test', 'ci/in-docker.sh ci/test.sh tdd') }
            }
            post {
                always {
                    junit allowEmptyResults: true, testResults: 'reports/tdd/results.xml'
                }
            }
        }
        stage('Verdict') {
            steps {
                script {
                    if (failedGates) {
                        error("품질 기준 미충족: ${failedGates.join(', ')}")
                    }
                }
            }
        }
    }
    post {
        always {
            recordIssues(
                enabledForFailure: true,
                aggregatingResults: false,
                tools: [
                    cppCheck(id: 'cppcheck', name: 'Cppcheck', pattern: 'reports/cppcheck.xml'),
                    checkStyle(id: 'lizard', name: 'Lizard (CCN/Length)', pattern: 'reports/lizard.xml')
                ]
            )
            recordCoverage(
                id: 'unit-coverage', name: 'Unit Coverage', enabledForFailure: true,
                tools: [[parser: 'COBERTURA', pattern: 'reports/unit/cobertura.xml']],
                ignoreParsingErrors: true
            )
            recordCoverage(
                id: 'integration-coverage', name: 'Integration Coverage', enabledForFailure: true,
                tools: [[parser: 'COBERTURA', pattern: 'reports/integration/cobertura.xml']],
                ignoreParsingErrors: true
            )
            recordCoverage(
                id: 'tdd-coverage', name: 'TDD Coverage', enabledForFailure: true,
                tools: [[parser: 'COBERTURA', pattern: 'reports/tdd/cobertura.xml']],
                ignoreParsingErrors: true
            )
            script {
                ['unit': 'Unit', 'integration': 'Integration', 'tdd': 'TDD'].each { dir, label ->
                    if (fileExists("reports/${dir}/html/index.html")) {
                        publishHTML(target: [
                            reportDir: "reports/${dir}/html", reportFiles: 'index.html',
                            reportName: "${label} LLVM Coverage (MC/DC)",
                            keepAll: true, allowMissing: true, alwaysLinkToLastBuild: true
                        ])
                    }
                }
            }
            archiveArtifacts artifacts: 'reports/**', allowEmptyArchive: true, fingerprint: true
        }
    }
}
