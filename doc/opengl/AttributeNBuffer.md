# VAO / VBO / EBO 는 언제 *분할* 해야 하는가

> "같은 형상을 100개·1000개 그릴 때는 VAO 1개로 충분하다" — 맞다. 분할 사유는 *개수* 가 아니라 **데이터 구조나 GPU 사용 패턴이 다를 때** 만 정당화된다.

이 노트는 분할이 *반드시* 필요한 5가지 시나리오를 정리한다.

---

## 분할 ≠ 인스턴스 개수

먼저 헷갈리지 말 것 — 같은 mesh 를 여러 개 그리는 것은 분할 사유가 **아니다**.

| 상황 | 분할 필요? | 해결 |
|---|---|---|
| Cube 1000개를 다른 위치에 그림 | ✗ | uniform `modelMat` 만 바꾸며 draw 1000번, 또는 instanced rendering |
| Cube 와 Sphere 를 함께 그림 | ✓ | 형상이 다르므로 VBO/EBO 분리 |

---

## 시나리오 1 — 형상(mesh) 자체가 다를 때 (가장 기본)

> Cube / Sphere / Quad / Terrain — 정점 좌표·인덱스 패턴이 서로 호환 안 됨.

| 자원 | 분리 | 이유 |
|---|---|---|
| VBO | ✓ | Cube 24~36정점, Sphere 수백 정점, Terrain 수만 정점 — 한 버퍼에 묶을 수 없음 |
| EBO | ✓ | 인덱스 시퀀스 자체가 다름 |
| VAO | ✓ | 각각 binding 상태를 별도로 캐싱 |

학습 단계에서 가장 흔히 만나는 분할 사유. [context.cpp](../../src/context/context.cpp) 도 현재 cube 1종이라 VAO/VBO/EBO 1세트.

---

## 시나리오 2 — Vertex attribute layout 이 다를 때 (VAO 분리의 핵심)

> 같은 cube 여도 셰이더 입력 인터페이스가 다르면 **VBO 자체는 공유 가능하나 VAO 는 따로 만들어야** 한다.

예시:
- 메시 A: `(pos, normal, uv)` = 32B stride, attrib `0/1/2`
- 메시 B: `(pos, normal, color, uv, tangent, bitangent)` = 56B stride, attrib `0/1/2/3/4/5`
- PBR mesh vs unlit debug mesh — 후자는 normal·tangent 가 없음

VAO 는 본질적으로 **"어느 VBO 의 어느 byte offset 부터, 어떤 포맷으로, 몇 stride 로 attrib N 에 매핑할지"** 의 *사전 베이크 상태*. layout 이 달라지면 VAO 한 개로 돌려쓸 수 없다.

> 우리 코드 [context.cpp `TrySetAttrib(0..3, ...)`](../../src/context/context.cpp) 호출이 바로 이 layout 정의. 다른 레이아웃을 쓰는 mesh 가 추가되면 VAO 추가 필수.

---

## 시나리오 3 — 업데이트 빈도가 다를 때 (VBO 분리의 핵심)

> `glBufferData` 의 hint(`GL_STATIC_DRAW` / `GL_DYNAMIC_DRAW` / `GL_STREAM_DRAW`) 가 드라이버의 GPU 메모리 배치(VRAM vs system RAM, write-combined 등)를 결정. 빈도가 다른 데이터를 한 VBO 에 섞으면 드라이버 휴리스틱이 어긋난다.

예시:

| 빈도 | hint | 사용처 |
|---|---|---|
| 한 번 업로드 후 평생 안 바뀜 | `GL_STATIC_DRAW` | terrain mesh, 환경 모델 |
| 매 프레임 일부 갱신 | `GL_DYNAMIC_DRAW` | skinned character mesh (CPU skinning 결과) |
| 매 프레임 처음부터 다시 작성 | `GL_STREAM_DRAW` | GUI vertex stream, debug line drawer, particle emitter |

