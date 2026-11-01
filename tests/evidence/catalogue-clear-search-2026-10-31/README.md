# Conditional search clear button

Workbench 1.8.0.13, PID44368, Empty_Lobby.ent. Reload e5a8582be5754737a8af9808bda273e9 passed without SCRIPT compilation errors (existing fixture resource/plugin diagnostics unchanged).

Existing UI helpers exercised the real button component: initial empty field has no X; setting `helmet` shows the right-aligned X; `GC_TestCatalogueButton("ClearSearch")` calls its production click handler and returns empty query, offset=0, total=813. It also resets the pending debounce text, hides the button immediately and focuses the input. Clearing preserves category/slot filters. Preview scheduling and inventory input remain unchanged.

MCP captures: empty.png, populated.png, cleared.png. Handler activation was tested; physical mouse delivery is not simulated by these helpers.
