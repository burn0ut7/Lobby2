# AO polygon authoring verification

2026-10-25, Workbench 1.8.0.13. Production script reload passed (PID 34708, operation 0873bf05aa084724b624deb6d8b55605); optional test-addon reload passed (PID 12084, operation 03ec8d076d294661a6106bd537a3de68). Native MCP observations are condensed in mcp-evidence.json.

- Native Tools > AO Polygon activated the production panel with Sides, Radius, Rotation and Generate AO polygon.
- Generated five sides at radius 100, rotation 0 on a newly placed empty AO prefab. Point count was five.
- Edited point 0 to `5 2 110` and entity position to `175 1 160`; saved and reopened. Both values persisted with exactly five points.
- Test-addon buttons call the inherited production generator with explicit inputs. Triangle 3/25/90 yielded `(25,0,0)`, `(-12.5,0,-21.651)`, `(-12.5,0,21.651)`.
- One native Undo restored the previous five points including `5 2 110`; Redo restored all three triangle points.
- Regenerated 7/40/360: seven equally spaced vertices, first `(0,0,40)`. Saved/reopened and all seven matched. Serialized coordinates round to three decimal places.
- Two sides were rejected: seven positions unchanged, history remained at index 2. Session log logs_2026-10-25_00-08-12/console.log lines 343–345 records the triangle, heptagon and rejection; line 374 records default pentagon generation.
- A first implementation using native ShapeEntity.SetPoints left history busy and merged inherited prefab points on reopening (five became nine). Replaced it with the shipped SCR_ShapeAreaTool editor-command pattern and made geometry instance-owned. Final Undo/Redo and save/reopen checks above passed.

Installed reference: pak revision 4942ab6194e34c74bd5972e7761785e2, scripts/WorkbenchGame/WorldEditor/SCR_ShapeAreaTool.c:285–323; Core/generated/WorkbenchAPI/WorldEditorAPI.c and Core/generated/Containers/BaseContainer.c. The custom tool constrains the native shape workflow to AO entities and exposes an explicit rotation angle.

Scope: Workbench authoring tests only; gameplay enforcement was not changed or rerun. Custom inherited-shape regeneration is rejected before mutation; native inherited-entry removal is unsupported. The base prefab's prior inherited square is removed; instances that relied on it must author geometry. No production authored AO instances were found in this checkout before the change. The test world is an editor fixture, not a playable AO scenario. Scripted-tool property-cell editing is not exposed by the current generic UI tools; nondefault values were supplied by the optional test tool, without duplicating generation logic.

Independent fresh reviewer approved the code, evidence, native pattern and documented migration consequence. Pre-delivery script readability check passed: GC names, Workbench module, protected implementation, tabs, Allman blocks and native command ownership. Normal Lobby2 project restored after testing, with only core, ArmaReforger and Lobby2 loaded; optional test code unloaded.
