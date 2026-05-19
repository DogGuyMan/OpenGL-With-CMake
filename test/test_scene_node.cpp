/**
 * @file test_scene_node.cpp
 * @brief SJH::SceneNode — 월드 행렬 캐싱 + 계층 (GL 컨텍스트 불요).
 */
#include "object/scene_node.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
    void RequireMatNear(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-5f)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                REQUIRE_THAT(a[c][r], WithinAbs(b[c][r], eps));
    }
}

TEST_CASE("SceneNode 기본 — 부모 없는 노드의 World == 로컬 행렬", "[scene_node]")
{
    SJH::SceneNode node;
    RequireMatNear(node.World(), glm::mat4(1.0f));

    SJH::Transform t;
    t.Translate = glm::vec3(1.0f, 2.0f, 3.0f);
    node.SetLocal(t);
    RequireMatNear(node.World(), t.GetLocalMatrix());
}

TEST_CASE("SceneNode SetTranslate/SetScale — World 즉시 반영", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(5.0f, 0.0f, 0.0f));
    node.SetScale(glm::vec3(2.0f, 2.0f, 2.0f));

    SJH::Transform expected;
    expected.Translate = glm::vec3(5.0f, 0.0f, 0.0f);
    expected.Scale = glm::vec3(2.0f, 2.0f, 2.0f);
    RequireMatNear(node.World(), expected.GetLocalMatrix());
}

TEST_CASE("SceneNode 캐시 — 변경 없이 두 번 호출하면 동일", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(1.0f, 1.0f, 1.0f));
    const glm::mat4 first = node.World();
    const glm::mat4 second = node.World();
    RequireMatNear(first, second);
}

TEST_CASE("SceneNode 계층 — World 는 부모 합성", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);

    REQUIRE(child.Parent() == &root);
    RequireMatNear(child.World(), root.World() * child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode 하향 dirty 전파 — 부모 변경이 자식 World 갱신", "[scene_node]")
{
    SJH::SceneNode root, child;
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);
    (void)child.World(); // 캐시 채움

    root.SetTranslate(glm::vec3(100.0f, 0.0f, 0.0f)); // 부모 이동
    RequireMatNear(child.World(), root.World() * child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode Detach — 분리 후 자식은 부모 영향 제거", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetTranslate(glm::vec3(10.0f, 0.0f, 0.0f));
    child.SetTranslate(glm::vec3(0.0f, 5.0f, 0.0f));
    root.Attach(&child);
    root.Detach(&child);

    REQUIRE(child.Parent() == nullptr);
    RequireMatNear(child.World(), child.Local().GetLocalMatrix());
}

TEST_CASE("SceneNode 재부착 — 새 부모로 옮기면 옛 부모 자식목록서 제거", "[scene_node]")
{
    SJH::SceneNode a, b, child;
    a.Attach(&child);
    b.Attach(&child); // child 를 b 로 이동
    REQUIRE(child.Parent() == &b);
    REQUIRE(a.Children().empty());
    REQUIRE(b.Children().size() == 1);
}

TEST_CASE("SceneNode 순환 가드 — 조상을 자식으로 부착 시 거부", "[scene_node]")
{
    SJH::SceneNode root, mid, leaf;
    root.Attach(&mid);
    mid.Attach(&leaf);
    leaf.Attach(&root); // leaf 아래에 조상 root 부착 시도 — 거부되어야 함
    REQUIRE(root.Parent() == nullptr);   // root 는 여전히 부모 없음
    REQUIRE(leaf.Children().empty());    // leaf 는 자식 없음
}

TEST_CASE("SceneNode WorldForward — 기본은 -Z", "[scene_node]")
{
    SJH::SceneNode node;
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(-1.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — yaw 90도면 -X", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetEulerRot(glm::vec3(0.0f, 90.0f, 0.0f));
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — pitch -90도면 -Y", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetEulerRot(glm::vec3(-90.0f, 0.0f, 0.0f));
    const glm::vec3 f = node.WorldForward();
    REQUIRE_THAT(f.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(f.y, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode WorldForward — 부모 회전 반영", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.SetEulerRot(glm::vec3(0.0f, 90.0f, 0.0f));
    root.Attach(&child);
    const glm::vec3 f = child.WorldForward(); // child 로컬 identity, 부모 yaw 90
    REQUIRE_THAT(f.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(f.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("SceneNode TranslateBy — 증분 누적", "[scene_node]")
{
    SJH::SceneNode node;
    node.SetTranslate(glm::vec3(1.0f, 0.0f, 0.0f));
    node.TranslateBy(glm::vec3(2.0f, 3.0f, 0.0f));
    const glm::mat4 w = node.World();
    REQUIRE_THAT(w[3][0], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(w[3][1], WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("SceneNode TranslateBy — 자식 World 갱신 (dirty 전파)", "[scene_node]")
{
    SJH::SceneNode root, child;
    root.Attach(&child);
    (void)child.World();
    root.TranslateBy(glm::vec3(5.0f, 0.0f, 0.0f));
    const glm::mat4 cw = child.World();
    REQUIRE_THAT(cw[3][0], WithinAbs(5.0f, 1e-5f));
}
