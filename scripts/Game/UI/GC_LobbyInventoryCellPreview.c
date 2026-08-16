class GC_LobbyInventoryCellPreview
{
	protected ItemPreviewWidget m_wItemPreview;
	protected ImageWidget m_wFallbackIcon;
	protected float m_fCameraZoomIncrement;
	protected ref array<IEntity> m_aZoomedItems = {};

	void GC_LobbyInventoryCellPreview(Widget cellRoot, string previewWidgetName, float cameraZoomIncrement = 0)
	{
		if (!cellRoot)
			return;

		m_fCameraZoomIncrement = cameraZoomIncrement;
		m_wItemPreview = ItemPreviewWidget.Cast(cellRoot.FindAnyWidget(previewWidgetName));
		m_wFallbackIcon = ImageWidget.Cast(cellRoot.FindAnyWidget("BaseIcon"));

		RenderTargetWidget renderTarget = m_wItemPreview;
		if (renderTarget)
		{
			renderTarget.SetClearColor(false, 0);
			renderTarget.SetBlendMode(RenderTargetWidgetBlendMode.ALPHA_BLEND);
		}
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
		{
			PreviewRenderAttributes previewAttributes = GetPreviewAttributes(itemEntity);
			if (previewAttributes && m_fCameraZoomIncrement != 0 && m_aZoomedItems.Find(itemEntity) == -1)
			{
				previewAttributes.ZoomCamera(m_fCameraZoomIncrement, 20, 20);
				m_aZoomedItems.Insert(itemEntity);
			}

			previewManager.SetPreviewItem(m_wItemPreview, itemEntity, previewAttributes, true);
		}

		m_wItemPreview.SetVisible(hasItem);
		if (m_wFallbackIcon)
			m_wFallbackIcon.SetVisible(!hasItem);
	}

	protected PreviewRenderAttributes GetPreviewAttributes(IEntity itemEntity)
	{
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(itemEntity.FindComponent(InventoryItemComponent));
		if (!inventoryItem)
			return null;

		return PreviewRenderAttributes.Cast(inventoryItem.FindAttribute(PreviewRenderAttributes));
	}
}