같은 mesh class 라도 인스턴스마다 빈도가 다르면 분리. dynamic mesh 를 정적 terrain VBO 한가운데에 넣으면 GPU 캐시가 자주 무효화된다.

---

## 시나리오 4 — Instanced rendering 의 per-instance attribute (VBO 추가의 대표 사유)

> "Square 1000개는 VAO/VBO/EBO 1개씩으로 충분" 이라는 직관은 **단순 draw 루프** 에는 맞다. 하지만 **고성능 instancing** 으로 그리려면 per-instance VBO 한 개가 *추가로* 필요하다.

```
VAO 1개
 ├── VBO #1 (per-vertex):  pos, normal, uv         — divisor 0 (vertex 마다 갱신)
 ├── VBO #2 (per-instance): modelMat, instanceColor — divisor 1 (instance 마다 갱신)
 └── EBO   (인덱스)
```

- `glVertexAttribDivisor(attribIdx, 1)` 로 attribute 가 vertex-rate 가 아닌 *instance-rate* 로 동작
- mesh 데이터(고정) 와 인스턴스별 데이터(가변) 의 update 빈도·접근 패턴이 다르므로 VBO 분리가 자연스럽다
- `glDrawElementsInstanced(... , 1000)` 한 번으로 1000개 큐브 → CPU 호출 1회, GPU 가 N번 펼침

| 자원 | 개수 |
|---|---|
| VAO | 1 |
| VBO | **2** (per-vertex + per-instance) |
| EBO | 1 |

---

## 시나리오 5 — 멀티패스 / 셰이더 인터페이스 분기 (VAO 만 분리)

> Deferred shading / shadow mapping / Z-prepass — 같은 메시 데이터를 다른 attribute 부분집합으로 입력해야 할 때.

예시:

| 패스 | 활성화 attribute | 이유 |
|---|---|---|
| Shadow pass (depth only) | position 만 (attrib 0) | 깊이만 기록 |
| G-buffer pass | position + normal + uv + tangent | material 정보 풀세트 |
| Outline pass | position + normal | 윤곽선 추출 |

VBO/EBO 는 **공유**, VAO 는 **패스마다 따로**. 각 VAO 가 그 패스의 attribute enable/disable 조합과 binding 상태를 사전에 베이크해두면, 매 프레임 `glEnableVertexAttribArray` / `glDisableVertexAttribArray` 토글 비용이 사라진다.

---

## 한 장 요약

| 시나리오 | VAO | VBO | EBO |
|---|---|---|---|
| 1. 형상 다름 | 별개 | 별개 | 별개 |
| 2. Attribute layout 다름 | **별개** | 공유 가능 | 공유 가능 |
| 3. 업데이트 빈도 다름 | 별개 권장 | **별개** | 데이터에 따라 |
| 4. Instanced per-instance attr | 1개 | **2개**(per-vert + per-inst) | 1개 |
| 5. 멀티패스 attribute 분기 | **패스마다 별개** | 공유 | 공유 |

### 일반 원칙
- 형상·인덱스 패턴이 다르다 → **VBO/EBO** 분리
- attribute layout 이나 활성화 조합이 다르다 → **VAO** 분리
- update 빈도·hint 가 다르다 → **VBO** 분리
- 그 외(같은 mesh를 위치만 바꿔 1000개) → 1개씩으로 충분, uniform 만 바꾸거나 instancing

---

## 관련 개념

- [PhongShading.md](PhongShading.md) — fragment shader 측의 라이팅 수식 (이 노트는 vertex 입력 측)
- [context.cpp `Init()` 메서드](../../src/context/context.cpp) — 단일 cube VAO/VBO/EBO 생성 예시 (시나리오 1 의 1세트 버전)
- [context.h `mVertexArrayObject` / `mVertexBufferObject` / `mElementBufferObject`](../../src/context/context.h) — UPtr 로 자원 소유. 시나리오 1·2 로 확장 시 vector 또는 별도 멤버로 늘려야 함.
