// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import UIKit
import ios_chrome_browser_shared_ui_util_util_swift
import ios_chrome_browser_ui_tab_switcher_tab_strip_ui_swift_constants

/// View Controller displaying the TabStrip.
@objcMembers
class TabStripViewController: UIViewController,
  TabStripConsumer, TabStripNewTabButtonDelegate, TabStripTabCellDelegate
{

  // The enum used by the data source to manage the sections.
  enum Section: Int {
    case tabs
  }

  private let layout: TabStripLayout
  // The CollectionView used to display the items.
  private let collectionView: UICollectionView
  // The DataSource for this collection view.
  private lazy var dataSource = createDataSource(
    tabCellRegistration: tabCellRegistration, groupCellRegistration: groupCellRegistration)
  private lazy var tabCellRegistration = createTabCellRegistration()
  private lazy var groupCellRegistration = createGroupCellRegistration()

  // Associates data to `dataSource` items, to reconfigure cells.
  private let itemData = NSMutableDictionary()

  // The New tab button.
  private let newTabButton: TabStripNewTabButton = TabStripNewTabButton()

  // Static decoration views that border the collection view. They are
  // visible when the selected cell reaches an edge of the collection view and
  // if the collection view can be scrolled.
  private let leftStaticSeparator: TabStripDecorationView = TabStripDecorationView()
  private let rightStaticSeparator: TabStripDecorationView = TabStripDecorationView()

  // Latest dragged item. This property is set when the item
  // is long pressed which does not always result in a drag action.
  private var draggedItem: TabSwitcherItem?

  // The item currently selected in the tab strip.
  // The collection view appears to sometimes forget what item is selected,
  // so it is better to store this information here rather than directly using
  // `collectionView.selectItem:` and `collectionView.deselectItem:`.
  // `self.ensureSelectedItemIsSelected()` is used to ensure the value of
  // `collectionView.indexPathsForSelectedItems` remains consistent with `selectedItem`.
  private var selectedItem: TabSwitcherItem? {
    didSet { self.ensureSelectedItemIsSelected() }
  }

  /// `true` if the dragged tab moved to a new index.
  private var dragEndAtNewIndex: Bool = false

  /// `true` if a drop animation is in progress.
  private var dropAnimationInProgress: Bool = false

  /// Targeted scroll offset, used on iOS 16 only.
  /// On iOS 16, the scroll animation after opening a new tab is delayed.
  /// This variable ensures that the most recent scroll event is processed.
  private var targetedScrollOffsetiOS16: CGFloat = 0

  /// `true` if the user is in incognito.
  public var isIncognito: Bool = false

  private var numberOfTabs: Int = 0

  /// Handles model updates.
  public weak var mutator: TabStripMutator?
  /// Tab strip delegate.
  public weak var delegate: TabStripViewControllerDelegate?
  /// Handles drag and drop interactions.
  public weak var dragDropHandler: TabCollectionDragDropHandler?

  /// The LayoutGuideCenter.
  @objc public var layoutGuideCenter: LayoutGuideCenter? {
    didSet {
      layoutGuideCenter?.reference(view: newTabButton.layoutGuideView, under: kNewTabButtonGuide)
    }
  }

  init() {
    layout = TabStripLayout()
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    super.init(nibName: nil, bundle: nil)

    layout.dataSource = dataSource
    layout.leftStaticSeparator = leftStaticSeparator
    layout.rightStaticSeparator = rightStaticSeparator
    layout.newTabButton = newTabButton

    collectionView.delegate = self
    collectionView.dragDelegate = self
    collectionView.dropDelegate = self
    collectionView.showsHorizontalScrollIndicator = false
    collectionView.allowsMultipleSelection = false
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) is not supported")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor(named: kGroupedPrimaryBackgroundColor)

    // Don't clip to bound the collection view to allow the shadow of the long press to be displayed fully.
    // The trailing placeholder will ensure that the cells aren't displayed out of the bounds.
    collectionView.clipsToBounds = false
    collectionView.translatesAutoresizingMaskIntoConstraints = false
    collectionView.backgroundColor = .clear
    view.addSubview(collectionView)

    let trailingPlaceholder = UIView()
    trailingPlaceholder.translatesAutoresizingMaskIntoConstraints = false
    trailingPlaceholder.backgroundColor = view.backgroundColor
    // Add the placeholder above the collection view but below the other elements.
    view.insertSubview(trailingPlaceholder, aboveSubview: collectionView)

    // Mirror the layer.
    rightStaticSeparator.transform = CGAffineTransformMakeScale(-1, 1)
    view.addSubview(leftStaticSeparator)
    view.addSubview(rightStaticSeparator)

    newTabButton.delegate = self
    newTabButton.isIncognito = isIncognito
    view.addSubview(newTabButton)

    if TabStripFeaturesUtils.isModernTabStripNewTabButtonDynamic() {
      NSLayoutConstraint.activate([
        collectionView.trailingAnchor.constraint(
          equalTo: view.trailingAnchor, constant: -TabStripConstants.NewTabButton.width),
        newTabButton.leadingAnchor.constraint(
          greaterThanOrEqualTo: view.leadingAnchor),
        newTabButton.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor),
      ])
    } else {
      NSLayoutConstraint.activate([
        newTabButton.leadingAnchor.constraint(
          equalTo: collectionView.trailingAnchor),
        newTabButton.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      ])
    }

    NSLayoutConstraint.activate(
      [
        /// `trailingPlaceholder` constraints.
        trailingPlaceholder.leadingAnchor.constraint(equalTo: collectionView.trailingAnchor),
        trailingPlaceholder.trailingAnchor.constraint(equalTo: view.trailingAnchor),
        trailingPlaceholder.topAnchor.constraint(equalTo: view.topAnchor),
        trailingPlaceholder.bottomAnchor.constraint(equalTo: view.bottomAnchor),

        /// `collectionView` constraints.
        collectionView.leadingAnchor.constraint(
          equalTo: view.leadingAnchor),
        collectionView.topAnchor.constraint(
          equalTo: view.topAnchor),
        collectionView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

        /// `newTabButton` constraints.
        newTabButton.leadingAnchor.constraint(
          greaterThanOrEqualTo: view.leadingAnchor),
        newTabButton.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor),
        newTabButton.bottomAnchor.constraint(equalTo: view.bottomAnchor),
        newTabButton.topAnchor.constraint(equalTo: view.topAnchor),
        newTabButton.widthAnchor.constraint(equalToConstant: TabStripConstants.NewTabButton.width),

        /// `leftStaticSeparator` constraints.
        leftStaticSeparator.leftAnchor.constraint(equalTo: collectionView.leftAnchor),
        leftStaticSeparator.bottomAnchor.constraint(
          equalTo: collectionView.bottomAnchor,
          constant: -TabStripConstants.StaticSeparator.bottomInset),
        /// `rightStaticSeparator` constraints.
        rightStaticSeparator.rightAnchor.constraint(equalTo: collectionView.rightAnchor),
        rightStaticSeparator.bottomAnchor.constraint(
          equalTo: collectionView.bottomAnchor,
          constant: -TabStripConstants.StaticSeparator.bottomInset),
      ])
  }

  override func viewWillTransition(
    to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)
    weak var weakSelf = self
    coordinator.animate(alongsideTransition: nil) { _ in
      // The tab cell size must be updated after the transition completes.
      // Otherwise the collection view width won't be updated.
      weakSelf?.layout.calculateTabCellSize()
      weakSelf?.layout.invalidateLayout()
    }
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    self.ensureSelectedItemIsSelected()
  }

  // MARK: - TabStripConsumer

  func populate(
    items itemIdentifiers: [TabStripItemIdentifier]?, selectedItem: TabSwitcherItem?,
    itemData: [TabStripItemIdentifier: TabStripItemData],
    itemParents: [TabStripItemIdentifier: TabGroupItem]
  ) {
    guard let itemIdentifiers = itemIdentifiers else {
      return
    }
    numberOfTabs = itemIdentifiers.lazy.filter { $0.itemType == .tab }.count

    var snapshot = NSDiffableDataSourceSectionSnapshot<TabStripItemIdentifier>()
    for itemIdentifier in itemIdentifiers {
      switch itemIdentifier.item {
      case .group(let tabGroupItem):
        snapshot.append([itemIdentifier])
        if !tabGroupItem.collapsed {
          snapshot.expand([itemIdentifier])
        }
      case .tab(_):
        snapshot.append([itemIdentifier], to: TabStripItemIdentifier(itemParents[itemIdentifier]))
      }
    }
    self.itemData.setDictionary(itemData)

    // TODO(crbug.com/325415449): Update this when #unavailable is rocognized by
    // the formatter.
    if #available(iOS 17.0, *) {
    } else {
      layout.cellAnimatediOS16 = true
    }

    // To make the animation smoother, try to select the item if it's already
    // present in the collection view.
    selectItem(selectedItem)
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot,
      animatingDifferences: !UIAccessibility.isReduceMotionEnabled,
      numberOfVisibleItemsChanged: true)
    selectItem(selectedItem)
  }

  func selectItem(_ item: TabSwitcherItem?) {
    self.selectedItem = item
  }

  func reconfigureItems(_ items: [TabStripItemIdentifier]) {
    var snapshot = dataSource.snapshot()
    let itemsInSnapshot = Set(snapshot.itemIdentifiers)
    let itemsToReconfigure = Set(items).intersection(itemsInSnapshot)
    snapshot.reconfigureItems(Array(itemsToReconfigure))
    dataSource.apply(snapshot)
  }

  func moveItem(
    _ itemToMoveIdentifier: TabStripItemIdentifier,
    beforeItem destinationItemIdentifier: TabStripItemIdentifier?
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    let itemIsExpanded = snapshot.isExpanded(itemToMoveIdentifier)
    let childItems = snapshot.snapshot(of: itemToMoveIdentifier).items
    snapshot.delete([itemToMoveIdentifier])
    if let destinationItemIdentifier = destinationItemIdentifier {
      snapshot.insert([itemToMoveIdentifier], before: destinationItemIdentifier)
    } else {
      snapshot.append([itemToMoveIdentifier])
    }
    if itemIsExpanded {
      snapshot.expand([itemToMoveIdentifier])
    }
    if !childItems.isEmpty {
      snapshot.append(childItems, to: itemToMoveIdentifier)
    }
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot, animatingDifferences: true)
    layout.invalidateLayout()
  }

  func moveItem(
    _ itemToMoveIdentifier: TabStripItemIdentifier,
    afterItem destinationItemIdentifier: TabStripItemIdentifier?
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    let itemIsExpanded = snapshot.isExpanded(itemToMoveIdentifier)
    let childItems = snapshot.snapshot(of: itemToMoveIdentifier).items
    snapshot.delete([itemToMoveIdentifier])
    if let destinationItemIdentifier = destinationItemIdentifier {
      snapshot.insert([itemToMoveIdentifier], after: destinationItemIdentifier)
    } else if let firstItemIdentifier = snapshot.items.first {
      snapshot.insert([itemToMoveIdentifier], before: firstItemIdentifier)
    } else {
      snapshot.append([itemToMoveIdentifier])
    }
    if itemIsExpanded {
      snapshot.expand([itemToMoveIdentifier])
    }
    if !childItems.isEmpty {
      snapshot.append(childItems, to: itemToMoveIdentifier)
    }
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot, animatingDifferences: true)
    layout.invalidateLayout()
  }

  func moveItem(
    _ itemToMoveIdentifier: TabStripItemIdentifier, insideGroup parentItem: TabGroupItem
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.delete([itemToMoveIdentifier])
    snapshot.append([itemToMoveIdentifier], to: TabStripItemIdentifier(parentItem))
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot, animatingDifferences: true)
    layout.invalidateLayout()
  }

  func insertItems(
    _ itemIdentifiers: [TabStripItemIdentifier],
    beforeItem destinationItemIdentifier: TabStripItemIdentifier?
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    if let destinationItemIdentifier = destinationItemIdentifier {
      snapshot.insert(itemIdentifiers, before: destinationItemIdentifier)
    } else {
      snapshot.append(itemIdentifiers)
    }
    snapshot.expand(itemIdentifiers.lazy.filter { $0.itemType == .group })
    let insertedLast = snapshot.items.last == itemIdentifiers.last
    insertItemsUsingSnapshot(snapshot, insertedLast: insertedLast)
  }

  func insertItems(
    _ itemIdentifiers: [TabStripItemIdentifier],
    afterItem destinationItemIdentifier: TabStripItemIdentifier?
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    if let destinationItemIdentifier = destinationItemIdentifier {
      snapshot.insert(itemIdentifiers, after: destinationItemIdentifier)
    } else if let firstItemIdentifier = snapshot.items.first {
      snapshot.insert(itemIdentifiers, before: firstItemIdentifier)
    } else {
      snapshot.append(itemIdentifiers)
    }
    snapshot.expand(itemIdentifiers.lazy.filter { $0.itemType == .group })
    let insertedLast = snapshot.items.last == itemIdentifiers.last
    insertItemsUsingSnapshot(snapshot, insertedLast: insertedLast)
  }

  func insertItems(
    _ itemIdentifiers: [TabStripItemIdentifier],
    insideGroup parentItem: TabGroupItem
  ) {
    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.append(itemIdentifiers, to: TabStripItemIdentifier(parentItem))
    let insertedLast = snapshot.items.last == itemIdentifiers.last
    insertItemsUsingSnapshot(snapshot, insertedLast: insertedLast)
  }

  func removeItems(_ items: [TabStripItemIdentifier]?) {
    guard let items = items else { return }

    numberOfTabs -= items.lazy.filter { $0.itemType == .tab }.count

    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.delete(items)
    itemData.removeObjects(forKeys: items)
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot,
      animatingDifferences: !UIAccessibility.isReduceMotionEnabled,
      numberOfVisibleItemsChanged: true)
  }

  func replaceItem(_ oldItem: TabSwitcherItem?, withItem newItem: TabSwitcherItem?) {
    guard let oldItem = TabStripItemIdentifier.tabIdentifier(oldItem),
      let newItem = TabStripItemIdentifier.tabIdentifier(newItem)
    else {
      return
    }

    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.insert([newItem], before: oldItem)
    snapshot.delete([oldItem])
    itemData.removeObject(forKey: oldItem)
    applySnapshot(dataSource: dataSource, snapshot: snapshot)
  }

  func updateItemData(
    _ updatedItemData: [TabStripItemIdentifier: TabStripItemData], reconfigureItems: Bool = false
  ) {
    itemData.addEntries(from: updatedItemData)
    if reconfigureItems {
      self.reconfigureItems(Array(updatedItemData.keys))
    }
  }

  func collapseGroup(_ group: TabGroupItem) {
    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.collapse([TabStripItemIdentifier(group)])
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot, animatingDifferences: true,
      numberOfVisibleItemsChanged: true)
  }

  func expandGroup(_ group: TabGroupItem) {
    var snapshot = dataSource.snapshot(for: .tabs)
    snapshot.expand([TabStripItemIdentifier(group)])
    applySnapshot(
      dataSource: dataSource, snapshot: snapshot, animatingDifferences: true,
      numberOfVisibleItemsChanged: true)
  }

  // MARK: - Public

  @objc func setNewTabButtonOnTabStripIPHHighlighted(_ iphHighlighted: Bool) {
    newTabButton.IPHHighlighted = iphHighlighted
  }

  // MARK: - TabStripTabCellDelegate

  func closeButtonTapped(for cell: TabStripTabCell?) {
    guard let cell = cell,
      let indexPath = collectionView.indexPath(for: cell),
      let item = dataSource.itemIdentifier(for: indexPath)?.tabSwitcherItem
    else {
      return
    }
    mutator?.close(item)
  }

  // MARK: - UIScrollViewDelegate

  func scrollViewDidEndDragging(
    _ scrollView: UIScrollView,
    willDecelerate decelerate: Bool
  ) {
    layout.cellAnimatediOS16 = false
    UserMetricsUtils.recordAction("MobileTabStripScrollDidEnd")
  }

  // MARK: - Private

  /// Applies `snapshot` to `dataSource` and updates the collection view layout.
  private func applySnapshot(
    dataSource: UICollectionViewDiffableDataSource<Section, TabStripItemIdentifier>,
    snapshot: NSDiffableDataSourceSectionSnapshot<TabStripItemIdentifier>,
    animatingDifferences: Bool = false,
    numberOfVisibleItemsChanged: Bool = false
  ) {
    if #available(iOS 17.0, *) {
      if numberOfVisibleItemsChanged {
        layout.needsSizeUpdate = true
      }
    } else {
      /// On iOS 16, the layout fails to invalidate when it should.
      /// To fix this, we set `needsSizeUpdate` to `true` whenever a snapshot
      /// is applied.
      layout.needsSizeUpdate = true
    }

    dataSource.apply(snapshot, to: .tabs, animatingDifferences: animatingDifferences)
    layout.needsSizeUpdate = false

    ensureSelectedItemIsSelected()
    updateVisibleCellIdentifiers()
  }

  /// Creates and returns the data source for the collection view.
  private func createDataSource(
    tabCellRegistration: UICollectionView.CellRegistration<TabStripTabCell, TabStripItemIdentifier>,
    groupCellRegistration: UICollectionView.CellRegistration<
      TabStripGroupCell, TabStripItemIdentifier
    >
  ) -> UICollectionViewDiffableDataSource<Section, TabStripItemIdentifier> {
    let dataSource = UICollectionViewDiffableDataSource<Section, TabStripItemIdentifier>(
      collectionView: collectionView
    ) {
      (
        collectionView: UICollectionView, indexPath: IndexPath,
        itemIdentifier: TabStripItemIdentifier
      )
        -> UICollectionViewCell? in
      return Self.getCell(
        collectionView: collectionView, indexPath: indexPath, itemIdentifier: itemIdentifier,
        tabCellRegistration: tabCellRegistration, groupCellRegistration: groupCellRegistration)
    }

    var snapshot = dataSource.snapshot()
    snapshot.appendSections([.tabs])
    dataSource.apply(snapshot)

    return dataSource
  }

  /// Creates the registrations of tab cells used in the collection view.
  private func createTabCellRegistration()
    -> UICollectionView.CellRegistration<TabStripTabCell, TabStripItemIdentifier>
  {
    let tabCellRegistration = UICollectionView.CellRegistration<
      TabStripTabCell, TabStripItemIdentifier
    > {
      (cell, indexPath, itemIdentifier) in
      guard let item = itemIdentifier.tabSwitcherItem else { return }
      let itemData = self.itemData[itemIdentifier] as? TabStripItemData
      cell.title = item.title
      cell.setGroupStrokeColor(itemData?.groupStrokeColor)
      cell.isFirstTabInGroup = itemData?.isFirstTabInGroup == true
      cell.isLastTabInGroup = itemData?.isLastTabInGroup == true
      cell.loading = item.showsActivity
      cell.delegate = self
      cell.accessibilityIdentifier = self.tabTripTabCellAccessibilityIdentifier(
        index: indexPath.item)
      cell.item = item
      cell.tabIndex = indexPath.item + 1
      cell.numberOfTabs = self.numberOfTabs

      item.fetchFavicon { (item: TabSwitcherItem?, image: UIImage?) -> Void in
        if let item = item, item == cell.item {
          cell.setFaviconImage(image)
        }
      }
    }

    // UICollectionViewDropPlaceholder uses a TabStripTabCell and needs the class to be
    // registered.
    collectionView.register(
      TabStripTabCell.self,
      forCellWithReuseIdentifier: TabStripConstants.CollectionView.tabStripTabCellReuseIdentifier)

    return tabCellRegistration
  }

  /// Creates the registrations of group cells used in the collection view.
  private func createGroupCellRegistration()
    -> UICollectionView.CellRegistration<
      TabStripGroupCell, TabStripItemIdentifier
    >
  {
    return UICollectionView.CellRegistration<
      TabStripGroupCell, TabStripItemIdentifier
    > {
      (cell, indexPath, itemIdentifier) in
      guard let item = itemIdentifier.tabGroupItem else { return }
      let itemData = self.itemData[itemIdentifier] as? TabStripItemData
      cell.title = item.rawTitle
      cell.titleContainerBackgroundColor = item.groupColor
      cell.collapsed = item.collapsed
      cell.setGroupStrokeColor(itemData?.groupStrokeColor)
      cell.accessibilityIdentifier = self.tabTripGroupCellAccessibilityIdentifier(
        index: indexPath.item)
    }
  }

  /// Retuns the cell to be used in the collection view.
  static private func getCell(
    collectionView: UICollectionView, indexPath: IndexPath, itemIdentifier: TabStripItemIdentifier,
    tabCellRegistration: UICollectionView.CellRegistration<TabStripTabCell, TabStripItemIdentifier>,
    groupCellRegistration: UICollectionView.CellRegistration<
      TabStripGroupCell, TabStripItemIdentifier
    >
  ) -> UICollectionViewCell? {
    switch itemIdentifier.item {
    case .tab(_):
      return collectionView.dequeueConfiguredReusableCell(
        using: tabCellRegistration,
        for: indexPath,
        item: itemIdentifier)
    case .group(_):
      return collectionView.dequeueConfiguredReusableCell(
        using: groupCellRegistration,
        for: indexPath,
        item: itemIdentifier)
    }
  }

  /// Returns a UIMenu for the context menu to be displayed at `indexPath`.
  private func contextMenuForIndexPath(_ indexPath: IndexPath) -> UIMenu {
    let selectedItem = dataSource.itemIdentifier(for: indexPath)
    switch selectedItem?.item {
    case .tab(let tabSwitcherItem):
      return contextMenuForTabSwitcherItem(tabSwitcherItem, at: indexPath)
    case .group(let tabGroupItem):
      return contextMenuForTabGroupItem(tabGroupItem, at: indexPath)
    case nil:
      return UIMenu()
    }

  }

  /// Returns a UIMenu for the context menu to be displayed at `indexPath` for a tab item.
  private func contextMenuForTabSwitcherItem(
    _ tabSwitcherItem: TabSwitcherItem, at indexPath: IndexPath
  )
    -> UIMenu
  {
    let selectedItem = tabSwitcherItem
    let actionFactory = ActionFactory(scenario: kMenuScenarioHistogramTabStripEntry)
    var menuElements: [UIMenuElement?] = []
    weak var weakSelf = self

    /// Action to add tab to new group.
    if TabStripFeaturesUtils.isModernTabStripWithTabGroups() {
      let addToNewGroup = actionFactory?.actionToAddTabsToNewGroup(
        withTabsNumber: 1, inSubmenu: false
      ) {
        weakSelf?.mutator?.createNewGroup(with: tabSwitcherItem)
      }
      menuElements.append(addToNewGroup)
    }

    /// Action to share tab.
    let share = actionFactory?.actionToShare {
      let cell = weakSelf?.collectionView.cellForItem(at: indexPath)
      weakSelf?.delegate?.tabStrip(weakSelf, shareItem: selectedItem, originView: cell)
    }
    menuElements.append(share)

    /// Actions to close this tab or other tabs.
    var closeMenuElements: [UIMenuElement?] = []
    let close = actionFactory?.actionToCloseRegularTab {
      weakSelf?.mutator?.close(selectedItem)
    }
    closeMenuElements.append(close)
    let closeOthers = actionFactory?.actionToCloseAllOtherTabs {
      weakSelf?.mutator?.closeAllItemsExcept(selectedItem)
    }
    closeMenuElements.append(closeOthers)
    let closeMenu = UIMenu(options: .displayInline, children: closeMenuElements.compactMap { $0 })
    menuElements.append(closeMenu)

    return UIMenu(children: menuElements.compactMap { $0 })
  }

  /// Returns a UIMenu for the context menu to be displayed at `indexPath` for a group item.
  private func contextMenuForTabGroupItem(_ tabGroupItem: TabGroupItem, at indexPath: IndexPath)
    -> UIMenu
  {
    let actionFactory = ActionFactory(scenario: kMenuScenarioHistogramTabStripEntry)
    weak var weakSelf = self
    var menuElements: [UIMenuElement?] = []

    /// Add edit group menu.
    var editGroupMenuElements: [UIMenuElement?] = []
    let renameGroup = actionFactory?.actionToRenameTabGroup {
      weakSelf?.mutator?.renameGroup(tabGroupItem)
    }
    editGroupMenuElements.append(renameGroup)
    let addNewTabToGroup = actionFactory?.actionToAddNewTabInGroup {
      weakSelf?.mutator?.addNewTabInGroup(tabGroupItem)
    }
    editGroupMenuElements.append(addNewTabToGroup)
    let ungroupGroup = actionFactory?.actionToUngroupTabGroup {
      weakSelf?.mutator?.ungroupGroup(tabGroupItem)
    }
    editGroupMenuElements.append(ungroupGroup)
    let editGroupMenu = UIMenu(
      options: .displayInline, children: editGroupMenuElements.compactMap { $0 })
    menuElements.append(editGroupMenu)

    /// Close group menu.
    var closeGroupMenuElements: [UIMenuElement?] = []
    let closeGroup = actionFactory?.actionToDeleteTabGroup {
      weakSelf?.mutator?.deleteGroup(tabGroupItem)
    }
    closeGroupMenuElements.append(closeGroup)
    let closeGroupMenu = UIMenu(
      options: .displayInline, children: closeGroupMenuElements.compactMap { $0 })
    menuElements.append(closeGroupMenu)

    return UIMenu(children: menuElements.compactMap { $0 })
  }

  // Update visible cells identifier, following a reorg of cells.
  func updateVisibleCellIdentifiers() {
    for indexPath in collectionView.indexPathsForVisibleItems {
      switch collectionView.cellForItem(at: indexPath) {
      case let tabCell as TabStripTabCell:
        tabCell.accessibilityIdentifier = tabTripTabCellAccessibilityIdentifier(
          index: indexPath.item)
        tabCell.tabIndex = indexPath.item + 1
        tabCell.numberOfTabs = numberOfTabs
      case let groupCell as TabStripGroupCell:
        groupCell.accessibilityIdentifier = tabTripGroupCellAccessibilityIdentifier(
          index: indexPath.item)
      default:
        continue
      }
    }
  }

  // Returns the accessibility identifier to set on a TabStripTabCell when
  // positioned at the given index.
  func tabTripTabCellAccessibilityIdentifier(index: Int) -> String {
    return "\(TabStripConstants.CollectionView.tabStripTabCellPrefixIdentifier)\(index)"
  }

  // Returns the accessibility identifier to set on a TabStripGroupCell when
  // positioned at the given index.
  func tabTripGroupCellAccessibilityIdentifier(index: Int) -> String {
    return "\(TabStripConstants.CollectionView.tabStripGroupCellPrefixIdentifier)\(index)"
  }

  /// Scrolls the collection view to the given horizontal `offset`.
  func scrollToContentOffset(_ offset: CGFloat) {
    // TODO(crbug.com/325415449): Update this when #unavailable is rocognized by
    // the formatter.
    if #available(iOS 17.0, *) {
    } else {
      if offset != targetedScrollOffsetiOS16 { return }
    }
    self.collectionView.setContentOffset(
      CGPoint(x: offset, y: 0),
      animated: !UIAccessibility.isReduceMotionEnabled)
  }

  /// Ensures `collectionView.indexPathsForSelectedItems` is consistent with
  /// `self.selectedItem`.
  func ensureSelectedItemIsSelected() {
    let expectedIndexPathForSelectedItem =
      TabStripItemIdentifier(selectedItem).flatMap { dataSource.indexPath(for: $0) }
    let observedIndexPathForSelectedItem = collectionView.indexPathsForSelectedItems?.first

    // If the observed selected indexPath doesn't match the expected selected
    // indexPath, update the observed selected item.
    if expectedIndexPathForSelectedItem != observedIndexPathForSelectedItem {
      collectionView.selectItem(
        at: expectedIndexPathForSelectedItem, animated: false, scrollPosition: [])
    }

    /// Invalidate the layout to correctly recalculate the frame of the `selected` cell.
    layout.selectedItem = selectedItem
    layout.invalidateLayout()
  }

  /// Inserts items by applying `snapshot`, updates `numberOfTabs` and optionally scrolls to the end of the collection view if `insertedLast` is true.
  /// To use this method, create a snapshot from `dataSource`, insert items in the snapshot and pass it as the first argument `snapshot`.
  func insertItemsUsingSnapshot(
    _ snapshot: NSDiffableDataSourceSectionSnapshot<TabStripItemIdentifier>, insertedLast: Bool
  ) {
    numberOfTabs = snapshot.items.lazy.filter { $0.itemType == .tab }.count

    applySnapshot(
      dataSource: dataSource, snapshot: snapshot,
      animatingDifferences: !UIAccessibility.isReduceMotionEnabled,
      numberOfVisibleItemsChanged: true)

    if insertedLast {
      // Don't scroll to the end of the collection view in RTL.
      let isRTL: Bool = collectionView.effectiveUserInterfaceLayoutDirection == .rightToLeft
      if isRTL { return }

      let offset = collectionView.contentSize.width - collectionView.frame.width
      if offset > 0 {
        if #available(iOS 17.0, *) {
          scrollToContentOffset(offset)
        } else {
          // On iOS 16, when the scroll animation and the insert animation
          // occur simultaneously, the resulting animation lacks of
          // smoothness.
          weak var weakSelf = self
          targetedScrollOffsetiOS16 = offset
          DispatchQueue.main.asyncAfter(
            deadline: .now() + TabStripConstants.CollectionView.scrollDelayAfterInsert
          ) {
            weakSelf?.scrollToContentOffset(offset)
          }
        }
      } else {
        layout.cellAnimatediOS16 = false
      }
    }
  }

  // MARK: - TabStripNewTabButtonDelegate

  @objc func newTabButtonTapped() {
    UserMetricsUtils.recordAction("MobileTabSwitched")
    UserMetricsUtils.recordAction("MobileTabStripNewTab")

    mutator?.addNewItem()
  }

}

