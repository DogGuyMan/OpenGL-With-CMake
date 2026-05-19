#include "object/scene_node.h"
#include <algorithm>

namespace SJH
{
    glm::mat4 SceneNode::World() const
    {
        if (mDirty)
        {
            const glm::mat4 parentWorld = mParent ? mParent->World() : glm::mat4(1.0f);
            mWorldCache = parentWorld * mLocal.GetLocalMatrix();
            mDirty = false;
        }
        return mWorldCache;
    }

    glm::vec3 SceneNode::WorldForward() const
    {
        return glm::normalize(glm::vec3(World() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    }

    void SceneNode::SetLocal(const Transform &local)
    {
        mLocal = local;
        MarkSubtreeDirty();
    }

    void SceneNode::SetTranslate(const glm::vec3 &t)
    {
        mLocal.Translate = t;
        MarkSubtreeDirty();
    }

    void SceneNode::SetEulerRot(const glm::vec3 &r)
    {
        mLocal.EulerRot = r;
        MarkSubtreeDirty();
    }

    void SceneNode::SetScale(const glm::vec3 &s)
    {
        mLocal.Scale = s;
        MarkSubtreeDirty();
    }

    void SceneNode::TranslateBy(const glm::vec3 &delta)
    {
        mLocal.Translate += delta;
        MarkSubtreeDirty();
    }

    void SceneNode::Attach(SceneNode *child)
    {
        if (child == nullptr || child == this)
            return;
        // 순환 가드 — child 가 이 노드의 조상이면 부착 시 사이클이 생긴다.
        if (child->IsAncestorOf(this))
            return;
        // 이미 다른 부모가 있으면 먼저 분리.
        if (child->mParent != nullptr)
            child->mParent->Detach(child);
        child->mParent = this;
        mChildren.push_back(child);
        child->MarkSubtreeDirty();
    }

    void SceneNode::Detach(SceneNode *child)
    {
        if (child == nullptr)
            return;
        auto it = std::find(mChildren.begin(), mChildren.end(), child);
        if (it == mChildren.end())
            return;
        mChildren.erase(it);
        child->mParent = nullptr;
        child->MarkSubtreeDirty();
    }

    bool SceneNode::IsAncestorOf(const SceneNode *node) const
    {
        for (const SceneNode *cur = node; cur != nullptr; cur = cur->mParent)
            if (cur == this)
                return true;
        return false;
    }

    void SceneNode::MarkSubtreeDirty()
    {
        mDirty = true;
        for (SceneNode *child : mChildren)
            child->MarkSubtreeDirty();
    }
}
