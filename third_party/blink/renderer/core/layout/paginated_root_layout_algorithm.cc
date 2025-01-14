// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/paginated_root_layout_algorithm.h"

#include <algorithm>

#include "third_party/blink/renderer/core/layout/block_layout_algorithm.h"
#include "third_party/blink/renderer/core/layout/constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/length_utils.h"
#include "third_party/blink/renderer/core/layout/logical_box_fragment.h"
#include "third_party/blink/renderer/core/layout/out_of_flow_layout_part.h"
#include "third_party/blink/renderer/core/layout/physical_box_fragment.h"
#include "third_party/blink/renderer/core/layout/simplified_oof_layout_algorithm.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

PaginatedRootLayoutAlgorithm::PaginatedRootLayoutAlgorithm(
    const LayoutAlgorithmParams& params)
    : LayoutAlgorithm(params) {}

const LayoutResult* PaginatedRootLayoutAlgorithm::Layout() {
  DCHECK(!GetBreakToken());
  auto writing_direction = GetConstraintSpace().GetWritingDirection();
  const BlockBreakToken* break_token = nullptr;
  LayoutUnit intrinsic_block_size;
  LogicalOffset page_offset;
  uint32_t page_index = 0;
  AtomicString page_name;

  container_builder_.SetIsBlockFragmentationContextRoot();

  do {
    // Lay out one page. Each page will become a fragment.
    const PhysicalBoxFragment* page =
        LayoutPage(page_index, page_name, break_token);

    if (page_name != page->PageName()) {
      // The page name changed. This may mean that the page size has changed as
      // well. We need to re-match styles and try again.
      //
      // Note: In many cases it could be possible to know the correct name of
      // the page before laying it out, by providing such information in the
      // break token, for instance. However, that's not going to work if the
      // very first page is named, since there's no break token then. So, given
      // that we may have to go back and re-layout in some cases, just do this
      // in all cases where named pages are involved, rather than having two
      // separate mechanisms. We could revisit this approach if it turns out to
      // be a performance problem (although that seems very unlikely).
      page_name = page->PageName();
      page = LayoutPage(page_index, page_name, break_token);
      DCHECK_EQ(page_name, page->PageName());
    }

    container_builder_.AddChild(*page, page_offset);

    LayoutUnit page_block_size =
        LogicalFragment(writing_direction, *page).BlockSize();
    intrinsic_block_size = std::max(intrinsic_block_size,
                                    page_offset.block_offset + page_block_size);
    page_offset.block_offset += page_block_size;
    break_token = page->GetBreakToken();
    page_index++;
  } while (break_token);

  container_builder_.SetIntrinsicBlockSize(intrinsic_block_size);

  // Compute the block-axis size now that we know our content size.
  LayoutUnit block_size = ComputeBlockSizeForFragment(
      GetConstraintSpace(), Style(), /* border_padding */ BoxStrut(),
      intrinsic_block_size, kIndefiniteSize);
  container_builder_.SetFragmentsTotalBlockSize(block_size);

  OutOfFlowLayoutPart(Node(), GetConstraintSpace(), &container_builder_).Run();

  return container_builder_.ToBoxFragment();
}

const PhysicalBoxFragment& PaginatedRootLayoutAlgorithm::CreateEmptyPage(
    const BlockNode& node,
    const ConstraintSpace& parent_space,
    const PhysicalBoxFragment& previous_fragmentainer) {
  WritingMode writing_mode = parent_space.GetWritingMode();
  // TODO(mstensho): We can do better than just using the size of the last page
  // (figure out the correct page size by checking the page index and name), but
  // there are other parts of the code that assume this behavior.
  LogicalSize page_size =
      previous_fragmentainer.Size().ConvertToLogical(writing_mode);
  ConstraintSpace fragmentainer_space =
      CreateConstraintSpaceForPages(node, parent_space, page_size);
  const BlockBreakToken* break_token = previous_fragmentainer.GetBreakToken();
  FragmentGeometry fragment_geometry =
      CalculateInitialFragmentGeometry(fragmentainer_space, node, break_token);
  LayoutAlgorithmParams params(node, fragment_geometry, fragmentainer_space,
                               break_token);
  SimplifiedOofLayoutAlgorithm algorithm(params, previous_fragmentainer);
  return To<PhysicalBoxFragment>(algorithm.Layout()->GetPhysicalFragment());
}

const PhysicalBoxFragment* PaginatedRootLayoutAlgorithm::LayoutPage(
    uint32_t page_index,
    const AtomicString& page_name,
    const BlockBreakToken* break_token) const {
  const LayoutView* view = Node().GetDocument().GetLayoutView();
  WritingMode writing_mode = GetConstraintSpace().GetWritingMode();
  LogicalSize page_size =
      view->PageAreaSize(page_index, page_name).ConvertToLogical(writing_mode);

  DCHECK(page_size.inline_size != kIndefiniteSize);
  DCHECK(page_size.block_size != kIndefiniteSize);
  ConstraintSpace child_space =
      CreateConstraintSpaceForPages(Node(), GetConstraintSpace(), page_size);
  FragmentGeometry fragment_geometry =
      CalculateInitialFragmentGeometry(child_space, Node(), GetBreakToken());
  BlockLayoutAlgorithm child_algorithm(
      {Node(), fragment_geometry, child_space, break_token});
  child_algorithm.SetBoxType(PhysicalFragment::kPageBox);
  const LayoutResult* result = child_algorithm.Layout();
  return &To<PhysicalBoxFragment>(result->GetPhysicalFragment());
}

ConstraintSpace PaginatedRootLayoutAlgorithm::CreateConstraintSpaceForPages(
    const BlockNode& node,
    const ConstraintSpace& space,
    const LogicalSize& page_size) {
  ConstraintSpaceBuilder space_builder(
      space, node.Style().GetWritingDirection(), /*is_new_fc=*/true);
  space_builder.SetAvailableSize(page_size);
  space_builder.SetPercentageResolutionSize(page_size);
  space_builder.SetInlineAutoBehavior(AutoSizeBehavior::kStretchImplicit);

  space_builder.SetFragmentationType(kFragmentPage);
  space_builder.SetShouldPropagateChildBreakValues();
  space_builder.SetFragmentainerBlockSize(page_size.block_size);
  space_builder.SetIsAnonymous(true);

  return space_builder.ToConstraintSpace();
}

}  // namespace blink
