# Custom setup extension

Small addition to the existing compact-controls reference: replace +5 minutes with a row-local editable time and Add time button. Accept whole seconds (30) or minutes:seconds (2:00, 11:00); default5:00. Preserve input through status polling. Keep End setup above; no duration controls for AO rules, which have no setup deadline. Existing server limit remains1-3600 seconds per addition. Label the format; invalid entries report a useful message without mutation. Keep existing dark native edit fields/buttons and row layout.

Verify specific non-default additions, multiple zones isolation, malformed/zero/excessive input, ended target, server-side range and unauthenticated rejection, central logs, and rendered row layout through MCP. Reuse prior compact-controls screenshots as baseline.
