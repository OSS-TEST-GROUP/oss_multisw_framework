# vehicle_info_mgr TLS 인증서

이 디렉토리의 `key.pem` / `cert.pem` 은 **git 에 커밋하지 않는다.**
사설키를 저장소에 두면 공개 저장소로 전달되는 순간 노출되기 때문이다.

## 개발·테스트용 인증서 생성

저장소 루트에서:

```bash
./scripts/gen-dev-cert.sh
```

- 생성 위치: `service/core/vehicle_info_mgr/config/security/{key,cert}.pem`
- 기본값: `CN=localhost`, 365일, `subjectAltName=DNS:localhost,IP:127.0.0.1`
- 옵션: `CN=myhost DAYS=730 ./scripts/gen-dev-cert.sh`, 덮어쓰기는 `FORCE=1`

경로는 `config/forest/server.json` 의 `security/cert.pem`·`security/key.pem`
기본값과 일치하므로 별도 설정 변경이 필요 없다.

## 운영 환경

이 스크립트로 만든 self-signed 인증서를 운영에 쓰지 않는다.
정식 CA 가 발급한 인증서를 같은 경로에 배치하거나 `server.json` 에서 경로를 지정한다.
