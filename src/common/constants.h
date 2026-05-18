#ifndef __SJH_CONSTANTS_H__
#define __SJH_CONSTANTS_H__

#include <glad/glad.h> // DEPTH_FUNC 의 GL 비교연산자 enum 값

/**
 * @file constants.h
 * @brief 프로젝트 전역 리터럴 문자열 상수 — 리소스 ID / 파일 경로 / uniform 이름 / ImGui 라벨.
 *
 * @details
 *  매직 스트링을 한곳에 모아 오타로 인한 silent 실패(특히 uniform 이름 — 위치 -1 로 무시됨)를 방지한다.
 *  로그 포맷 문자열과 진단 컨텍스트 문자열은 의도적으로 제외 — 본질적으로 사용처 지역(local)이라
 *  상수화 이득이 적다.
 */
namespace SJH::Const
{
    // ─── 리소스 ID — ResourceRegistry 의 Image/Texture/Material 조회 키 ───
    inline constexpr auto STR_IMAGE_DARK_GRAY = "image_dark_gray";
    inline constexpr auto STR_IMAGE_GRAY = "image_gray";
    inline constexpr auto STR_IMAGE_MARBLE = "image_marble";
    inline constexpr auto STR_IMAGE_BOX1_DIFFUSE = "image_box1_diffuse";
    inline constexpr auto STR_IMAGE_BOX2_DIFFUSE = "image_box2_diffuse";
    inline constexpr auto STR_IMAGE_BOX2_SPECULAR = "image_box2_specular";
    inline constexpr auto STR_IMAGE_PLANE = "image_plane";

    inline constexpr auto STR_TEXTURE_DARK_GRAY = "texture_dark_gray";
    inline constexpr auto STR_TEXTURE_GRAY = "texture_gray";
    inline constexpr auto STR_TEXTURE_MARBLE = "texture_marble";
    inline constexpr auto STR_TEXTURE_BOX1_DIFFUSE = "texture_box1_diffuse";
    inline constexpr auto STR_TEXTURE_BOX2_DIFFUSE = "texture_box2_diffuse";
    inline constexpr auto STR_TEXTURE_BOX2_SPECULAR = "texture_box2_specular";
    inline constexpr auto STR_TEXTURE_WINDOW = "texture_window";

    inline constexpr auto STR_MATERIAL_PLANE = "material_plane";
    inline constexpr auto STR_MATERIAL_BOX1 = "material_box1";
    inline constexpr auto STR_MATERIAL_BOX2 = "material_box2";

    // ─── 셰이더 GLSL 파일 경로 (Program::CreateWithVSFS 인자) ───
    inline constexpr auto PATH_SHADER_LIGHTING_VS = "./resources/shader/lighting.vs";
    inline constexpr auto PATH_SHADER_LIGHTING_FS = "./resources/shader/lighting.fs";
    inline constexpr auto PATH_SHADER_SIMPLE_VS = "./resources/shader/simple.vs";
    inline constexpr auto PATH_SHADER_SIMPLE_FS = "./resources/shader/simple.fs";
    inline constexpr auto PATH_SHADER_TEXTURE_VS = "./resources/shader/texture.vs";
    inline constexpr auto PATH_SHADER_TEXTURE_FS = "./resources/shader/texture.fs";
    inline constexpr auto PATH_SHADER_POSTPROCESS_INVERT_FS = "./resources/shader/postprocess/invert.fs";
    inline constexpr auto PATH_SHADER_POSTPROCESS_GAMMA_FS = "./resources/shader/postprocess/gamma.fs";
    inline constexpr auto PATH_SHADER_POSTPROCESS_SHARPEN_FS = "./resources/shader/postprocess/sharpening.fs";
    inline constexpr auto PATH_SHADER_POSTPROCESS_BLUR_FS = "./resources/shader/postprocess/blurring.fs";
    inline constexpr auto PATH_SHADER_POSTPROCESS_SOBLE_FS = "./resources/shader/postprocess/sobel.fs";

    // ─── 텍스처 이미지 파일 경로 (Image::Load 인자) ───
    inline constexpr auto PATH_TEX_MARBLE = "./resources/texture/marble.jpg";
    inline constexpr auto PATH_TEX_CONTAINER = "./resources/texture/container.jpg";
    inline constexpr auto PATH_TEX_CONTAINER2 = "./resources/texture/container2.png";
    inline constexpr auto PATH_TEX_CONTAINER2_SPECULAR = "./resources/texture/container2_specular.png";
    inline constexpr auto PATH_TEX_WINDOW = "./resources/texture/blending_transparent_window.png";

    // ─── 셰이더 uniform 이름 ───
    inline constexpr auto UNI_BASE_COLOR = "baseColor";
    inline constexpr auto UNI_TRANSFORM_MAT = "transformMat";
    inline constexpr auto UNI_MODEL_TRANSFORM_MAT = "modelTransformMat";
    inline constexpr auto UNI_VIEW_POS = "viewPos";
    inline constexpr auto UNI_DIR_LIGHT = "dirLight";
    inline constexpr auto UNI_SPOT_LIGHT = "spotLight";
    inline constexpr auto UNI_DIR_LIGHT_ENABLED = "dirLightEnabled";
    inline constexpr auto UNI_SPOT_LIGHT_ENABLED = "spotLightEnabled";
    inline constexpr auto UNI_TEX = "tex";
    inline constexpr auto UNI_POSTPROCESS_FRAMETEXTURE = "frameTexture";
    inline constexpr auto UNI_POSTPROCESS_GAMMA = "gamma";
    inline constexpr auto UNI_MATERIAL_DIFFUSE = "material.diffuse";
    inline constexpr auto UNI_MATERIAL_SPECULAR = "material.specular";
    inline constexpr auto UNI_MATERIAL_SHININESS = "material.shininess";
    // 배열 uniform — prefix + 인덱스 + STR_INDEX_CLOSE 로 결합. 예: "pointLights[" + "0" + "]".
    inline constexpr auto UNI_POINT_LIGHTS_PREFIX = "pointLights[";
    inline constexpr auto UNI_POINT_LIGHTS_ENABLED_PREFIX = "pointLightsEnabled[";
    inline constexpr auto STR_INDEX_CLOSE = "]";

