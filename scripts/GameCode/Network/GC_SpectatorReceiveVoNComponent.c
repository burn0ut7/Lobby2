//! Keep this AFTER the transmitter in Character_Spectator.et: native voice
//! reception resolves the last registered VoN component on the controlled body.
//! Capture is explicitly owned by GC_SpectatorTransmitVoNComponent.
class GC_SpectatorReceiveVoNComponentClass : SCR_VoNComponentClass
{
}

class GC_SpectatorReceiveVoNComponent : SCR_VoNComponent
{
}
