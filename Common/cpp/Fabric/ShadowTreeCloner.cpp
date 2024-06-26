#ifdef RCT_NEW_ARCH_ENABLED

#include <utility>

#include "ShadowTreeCloner.h"

namespace reanimated {

ShadowNode::Unshared cloneShadowTreeWithNewProps(
    const ShadowNode::Shared &oldRootNode,
    const ShadowNodeFamily &family,
    RawProps &&rawProps) {
  // adapted from ShadowNode::cloneTree

  auto ancestors = family.getAncestors(*oldRootNode);

  if (ancestors.empty()) {
    return ShadowNode::Unshared{nullptr};
  }

  auto &parent = ancestors.back();
  auto &source = parent.first.get().getChildren().at(parent.second);

  PropsParserContext propsParserContext{
      source->getSurfaceId(), *source->getContextContainer()};
  const auto props = source->getComponentDescriptor().cloneProps(
      propsParserContext, source->getProps(), std::move(rawProps));

  auto newChildNode = source->clone(
      {/* .props = */ props,
       ShadowNodeFragment::childrenPlaceholder(),
       source->getState()});

  for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
    auto &parentNode = it->first.get();
    auto childIndex = it->second;

    auto children = parentNode.getChildren();
    const auto &oldChildNode = *children.at(childIndex);
    react_native_assert(ShadowNode::sameFamily(oldChildNode, *newChildNode));

    if (!parentNode.getSealed()) {
      // Optimization: if a ShadowNode is unsealed, we can directly update its
      // children instead of cloning the whole path to the root node.
      auto &parentNodeNonConst = const_cast<ShadowNode &>(parentNode);
      parentNodeNonConst.replaceChild(oldChildNode, newChildNode, childIndex);
      // Unfortunately, `replaceChild` does not update Yoga nodes, so we need to
      // update them manually here.
      static_cast<YogaLayoutableShadowNode *>(&parentNodeNonConst)
          ->updateYogaChildren();
      return std::const_pointer_cast<ShadowNode>(oldRootNode);
    }

    children[childIndex] = newChildNode;

    newChildNode = parentNode.clone(
        {ShadowNodeFragment::propsPlaceholder(),
         std::make_shared<ShadowNode::ListOfShared>(children),
         parentNode.getState()});
  }

  return std::const_pointer_cast<ShadowNode>(newChildNode);
}

} // namespace reanimated

#endif // RCT_NEW_ARCH_ENABLED
