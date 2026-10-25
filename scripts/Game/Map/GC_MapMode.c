//! The stock map caches components by mode, not by configuration resource.
//! Keep each Lobby2 configuration separate from the normal and GM maps.
modded enum EMapEntityMode
{
	GC_BRIEFING = 1787356811,
	GC_PREVIEW = 1787356812,
	GC_SPECTATOR = 1787356813
}
