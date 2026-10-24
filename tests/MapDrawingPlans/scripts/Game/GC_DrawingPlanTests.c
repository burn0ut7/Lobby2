modded class TILW_DrawingSyncComponent
{
	//------------------------------------------------------------------------------------------------
	string GC_TestDrawing(string action)
	{
		if (!Replication.IsServer())
			return "server-only-fixture";
		if (action == "start-local")
			return TILW_DrawingSystem.GetInstance().GC_TestStartDrawing();
		if (action == "add")
		{
			array<int> points = {1000000000, 1100000000, 1200000000};
			int properties = (63 << 10) | (3 << 5) | (1 << 7) | (4 << 1);
			BroadcastDrawing(GetGame().GetPlayerController().GetPlayerId() << 16, properties, points);
		}
		array<ref array<int>> drawings = {};
		GC_CapturePlan(drawings);
		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("drawings", drawings);
		return context.SaveToString();
	}
}

modded class TILW_DrawingSystem
{
	//------------------------------------------------------------------------------------------------
	string GC_TestStartDrawing()
	{
		OnStartDrawing(1, EActionTrigger.DOWN);
		bool preview = m_PreviewDrawing != null;
		OnStopDrawing(0, EActionTrigger.UP);
		return "preview=" + preview.ToString();
	}
}
