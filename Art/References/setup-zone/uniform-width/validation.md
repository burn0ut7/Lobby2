# Validation

2026-10-25, embedded Workbench Play in the optional McpTestAddon, Arland. Native script reload completed (7e541b50734b4fa4a3a1b681b122e376). Entered GAME through production lobby controls as an authenticated local admin.

- Default setup, setup-return and AO-return cards all measured x=1159.18 and width=294.637 at viewport scaling 1.3037 (226 design units). Receipt r33afdfe0860d4750a81b112716aec409 and default-stack.png. Heights remain 32/68 design units; gaps remain six.
- Longer custom setup/AO labels and a simultaneous announcement fitted inside the same width; see long-labels.png and receipt r709bfcedded04ebe8f83452d13b49bb4. Native desired/minimum font sizes reduce long labels; very long text should still be avoided for readability.
- admin-controls.png shows Stop all boundaries in the Boundaries tab. Invoked the actual button handler (ra3f6752e8a794ab6a76a8b4c18b0cb27); resulting catalog r06eb3479a1454b6ea916e4c751c26593 confirms both AOs disabled, both setups ended and announcement cleared.
- Removed optional Admin name attributes and assignments. Native entity names and generated AO/Setup numbers remain visible in the catalog. No serialized m_sAdminName values found in prefabs/tests.
- Installed TextWidget declarations and SCR_InputButtonComponent usage establish native desired/minimum font fitting. Source readability and git diff whitespace checks completed.

Scope: embedded host at the current viewport, not dedicated/remote multiplayer or a resolution matrix. No network/gameplay rules changed. Logs include existing world/plugin/resource/loadout diagnostics plus AOFixture0 initialization before configuration; no new HUD exception observed. Full receipts and error snapshot are adjacent JSON files.

Independent fresh review approved the change with no actionable defects; retained the viewport/readability and existing-log limitations above.

