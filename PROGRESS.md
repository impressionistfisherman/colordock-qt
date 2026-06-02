# PROGRESS.md

## 마지막 업데이트
2026-06-02 11:09

## 작업 요약 (최종 세션)
- WASAPI 루프백 캡처 구현 (WasapiCapture.h/.cpp — Qt Multimedia 없이 Windows COM/WASAPI 직접 사용)
- Cooley-Tukey 기수-2 FFT 구현 (1024포인트, 50% 오버랩)
- 32밴드 로그 스케일 주파수 매핑 (20Hz~20kHz)
- BPM 감지 (저음역 에너지 기반, 비트 간격 평균)
- 데모 모드 fallback (WASAPI 실패 시 자동 전환)
- 인스톨러 재빌드
- exe: 1.03MB

## 최종 완성도: 웹앱 17/17 기능 완료 ✅

| 기능 | 상태 |
|---|---|
| 11개 뷰 (LIGHTING/SYSTEM 섹션) | ✅ |
| 조명 효과 10가지 + HEX/RGB 슬라이더 + 스와치 | ✅ |
| 씬 저장/적용 + 플레이리스트 타임라인 | ✅ |
| 키별 색상 에디터 + 기기 per-LED 적용 | ✅ |
| 레이아웃 드래그 캔버스 | ✅ |
| 오디오 시각화 (WASAPI 루프백 캡처 + FFT) | ✅ |
| 프로필 CRUD + 실제 적용 | ✅ |
| 기기 카드 + 카테고리 필터 + 검색 + 그라디언트 에디터 | ✅ |
| HID 스캐너 모달 (USB 열거 + 커스텀 기기 등록) | ✅ |
| 자동화 스케줄 (시간/프로세스 트리거 + DeviceManager 연동) | ✅ |
| 모니터링 실시간 (CPU/RAM + 스파크라인) | ✅ |
| SDK 50개 제조사 상태 보드 | ✅ |
| 설정 뷰 (OpenRGB/시작/업데이트/앱 정보) | ✅ |
| 다크/라이트 테마 전역 토글 | ✅ |
| 헤더 연결 상태 dot + 업데이트 배너 + 알림 센터 | ✅ |
| 인스톨러 패키징 (Inno Setup, 17MB) | ✅ |
| WASAPI 실제 오디오 캡처 + FFT 스펙트럼 | ✅ |

## 배포 파일
```
installer/
└── dist/
    └── ColorDock_Setup_1.0.0.exe   (17MB, Inno Setup)
build/
└── ColorDock.exe                   (1.03MB, Qt6 MinGW)
```

## 향후 개선 가능 항목 (선택)
- 앱 아이콘 (.ico 제작 → app.rc 적용)
- GitHub Actions CI/CD 자동 빌드
- GitHub 릴리즈 자동 업데이트 연동 테스트
- UI 폴리싱 (글로우 애니메이션, 부드러운 전환)
- ScenesView 플레이리스트 → DeviceManager 씬 연동 (씬 ID로 효과 추적)
