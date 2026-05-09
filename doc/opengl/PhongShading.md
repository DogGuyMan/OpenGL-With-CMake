# Phong Shading — 3 가지 헷갈리는 지점 정리

> [simple.vs](../../resources/shader/simple.vs) / [simple.fs](../../resources/shader/simple.fs) 작성 중 마주친 세 가지 의문에 대한 노트.
> 1학년 수준 직관 + 정확한 수식 페어링.

---

## Q1. `transpose(inverse(modelTransformMat)) * vec4(aNormal, 0.0)` — 왜 normal 에만 이런 변환?

### 한 줄 요약
**normal 은 정점이 아니라 "표면에 수직인 방향" 이라서, 모델이 변형되면 normal 도 *반대로* 변형해야 직각이 유지된다.**

### 직관 — 풍선을 위에서 누르는 그림
풍선을 평평한 발판 위에서 손바닥으로 누른다고 상상.
- 풍선 자체는 **위아래로 짜부라지고 옆으로 늘어남** (Y축 0.5배, X축 2배 = non-uniform scale)
- 풍선 표면에 박아둔 **압정**(=normal) 은 어떻게 될까?

세로로 짜부 시켰으면, 표면 기울기는 **더 옆으로 누워서** 압정도 *옆으로 누워야* 직각이 유지될 것 같지만 — **반대다**. Y가 작아진 만큼 압정의 Y 성분은 *더 커져야* (더 위를 향해야) 새 표면에 직각.

→ 정점에 적용한 변환을 normal 에는 *반대로* 적용해야 한다 = `inverse`.
→ vector 와 covector 의 차이로 한 번 더 transpose 가 붙어 `transpose(inverse(M))` 이 됨.

### 수식 유도 (1줄)
표면 위 임의의 접선 벡터를 t, normal 을 n 이라 하면 정의상 `n · t = 0`.
변환 후에도 `n' · t' = 0` 이 유지되어야 하니, `t' = M·t` 일 때
```
n' = (M⁻¹)ᵀ · n
```
이 자동 도출.

### 언제 생략 가능한가
- M 이 **회전 + 균등 스케일 + 평행이동** 만이면 `(M⁻¹)ᵀ == M` (방향 성분 한정). 그래서 균등 스케일만 쓰는 엔진은 `mat3(M) * aNormal` 로 끝냄.
- 자유 변형(non-uniform scale, shear)을 허용하면 normal matrix 필수.

### `vec4(aNormal, 0.0)` 의 0
방향 벡터(w=0) 라서 평행이동 칸이 무시됨. normal 은 위치가 아니라 *방향* 이므로 평행이동되면 안 됨. 위치 벡터(`vec4(aPos, 1.0)`) 의 w=1 과 대조.

---

## Q2. `gl_Position` 따로, `positionVector` 따로 — 왜 같은 정점을 두 번 변환?

### 한 줄 요약
**둘은 *서로 다른 좌표계* 의 위치다. 화면에 그릴 위치(clip space)와 빛 계산용 위치(world space)는 다른 공간이라 분리.**

### 두 변환의 차이

| 변수 | 곱한 행렬 | 결과 좌표계 | 용도 |
|---|---|---|---|
| `gl_Position` | `transformMat = proj × view × model` | clip / NDC | 래스터라이저가 화면 픽셀 어디에 그릴지 결정 |
| `positionVector` | `modelTransformMat = model` 만 | world space | FS 에서 빛까지 거리·방향 계산 |

### 직관 — "지도 vs 사진" 비유
- **gl_Position = 사진 속 픽셀 좌표.** 원근 때문에 멀리 있는 큐브는 작게 찍힘. 화면 어디에 칠할지 정하는 용도.
- **positionVector = 실제 세상 좌표.** "이 큐브는 (3, 0, -5) 에 있고, 빛은 (3, 3, 3) 에 있다 → 두 점 사이 벡터는 (0, 3, 8)" 같은 *물리적* 거리/방향 계산용.

> 사진 픽셀 좌표로 "두 도시의 직선 거리 km" 를 구하면 원근 왜곡 때문에 엉터리. 빛 방향도 마찬가지로 clip space 로 계산하면 깨짐.

### Phong 라이팅에서 정확히 무엇을 계산하나
[simple.fs:32](../../resources/shader/simple.fs):
```glsl
vec3 lightDir = normalize(lightPos - positionVector);
```
- `lightPos` 는 world space 로 CPU 가 넘김 (`mLightPos`)
- `positionVector` 도 world space 여야 두 벡터의 차가 의미 있는 *세상에서의 빛 방향* 이 됨

만약 `gl_Position` (clip space) 으로 같은 계산을 하면:
- clip space 는 projection 적용 후 좌표 — perspective divide 후 `[-1, 1]` 정규화
- 거리 비율이 z 깊이에 따라 *비선형* 으로 일그러짐
- 빛이 카메라 위치에 따라 다르게 보이는 이상한 결과

