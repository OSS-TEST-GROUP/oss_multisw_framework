# 루프 B 시나리오 자산 (`.xosc`)

`docker-compose-sils-ss2.yaml` 의 `ss2-scenario` 가 이 디렉터리를 `/scenarios` 로 읽기 전용
마운트한다. **파일 자체는 커밋하지 않는다** (`.gitignore` 에서 이 README 만 추적).

## 어디서 가져오나

| 출처 | 계정 | 언제 |
|---|---|---|
| **상류 동봉 샘플** — `tier4/scenario_simulator_v2` 의 `test_runner/scenario_test_runner/scenario/` | **불필요** | **먼저 이걸로 시작할 것** |
| Web.Auto Scenario Editor 저작물 | Scenario Editor 접근에만 필요 | 실제 요구 시나리오를 태울 때 |

**Web.Auto 계정은 브링업에 필요 없다.** 계정이 필요한 것은 저작뿐이고, 실행·판정은 계정 없이 된다
([BR-S](../../docs/sils/sils-real-bridge-requirements.md)).

> TIER IV Scenario Format v2.0(YAML)은 `.xosc` 가 아니다 — `scenario_test_runner` 가 내부에서
> 변환한다. 변환 경로 확인은 작업 항목 **B1-4**.

## 쓰는 법

```bash
# 실행
SILS_SS2_SCENARIO=sample.xosc \
  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-scenario

# 같은 파일에서 판정 기준 추출 (Linux 없이도 된다)
python3 tools/sils/conformance/import_openscenario.py \
    --xosc scenarios/ss2/sample.xosc --out /tmp/requirements.yaml
```

같은 `.xosc` 가 자극과 판정 양쪽으로 흐르는 것이 루프 B 의 핵심 성질이다 —
[simulator-loops §4](../../docs/sils/sils-simulator-loops.md).