    // ─── uniform struct 멤버 suffix (program_uniforms 가 prefix 와 결합) ───
    inline constexpr auto SFX_DIRECTION = ".direction";
    inline constexpr auto SFX_POSITION = ".position";
    inline constexpr auto SFX_AMBIENT = ".ambient";
    inline constexpr auto SFX_DIFFUSE = ".diffuse";
    inline constexpr auto SFX_SPECULAR = ".specular";
    inline constexpr auto SFX_ATTENUATION = ".attenuation";
    inline constexpr auto SFX_CUTOFF = ".cutoff";
    inline constexpr auto SFX_OUTER_CUTOFF = ".outerCutoff";

    // ─── ImGui 위젯 라벨 (라벨이 위젯 ID 도 겸함 — 변경 시 상태 분리 주의) ───
    inline constexpr auto LBL_UI_WINDOW = "ui window";
    inline constexpr auto LBL_DIRLIGHT = "dirLight";
    inline constexpr auto LBL_DIR_ENABLED = "dir.enabled";
    inline constexpr auto LBL_DIR_DIRECTION = "dir.direction";
    inline constexpr auto LBL_DIR_AMBIENT = "dir.ambient";
    inline constexpr auto LBL_DIR_DIFFUSE = "dir.diffuse";
    inline constexpr auto LBL_DIR_SPECULAR = "dir.specular";
    inline constexpr auto LBL_POINTLIGHT_PREFIX = "pointLight[";
    inline constexpr auto LBL_P_ENABLED = "p.enabled";
    inline constexpr auto LBL_P_POSITION = "p.position";
    inline constexpr auto LBL_P_DISTANCE = "p.distance";
    inline constexpr auto LBL_P_AMBIENT = "p.ambient";
    inline constexpr auto LBL_P_DIFFUSE = "p.diffuse";
    inline constexpr auto LBL_P_SPECULAR = "p.specular";
    inline constexpr auto LBL_SPOTLIGHT = "spotLight";
    inline constexpr auto LBL_S_ENABLED = "s.enabled";
    inline constexpr auto LBL_S_POSITION = "s.position";
    inline constexpr auto LBL_S_DIRECTION = "s.direction";
    inline constexpr auto LBL_S_CUTOFF = "s.cutoff(deg)";
    inline constexpr auto LBL_S_OUTER_CUTOFF = "s.outerCutoff(deg)";
    inline constexpr auto LBL_S_DISTANCE = "s.distance";
    inline constexpr auto LBL_S_AMBIENT = "s.ambient";
    inline constexpr auto LBL_S_DIFFUSE = "s.diffuse";
    inline constexpr auto LBL_S_SPECULAR = "s.specular";
    inline constexpr auto LBL_FLASH_LIGHT = "flash light";
    inline constexpr auto LBL_ANIMATION = "animation";
    inline constexpr auto LBL_CLEAR_COLOR = "clear color";
    inline constexpr auto LBL_POSTPROCESS_GAMMA = "gamma";
    inline constexpr auto LBL_CAMERA_POS = "camera pos";
    inline constexpr auto LBL_CAMERA_YAW = "camera yaw";
    inline constexpr auto LBL_CAMERA_PITCH = "camera pitch";
    inline constexpr auto LBL_RESET_CAMERA = "reset camera";
    inline constexpr auto LBL_DEPTH_FUNC = "depth func";

    // ─── depth test 비교 연산자 — ImGui Combo 라벨 ↔ GL enum, 인덱스 1:1 동기 필수 ───
    // glClearDepth(1.0f): 제일 가까운 값 0, 제일 먼 값 1.
    // ┌───────┬─────────────┬──────────────────────────────────┐
    // │ 인덱스 │ 값          │ 의미                             │
    // ├───────┼─────────────┼──────────────────────────────────┤
    // │ 0     │ GL_ALWAYS   │ 항상 통과 (depth test 무력화 효과) │
    // │ 1     │ GL_NEVER    │ 항상 실패 (아무것도 안 그려짐)    │
    // │ 2     │ GL_LESS     │ 더 가까우면 통과 (기본값)        │
    // │ 3     │ GL_LEQUAL   │ 같거나 가까우면 통과             │
    // │ 4     │ GL_GREATER  │ 더 멀면 통과                     │
    // │ 5     │ GL_GEQUAL   │ 같거나 멀면 통과                 │
    // │ 6     │ GL_EQUAL    │ 깊이 같을 때만                   │
    // │ 7     │ GL_NOTEQUAL │ 깊이 다를 때만                   │
    // └───────┴─────────────┴──────────────────────────────────┘
    inline constexpr const char *DEPTH_FUNC_LABELS[8] = {
        "GL_ALWAYS", "GL_NEVER",
        "GL_LESS", "GL_LEQUAL",
        "GL_GREATER", "GL_GEQUAL",
        "GL_EQUAL", "GL_NOTEQUAL"};

    inline constexpr GLenum DEPTH_FUNC[8] = {
        GL_ALWAYS, GL_NEVER,
        GL_LESS, GL_LEQUAL,
        GL_GREATER, GL_GEQUAL,
        GL_EQUAL, GL_NOTEQUAL};
}

#endif // __SJH_CONSTANTS_H__