### View space 변종
엔진에 따라 `view × model` 까지 적용한 *view space* 좌표를 보내기도 함 (lightPos 도 같이 view space 로 변환해서). 핵심은 **light · position · viewer 가 *같은 공간*에 있어야 한다**는 것.

---

## Q3. `reflect(I, N)` 의 정확한 수식

### 흔한 오해
```
2 * pixelNorm * dot(-lightDir, pixelNorm)        ← 입사 벡터 I 항이 빠짐
```

### GLSL 정의 (정확)
```
reflect(I, N) = I - 2.0 * dot(N, I) * N
```

우리 셰이더의 호출은 `reflect(-lightDir, pixelNorm)` 이므로 I = `-lightDir`, N = `pixelNorm`:
```
reflectDir = -lightDir - 2 * dot(pixelNorm, -lightDir) * pixelNorm
           = -lightDir + 2 * dot(pixelNorm,  lightDir) * pixelNorm
```

오해된 식 = `2·N·dot(N,-L)` = 위 식의 *두 번째 항*만. **첫 번째 항 `-L` (입사 벡터 본체) 이 누락**.

### 직관 — 당구공이 쿠션에 부딪히기
입사 벡터 I 를 두 성분으로 분해:
1. **쿠션과 평행한 성분 (수평 미끄러짐)** — 충돌 후에도 그대로 유지
2. **쿠션에 수직한 성분 (벽에 박는 힘)** — 충돌 후 부호 뒤집힘 (튕김)

수식으로 쓰면:
- `I_∥ = (N·I) · N` — N 방향 성분 (벡터). `(N·I)` 는 스칼라(투영 길이), 거기에 N 곱해서 벡터로 복원
- `I_⊥ = I - I_∥` — 표면 평행 성분 (쿠션과 나란한)
- 반사 후: `R = I_⊥ - I_∥`
            = `(I - I_∥) - I_∥`
            = `I - 2·I_∥`
            = **`I - 2·(N·I)·N`** ← 정확한 식

### 그림으로
```
        N
        ↑
  I_∥   |
   ↘    |
    ↘ θ | θ
─────●─────  ← 쿠션 표면
    ↗ θ | θ
   ↗    |
  R     |
        |
```
- `θ` 가 N 과 I 사이 각도일 때, R 도 N 과 같은 각도를 이루며 반대편으로
- 평행 성분(좌→우)은 그대로, 수직 성분(↓→↑)만 반전

### 오해된 식이 *부분적으로* 맞는 이유
`2·N·dot(-L, N)` 은 **"수직 성분 2개분"** 을 정확히 가리킨다 (`I_∥` 의 부호와 절댓값이 맞음). 거기서 *입사 벡터 I 자체* 만 더해주면 완성:
```
reflectDir = I + 2·N·dot(-L, N)              ← 누락분 보충
           = -lightDir + 2·pixelNorm·dot(-lightDir, pixelNorm)
           ≡ -lightDir - 2·pixelNorm·dot(pixelNorm, lightDir)·(-1)
           = … (부호 정리하면 동일)
```

### Phong specular 에서의 의미
`reflect(-lightDir, pixelNorm)` = "빛이 표면에 부딪힌 뒤 튕겨 나가는 방향". Phong 모델은 이 방향과 카메라(`viewDir`) 가 일치할수록 specular 가 세짐:
```glsl
float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularShininess);
```
- `dot(viewDir, reflectDir)` ∈ `[0, 1]` : 두 방향 정렬도
- `pow(..., shininess)` : "정렬이 약간만 어긋나도 빠르게 0 으로 떨어지는 정도" — shininess 클수록 하이라이트 좁고 뾰족.

---

## 한 장 요약

| 질문 | 핵심 한 줄 | 정확한 수식 |
|---|---|---|
| Q1 normal 변환 | 정점은 M, normal 은 M 의 inverse-transpose. 위치가 아닌 *방향* + 직각 보존 보정 | `n' = (M⁻¹)ᵀ · n`,  `w=0` |
| Q2 position 두 번 | clip space 는 그릴 픽셀 결정, world space 는 빛/거리 계산. 광원·정점·뷰어가 *같은 공간* 이어야 함 | `gl_Position = P·V·M·p`,  `posVec = M·p` |
| Q3 reflect 식 | 입사 벡터 I 의 N-수직 성분만 부호 반전. 평행 성분은 유지. 흔한 오해는 *I 본체* 누락 | `R = I − 2(N·I) N` |

---

## 관련 파일

- [simple.vs](../../resources/shader/simple.vs) — Q1, Q2 의 normal·position 변환 위치
- [simple.fs](../../resources/shader/simple.fs) — Q3 의 `reflect()` 호출 위치
- [context.cpp](../../src/context/context.cpp) — `lightPos` / `viewPos` 등 lighting uniform 세팅 측
