/**
 * @file scene_graph.h
 * @brief enum 인덱스 기반 씬 그래프 — 노드 소유 + enum->노드 O(1) 조회 파사드.
 */
#ifndef __SJH_SCENE_GRAPH_H__
#define __SJH_SCENE_GRAPH_H__

#include "object/scene_node.h"
#include <array>
#include <cstddef>

namespace SJH
{
    /**
     * @brief enum 인덱스 기반 씬 그래프.
     * @tparam TId 마지막 항목이 `Count` 인 enum — 정수값이 노드 배열 인덱스.
     * @details 모든 @ref SceneNode 를 std::array 로 소유한다. 배열이라 노드 주소가
     *          고정되므로 노드 간 부모/자식 포인터가 안전하다. SceneNode 가 복사/이동
     *          불가이므로 SceneGraph 도 복사/이동 불가 — 소유자가 멤버로 in-place 생성한다.
     */
    template <typename TId>
    class SceneGraph
    {
    public:
        /// @brief 노드 개수 — TId::Count 의 정수값.
        static constexpr std::size_t Count = static_cast<std::size_t>(TId::Count);

        /// @brief @p id 노드 참조 (변경 가능).
        SceneNode &At(TId id) { return mNodes[Index(id)]; }
        /// @brief @p id 노드 참조 (읽기 전용).
        const SceneNode &At(TId id) const { return mNodes[Index(id)]; }

        /// @brief @p id 노드의 월드 행렬.
        glm::mat4 World(TId id) const { return mNodes[Index(id)].World(); }

        /// @brief @p id 노드의 월드 전방 벡터.
        glm::vec3 WorldForward(TId id) const { return mNodes[Index(id)].WorldForward(); }

        /// @brief @p id 노드의 로컬 변환 일괄 교체.
        void SetLocal(TId id, const Transform &local) { mNodes[Index(id)].SetLocal(local); }

        /// @brief @p child 를 @p parent 의 자식으로 부착.
        void Attach(TId child, TId parent) { mNodes[Index(parent)].Attach(&mNodes[Index(child)]); }

        /// @brief @p child 를 현재 부모에서 분리.
        void Detach(TId child)
        {
            SceneNode &node = mNodes[Index(child)];
            if (node.Parent() != nullptr)
                node.Parent()->Detach(&node);
        }

    private:
        static std::size_t Index(TId id) { return static_cast<std::size_t>(id); }

        std::array<SceneNode, Count> mNodes;
    };
}
#endif // __SJH_SCENE_GRAPH_H__
