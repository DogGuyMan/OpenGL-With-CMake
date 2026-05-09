#version 330 core

in vec3 vsNormal;
in vec3 vsPosition;
in vec4 vsColor;
in vec2 vsTexCoord;

// Rasterization 과정을 거쳐 픽셀별로 할당된 vertex shader의 출력값이 입력됨
out vec4 fragColor;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

uniform vec4 baseColor;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform float specularStrength;
uniform float specularShininess;
uniform vec3 viewPos;

void main() {
    // 0. 텍스쳐 젹용
    vec4 tex0Color = texture(tex0, vsTexCoord);
    vec4 tex1Color = texture(tex1, vsTexCoord);

    float w = smoothstep(0.2, 0.8, vsTexCoord.x);
    fragColor = baseColor * vsColor;
    fragColor = mix(tex0Color, tex1Color, w) * fragColor;

    // 1. 오브젝트 컬러링
    vec3 ambient = material.ambient * light.ambient;

    // 2. Diffuse 라이팅
    vec3 lightDir = normalize(light.position - vsPosition);
    vec3 pixelNorm = normalize(vsNormal);   // vertex shader에서 계산된 normal은
                                            // rasterization 되는 과정에서 선형 보간이 진행되서
                                            // FS에 와서도 normalize를 해줘야 함.
    float diff = max(dot(pixelNorm, lightDir), 0.0);
    vec3 diffuse = diff * light.ambient;

    // 3. Specular 라이팅
    vec3 viewDir = normalize(viewPos - vsPosition);
    vec3 reflectDir = reflect(-lightDir, pixelNorm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * material.specular * light.specular;

    // 스페큘러 곱하는게 아니라 더하기임.
    // fragColor = (ambient + vec4(diffuse, 1.0) + vec4(specular, 1.0)) * fragColor;
    fragColor = vec4((ambient + diffuse + specular), 1.0f) * fragColor;
}
