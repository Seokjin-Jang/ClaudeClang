# PICT 사용 예

[PICT](https://github.com/microsoft/pict)는 파라미터 조합을 pairwise로 줄여 주는 도구다.

## 모델 파일 (`model.txt`)

```
Mode:    Init, Normal, Degraded
Speed:   0, 1, 119, 120, 121
Brake:   On, Off
```

제약 조건 예:

```
IF [Mode] = "Init" THEN [Speed] = 0;
```

## 실행

```bash
pict model.txt > cases.tsv
```

생성된 각 행을 테스트 케이스 하나로 만들고, 예상 출력값은 **설계/요구사항에서** 정한다. 도구가 만든 조합은 입력 후보일 뿐 기대값을 대신 정해주지 않는다. 경계값(위 예의 119/120/121)은 PICT 값에 직접 포함시킨다.
