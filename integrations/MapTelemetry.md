# Map telemetry

The recorder emits change-only `map-state` replacement snapshots per faction (empty key = public). Canonical map-shape geometry/style and briefing-marker visibility remain owned by existing Lobby2 systems. Shared custom/military markers are server-native; client-local markers and admin boundary hints are excluded. The optional MapDrawing adapter supplies packed drawings; decoded per installed addon rules, not live-tested here.

Each snapshot contains faction, availability flags, shapes (flat world X/Z points, packed ARGB line/fill, width, inverted, closed, worldWidth), and markers (world position, label, icon resource/name, packed ARGB color, rotation degrees, size/minSize/worldScale, nativeType/config/flags, militaryIdentity/dimension). Military metadata is retained even where a viewer cannot reproduce composite artwork. New military fields are optional in the shared validator for initial map capture compatibility.

One faction is checked each frame, with one second between cycles. Unchanged state is omitted. Short-lived changes between samples can be missed. More than64factions,384shapes,512markers or64KiB snapshot stops capture explicitly. No gameplay mutation or player web requests.

Live Workbench1.8.0.13 proof: recordings01a19e1e-6933-897d-8400-00000072245f and01a19e23-5486-8a30-8400-00000074f037. Faction separation, concave/inverted fill, mission/shared marker removal and AO removal observed. First recording has9530records, zero drops, complete website intake/read API verification. Optional test helper lives only in McpTestAddon and requires existing admin authorization. All39synthetic contract cases passed.

Native source references (catalogue721cd7b05eb54d3ca3591d9097af381d): SCR_MapMarkerManagerComponent.c48–69 copied collections; SCR_MapMarkerEntryPlaced.c43–59 icon/color getters; SCR_MapMarkerEntryMilitary.c132–145 military identity/dimension/flags/color. MapDrawing TILW_Drawing.c52–75 decompression and Helpers/TILW_DrawingHelper.c34–49 color/visibility.
