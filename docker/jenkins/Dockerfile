# Jenkins 안정 버전(LTS). 플러그인과 설정(JCasC)을 이미지에 미리 포함한다.
FROM jenkins/jenkins:lts-jdk21

USER root
# 파이프라인이 호스트 Docker 데몬(docker.sock)으로 claudeclangbuild 컨테이너를 실행하므로 Docker CLI 만 설치한다.
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl \
    && install -m 0755 -d /etc/apt/keyrings \
    && curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc \
    && . /etc/os-release \
    && echo "deb [signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian ${VERSION_CODENAME} stable" \
        > /etc/apt/sources.list.d/docker.list \
    && apt-get update && apt-get install -y --no-install-recommends docker-ce-cli docker-buildx-plugin \
    && rm -rf /var/lib/apt/lists/*

USER jenkins
COPY plugins.txt /usr/share/jenkins/ref/plugins.txt
RUN jenkins-plugin-cli --plugin-file /usr/share/jenkins/ref/plugins.txt

COPY casc.yaml /usr/share/jenkins/ref/casc.yaml
# CSP 해제: llvm-cov HTML 리포트가 Jenkins 안에서 스타일 포함해서 보이도록 한다. (로컬 전용 환경)
ENV CASC_JENKINS_CONFIG=/usr/share/jenkins/ref/casc.yaml \
    JAVA_OPTS="-Djenkins.install.runSetupWizard=false -Dhudson.model.DirectoryBrowserSupport.CSP="