// MARK: - UICollectionViewDelegateFlowLayout

extension TabStripViewController: UICollectionViewDelegateFlowLayout {

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if #available(iOS 16, *) {
    } else {
      self.collectionView(collectionView, performPrimaryActionForItemAt: indexPath)
    }
  }

  func collectionView(_ collectionView: UICollectionView, shouldSelectItemAt indexPath: IndexPath)
    -> Bool
  {
    switch dataSource.itemIdentifier(for: indexPath)?.item {
    case .tab(_):
      // Only tabs are selectable since being selected is equivalent to having
      // the associated WebState being active.
      return true
    default:
      return false
    }
  }

  func collectionView(
    _ collectionView: UICollectionView, performPrimaryActionForItemAt indexPath: IndexPath
  ) {
    guard let itemIdentifier = dataSource.itemIdentifier(for: indexPath) else { return }
    switch itemIdentifier.item {
    case .tab(let tabSwitcherItem):
      mutator?.activate(tabSwitcherItem)
    case .group(let tabGroupItem):
      if tabGroupItem.collapsed {
        mutator?.expandGroup(tabGroupItem)
      } else {
        mutator?.collapseGroup(tabGroupItem)
      }
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    highlightPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let tabStripCell = collectionView.cellForItem(at: indexPath) as? TabStripCell else {
      return nil
    }
    return UITargetedPreview(view: tabStripCell, parameters: tabStripCell.dragPreviewParameters)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    return self.collectionView(
      collectionView, contextMenuConfigurationForItemsAt: [indexPath], point: point)
  }

  func collectionView(
    _ collectionView: UICollectionView, contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    if indexPaths.count != 1 {
      return nil
    }

    weak var weakSelf = self
    return UIContextMenuConfiguration(actionProvider: { suggestedActions in
      return weakSelf?.contextMenuForIndexPath(indexPaths[0])
    })
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    switch dataSource.itemIdentifier(for: indexPath)?.item {
    case .tab(let tabSwitcherItem):
      return layout.calculateCellSizeForTabSwitcherItem(tabSwitcherItem)
    case .group(let tabGroupItem):
      return layout.calculateCellSizeForTabGroupItem(tabGroupItem)
    case nil:
      return CGSizeZero
    }
  }

}

