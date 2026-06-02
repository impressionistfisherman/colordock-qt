# CLAUDE.md (Project: ARGB Management Tool)
# 전역 CLAUDE.md 상속 + 아래 내용으로 보완

---

## 프로젝트 개요

- **앱 이름**: ColorDock RGB
- **경로**: `C:\Users\Hyeonil-Choi\Desktop\colordock-qt`
- **스택**: C++17 / Qt 6.8.3 (MinGW 64-bit)
- **빌드**: CMake + Ninja (`build/` 디렉터리)
- **목적**: Windows용 ARGB 장치 통합 관리 앱 (OpenRGB 연동)
- **배포**: `installer/dist/ColorDock_Setup_1.0.0.exe` (Inno Setup, 17MB)

---

## 빌드 방법

```powershell
# 빌드 (build/ 디렉터리에서)
cd build
cmake --build . --config Release

# 실행 파일
build/ColorDock.exe

# 인스톨러 빌드
& "C:\Program Files (x86)\Inno Setup 6\iscc.exe" installer\ColorDock_Setup.iss
```

Qt 경로: `C:/Qt/6.8.3/mingw_64`

---

## 주요 파일 구조

```
colordock-qt/
├── CMakeLists.txt
├── CLAUDE.md / PROGRESS.md
├── installer/
│   ├── ColorDock_Setup.iss          Inno Setup 스크립트
│   └── dist/ColorDock_Setup_1.0.0.exe
├── src/
│   ├── MainWindow.h/.cpp            메인 창 + 헤더 + 사이드바 + 트레이 + 알림센터
│   ├── views/
│   │   ├── EffectsView              조명 효과 10가지 + 색상환 + HEX/RGB 슬라이더
│   │   ├── DevicesView              기기 카드 + 카테고리 필터 + 검색 + 그라디언트 에디터
│   │   ├── KeyboardView             60키 키별 색상 에디터 + per-LED 적용
│   │   ├── ScenesView               씬 관리 + 플레이리스트 타임라인
│   │   ├── AudioView                WASAPI 루프백 캡처 + FFT 스펙트럼 32밴드
│   │   ├── MonitoringView           CPU/RAM 실시간 (Windows API + 스파크라인)
│   │   ├── ProfilesView             사용자 프로필 CRUD + DeviceManager 연동
│   │   ├── SettingsView             OpenRGB 연결 설정, 시작 동작, 업데이트
│   │   ├── LayoutView               드래그 캔버스, 크기/회전/반전/제외
│   │   ├── ScheduleView             시간/프로세스 트리거 + DeviceManager 연동
│   │   ├── SdkView                  50개 제조사 SDK 상태 칩
│   │   └── HidScannerDialog         USB HID 스캐너 + 커스텀 기기 등록
│   ├── hardware/
│   │   ├── DeviceManager            기기 단일 진실 소스, 효과 엔진 (20fps 타이머)
│   │   └── OpenRGBClient            TCP 6742 OpenRGB 프로토콜
│   └── utils/
│       ├── AppSettings              QSettings 래퍼 (싱글톤)
│       ├── UpdateChecker            GitHub API 업데이트 확인
│       └── WasapiCapture            WASAPI 루프백 캡처 + FFT (QThread 기반)
└── build/
    └── ColorDock.exe                1.03MB
```

---

## 주요 설계 규칙

- **DeviceManager**: 모든 기기 상태의 단일 진실 소스. 뷰에서 직접 하드웨어 접근 금지
- **AppSettings**: `AppSettings::instance().get/set()` 으로 설정 접근 (QSettings 기반)
- **씬/프로필/스케줄 저장 위치**: `QStandardPaths::AppLocalDataLocation` (AppData\Local\ColorDock\ColorDock)
- **효과 루프**: DeviceManager 내 50ms 타이머 (20fps), `applyEffect(name, baseColor)` 호출
- **뷰 → DM 연동 패턴**: 뷰 생성자에서 `DeviceManager*` 포인터를 받아 저장, 직접 호출

---

## 주의사항

- `qrand()` 사용 불가 (Qt6 제거됨) → `rand()` 또는 `QRandomGenerator` 사용
- `QPainterPath` 사용 시 `#include <QPainterPath>` 별도 필요 (Qt6에서 분리됨)
- Q_OBJECT 클래스를 `#ifdef` 내부에 정의하면 MOC가 처리 못함 → 헤더에서 가드 제거
- WASAPI 관련 COM 인터페이스는 사용 스레드에서 `CoInitializeEx` 필요
- `KSDATAFORMAT_SUBTYPE_IEEE_FLOAT` — MinGW에서 ksmedia.h 없으므로 직접 GUID 정의

---

## 테스트 방법

### 수동 테스트 (Manual Test) — 현재 적용

```powershell
# 앱 실행
.\build\ColorDock.exe
```

| 테스트 항목 | 확인 방법 |
|---|---|
| OpenRGB 연결 | OpenRGB를 `--server` 모드로 실행 후 앱 시작, 헤더 dot이 초록색인지 확인 |
| 씬 저장/적용 | 씬 저장 후 효과 변경, 씬 적용 → 조명 복원되는지 확인 |
| 자동화 트리거 | 현재 시각으로 스케줄 설정 후 1분 대기, 조명 변경 확인 |
| WASAPI 캡처 | 음악 재생 중 캡처 시작, 스펙트럼 바 반응 확인 |
| 그라디언트 에디터 | 기기 카드 → 그라디언트 버튼 → 프리셋 적용 후 시각 확인 |
| 테마 토글 | 사이드바 하단 🌙/☀️ 버튼 → 전체 UI 색상 변경 확인 |
| HID 스캐너 | 기기 뷰 → 미지원 기기 추가 → 스캔 → 등록 → 기기 목록 반영 확인 |

### 테스트 결과 기록
- `TEST_RESULT.md` (프로젝트 루트)에 날짜/항목/결과 기록

---

## 향후 개선 항목 (PROGRESS.md 참조)

- 앱 아이콘 (.ico 제작 → app.rc 적용)
- GitHub Actions CI/CD 자동 빌드
- ScenesView 플레이리스트 → 씬 ID로 DeviceManager 효과 추적
- UI 폴리싱 (글로우 애니메이션)
