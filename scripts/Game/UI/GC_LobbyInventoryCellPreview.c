class GC_LobbyInventoryCellPreview
{
	protected ItemPreviewWidget m_wItemPreview;
	protected ImageWidget m_wFallbackIcon;

	void GC_LobbyInventoryCellPreview(Widget cellRoot, string previewWidgetName)
	{
		if (!cellRoot)
			return;

		m_wItemPreview = ItemPreviewWidget.Cast(cellRoot.FindAnyWidget(previewWidgetName));
		m_wFallbackIcon = ImageWidget.Cast(cellRoot.FindAnyWidget("BaseIcon"));
	}

	bool IsValid()
	{
		return m_wItemPreview != null;
	}

	void SetItemEntity(ItemPreviewManagerEntity previewManager, IEntity itemEntity)
	{
		if (!m_wItemPreview)
			return;

		bool hasItem = itemEntity != null && previewManager != null;
		if (hasItem)
			previewManager.SetPreviewItem(m_wItemPreview, itemEntity, null, true);

		m_wItemPreview.SetVisible(hasItem);
		if (m_wFallbackIcon)
			m_wFallbackIcon.SetVisible(!hasItem);
	}
}