extension TabStripViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  // MARK: - UICollectionViewDragDelegate

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionIsRestrictedToDraggingApplication session: UIDragSession
  ) -> Bool {
    // Needed to avoid triggering new Chrome window opening when dragging
    // an item close to an edge of the collection view.
    // Dragged item can still be dropped in another Chrome window.
    return true
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionWillBegin session: UIDragSession
  ) {
    dragEndAtNewIndex = false
    HistogramUtils.recordHistogram(
      kUmaTabStripViewDragDropTabsEvent, withSample: DragDropTabs.dragBegin.rawValue,
      maxValue: DragDropTabs.maxValue.rawValue)
    dragDropHandler?.dragWillBegin(for: draggedItem)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionDidEnd session: UIDragSession
  ) {
    var dragEvent =
      dragEndAtNewIndex
      ? DragDropTabs.dragEndAtNewIndex
      : DragDropTabs.dragEndAtSameIndex

    // If a drop animation is in progress and the drag didn't end at a new index,
    // that means the item has been dropped outside of its collection view.
    if dropAnimationInProgress && !dragEndAtNewIndex {
      dragEvent = DragDropTabs.dragEndInOtherCollection
    }

    HistogramUtils.recordHistogram(
      kUmaTabStripViewDragDropTabsEvent, withSample: dragEvent.rawValue,
      maxValue: DragDropTabs.maxValue.rawValue)

    dragDropHandler?.dragSessionDidEnd()
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    guard let cell = collectionView.cellForItem(at: indexPath) as? TabStripTabCell else {
      return nil
    }
    return cell.dragPreviewParameters
  }

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
    guard let itemIdentifier = dataSource.itemIdentifier(for: indexPath),
      let item = itemIdentifier.tabSwitcherItem,
      let dragItem = dragDropHandler?.dragItem(for: item)
    else {
      return []
    }
    draggedItem = item
    return [dragItem]
  }

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForAddingTo session: UIDragSession,
    at indexPath: IndexPath,
    point: CGPoint
  ) -> [UIDragItem] {
    // Prevent more items from getting added to the drag session.
    return []
  }

  // MARK: - UICollectionViewDropDelegate

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {
    guard let dropOperation: UIDropOperation = dragDropHandler?.dropOperation(for: session) else {
      return UICollectionViewDropProposal(operation: .cancel)
    }
    /// Use `insertIntoDestinationIndexPath` if the dragged item is not from the same
    /// collection view. This prevents having unwanted empty space in the collection view.
    return UICollectionViewDropProposal(
      operation: dropOperation,
      intent: dropOperation == .move
        ? .insertAtDestinationIndexPath : .insertIntoDestinationIndexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {
    for item in coordinator.items {
      // Append to the end of the collection, unless drop index is specified.
      // The sourceIndexPath is nil if the drop item is not from the same
      // collection view. Set the destinationIndex to reflect the addition of an
      // item.
      let tabsCount = collectionView.numberOfItems(inSection: 0)
      var destinationIndex = item.sourceIndexPath != nil ? (tabsCount - 1) : tabsCount
      if let destinationIndexPath = coordinator.destinationIndexPath {
        destinationIndex = destinationIndexPath.item
      }
      let dropIndexPah: IndexPath = IndexPath(item: destinationIndex, section: 0)
      dragEndAtNewIndex = true

      // Drop synchronously if local object is available.
      if item.dragItem.localObject != nil {
        weak var weakSelf = self
        coordinator.drop(item.dragItem, toItemAt: dropIndexPah).addCompletion {
          _ in
          weakSelf?.dropAnimationInProgress = false
        }
        // The sourceIndexPath is non-nil if the drop item is from this same
        // collection view.
        self.dragDropHandler?.drop(
          item.dragItem, to: UInt(destinationIndex),
          fromSameCollection: (item.sourceIndexPath != nil))
      } else {
        // Drop asynchronously if local object is not available.
        let placeholder: UICollectionViewDropPlaceholder = UICollectionViewDropPlaceholder(
          insertionIndexPath: dropIndexPah,
          reuseIdentifier: TabStripConstants.CollectionView.tabStripTabCellReuseIdentifier)
        placeholder.previewParametersProvider = {
          (placeholderCell: UICollectionViewCell) -> UIDragPreviewParameters? in
          guard let tabStripCell = placeholderCell as? TabStripTabCell else {
            return nil
          }
          return tabStripCell.dragPreviewParameters
        }

        let context: UICollectionViewDropPlaceholderContext = coordinator.drop(
          item.dragItem, to: placeholder)
        self.dragDropHandler?.dropItem(
          from: item.dragItem.itemProvider, to: UInt(destinationIndex), placeholderContext: context)
      }
    }
  }

}
