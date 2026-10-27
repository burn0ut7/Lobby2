modded class SCR_CharacterCameraHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnAfterCameraUpdate(float pDt, bool pIsKeyframe, inout vector transformMS[4], inout vector transformWS[4])
	{
		super.OnAfterCameraUpdate(pDt, pIsKeyframe, transformMS, transformWS);
		if (!GC_LobbyGameMode.GetGameMode())
			return;

		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.GC_PublishNetworkCameraTransform(GetOwner(), transformMS, transformWS);
	}
}
