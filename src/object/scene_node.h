/**
 * @file scene_node.h
 * @brief intrusive 씬 그래프 노드 — Transform payload + 부모/자식 + 월드 행렬 캐싱.
 */
#ifndef __SJH_SCENE_NODE_H__
#define __SJH_SCENE_NODE_H__

#include "object/transform.h"
#include <glm/glm.hpp>
#include <vector>

namespace SJH
{
    /**
     * @brief intrusive 씬 그래프 노드.
     * @details 로컬 @ref Transform 을 payload 로 들고 부모/자식 포인터로 계층을 구성한다.
     *          월드 행렬은 캐싱하며 dirty 플래그로 무효화한다 (변경 시 자손까지 하향 전파).
     *          복사/이동 불가 — 다른 노드가 포인터로 참조하므로 주소가 고정되어야 한다.
     */
    class SceneNode
    {
    public:
        SceneNode() = default;
        ~SceneNode() = default;
        SceneNode(const SceneNode &) = delete;
        SceneNode &operator=(const SceneNode &) = delete;
        SceneNode(SceneNode &&) = delete;
        SceneNode &operator=(SceneNode &&) = delete;

        /// @brief 로컬 변환 읽기.
        const Transform &Local() const { return mLocal; }
        /// @brief 부모 노드 (루트면 nullptr).
        SceneNode *Parent() const { return mParent; }
        /// @brief 직속 자식 목록 (비소유 관찰자).
        const std::vector<SceneNode *> &Children() const { return mChildren; }

        /// @brief 월드 행렬 — dirty 면 재계산 후 캐시. parent 가 있으면 parent->World() 합성.
        glm::mat4 World() const;
        /// @brief 노드의 월드 전방 단위 벡터 — 회전된 -Z. 카메라 front·라이트 방향에 사용.
        glm::vec3 WorldForward() const;

        /// @brief 로컬 변환 일괄 교체 — 자신+자손 dirty.
        void SetLocal(const Transform &local);
        /// @brief 로컬 Translate 만 교체 — 자신+자손 dirty.
        void SetTranslate(const glm::vec3 &t);
        /// @brief 로컬 EulerRot 만 교체 — 자신+자손 dirty.
        void SetEulerRot(const glm::vec3 &r);
        /// @brief 로컬 Scale 만 교체 — 자신+자손 dirty.
        void SetScale(const glm::vec3 &s);
        /// @brief 로컬 Translate 를 @p delta 만큼 증분 — 자신+자손 dirty.
        void TranslateBy(const glm::vec3 &delta);

        /**
         * @brief @p child 를 이 노드의 자식으로 부착.
         * @details child 가 이미 부모를 가지면 먼저 분리한다. child 가 nullptr·자기 자신·
         *          이 노드의 조상이면 (순환) 아무 일도 하지 않는다.
         */
        void Attach(SceneNode *child);
        /// @brief @p child 를 자식 목록에서 분리 (child 의 부모를 nullptr 로).
        void Detach(SceneNode *child);

    private:
        void MarkSubtreeDirty();
        bool IsAncestorOf(const SceneNode *node) const;

        Transform mLocal;
        SceneNode *mParent = nullptr;             ///< 비소유 — SceneGraph 가 노드를 소유.
        std::vector<SceneNode *> mChildren;       ///< 비소유 관찰자.
        mutable glm::mat4 mWorldCache{1.0f};
        mutable bool mDirty = true;
    };
}
#endif // __SJH_SCENE_NODE_H__
