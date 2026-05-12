#version 330 core

in vec3 vsNormal;
in vec3 vsPosition;
in vec4 vsColor;
in vec2 vsTexCoord;

// Rasterization 과정을 거쳐 픽셀별로 할당된 vertex shader의 출력값이 입력됨
out vec4 fragColor;

struct Light {
    vec3 position;
    vec3 attenuation;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
uniform Material material;

uniform vec4 baseColor;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform vec3 viewPos;

void main() {
    // 0. 텍스쳐 젹용
    vec4 tex0Color = texture(tex0, vsTexCoord);
    vec4 tex1Color = texture(tex1, vsTexCoord);

    float w = smoothstep(0.2, 0.8, vsTexCoord.x);
    fragColor = baseColor * vsColor;
    fragColor = mix(tex0Color, tex1Color, w) * fragColor;

    // 1. 오브젝트 디퓨즈 텍스쳐링
    vec3 diffuseTexColor = texture(material.diffuse, vsTexCoord).rgb;
    vec3 ambient = diffuseTexColor * light.ambient;

    // 2. Diffuse 라이팅
    float dist = length(light.position - vsPosition);
    vec3 distPoly = vec3(1.0, dist, dist * dist); // 1.0, d, d*d
    //  distPoly.x * light.attenuation.x + => 1.0 * Kc +
    //  distPoly.y * light.attenuation.y + => d * K1 +
    //  distPoly.z * light.attenuation.z + => d*d*K2 +
    float attenuation = 1.0 / dot(distPoly, light.attenuation);
    vec3 lightDir = (light.position - vsPosition) / dist;
    vec3 pixelNorm = normalize(vsNormal); // vertex shader에서 계산된 normal은
    // rasterization 되는 과정에서 선형 보간이 진행되서
    // FS에 와서도 normalize를 해줘야 함.
    float diff = max(dot(pixelNorm, lightDir), 0.0);
    // light.diffuse 채널을 써야 함. 직전 버전은 light.ambient 와 곱해
    // diffuse 슬라이더가 무력화되고 ambient 슬라이더가 두 항을 동시에 흔들었음.
    vec3 diffuse = diff * diffuseTexColor * light.diffuse;

    // 3. Specular 라이팅
    // GLSL core profile 에서는 `texture` 사용 (texture2D 는 deprecated).
    // in 변수명은 vsTexCoord — texCoord 는 정의되지 않은 식별자.
    vec3 specTexColor = texture(material.specular, vsTexCoord).rgb;
    vec3 viewDir = normalize(viewPos - vsPosition);
    // 3-1 Phong:
    vec3 reflectDir = reflect(-lightDir, pixelNorm);
    //  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // 3-2 Blinn-Phong:
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(pixelNorm, halfwayDir), 0.0), material.shininess);
    vec3 specular = spec * specTexColor * light.specular;

    // specular 는 albedo 와 *곱하지 않고* 더한다.
    // (ambient + diffuse) 만 surface albedo(fragColor) 로 modulate, specular 는 빛 자체 색으로 가산.
    // 이전 형태인 (ambient+diffuse+specular)*fragColor 는 specular 가 baseColor(주황) 에 tint 되어 묻혔음.

    fragColor = vec4((ambient + diffuse + specular), 1.0f) * attenuation * fragColor;

    // DEBUG

    // fragColor = vec4(pixelNorm * 0.5 + 0.5, 1.0); // Test 1 — Normal이 회전을 따라가는가?
    // fragColor = vec4(normalize(light.position - vsPosition) * 0.5 + 0.5, 1.0); // Test 2 — LightDir이 회전과 무관한가?
    // fragColor = vec4(vec3(diff), 1.0); // Test 3 — Diffuse(N·L)는 광원 방향과 정렬되는가?
}
