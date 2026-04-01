# SUnrealMcp Tool Inventory

This inventory records the current lifecycle classification of the non-`core` capability surface under the `core / stable / candidate / temporary` model defined in the skill docs.

Scope for this pass:

- `McpServer/src/tools/*.ts`
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Stable/*`
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Candidate/*`
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Temporary/*`
- `UnrealPlugin/SUnrealMcp/Source/SUnrealMcp/Private/Commands/Shared/*`

Notes:

- `core` transport and task infrastructure commands are intentionally excluded from the main table below because this pass focuses on the lifecycle-managed non-`core` surface.
- Classification is about lifecycle maturity, not whether the current implementation works.
- `action: keep` means "keep in the current lifecycle lane for now", not "promote automatically."
- Domain families such as `Blueprint`, `Editor`, `Node`, `UMG`, or `Project` are current organizational groups, not fixed taxonomy. They may be regrouped later if the command surface evolves.

## Current Inventory

- tool: `create_blueprint`
  command: `create_blueprint`
  family: `Blueprint`
  classification: `stable`
  action: `keep`
  reason: Foundational asset-creation primitive with broad reuse and clean capability-level naming.

- tool: `add_component_to_blueprint`
  command: `add_component_to_blueprint`
  family: `Blueprint`
  classification: `stable`
  action: `keep`
  reason: Core composition primitive for Blueprint authoring with strong reuse across many tasks.

- tool: `compile_blueprint`
  command: `compile_blueprint`
  family: `Blueprint`
  classification: `stable`
  action: `keep`
  reason: Universal validation/build step for Blueprint workflows and part of the long-term editing loop.

- tool: `set_blueprint_property`
  command: `set_blueprint_property`
  family: `Blueprint`
  classification: `stable`
  action: `keep`
  reason: General-purpose property mutation on the Blueprint default object, not tied to one project pattern.

- tool: `set_component_property`
  command: `set_component_property`
  family: `Blueprint`
  classification: `stable`
  action: `keep`
  reason: General component mutation primitive that covers many authoring cases without overfitting to one asset type.

- tool: `set_static_mesh_properties`
  command: `set_static_mesh_properties`
  family: `Blueprint`
  classification: `candidate`
  action: `keep`
  reason: Useful wrapper for a common case, but narrower than the stable component/property primitives and may later merge into a more general asset/component configuration surface.

- tool: `set_physics_properties`
  command: `set_physics_properties`
  family: `Blueprint`
  classification: `candidate`
  action: `keep`
  reason: Common enough to keep watching, but still a domain-specific convenience layer over broader component/property editing.

- tool: `get_actors_in_level`
  command: `get_actors_in_level`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: Base discovery primitive for nearly every level-editing workflow.

- tool: `find_actors_by_label`
  command: `find_actors_by_label`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: General lookup primitive that complements actor discovery and stays capability-shaped.

- tool: `spawn_actor`
  command: `spawn_actor`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: Core scene-authoring primitive with clear long-term value.

- tool: `spawn_blueprint_actor`
  command: `spawn_blueprint_actor`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: Distinct and broadly useful scene-placement primitive for Blueprint-authored content.

- tool: `delete_actor`
  command: `delete_actor`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: Fundamental inverse operation for scene editing and cleanup.

- tool: `set_actor_transform`
  command: `set_actor_transform`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: General transform mutation primitive with durable cross-task reuse.

- tool: `get_actor_properties`
  command: `get_actor_properties`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: Broad inspection primitive needed before safe mutation in many workflows.

- tool: `set_actor_property`
  command: `set_actor_property`
  family: `Editor`
  classification: `stable`
  action: `keep`
  reason: General actor mutation primitive with long-term applicability.

- tool: `add_blueprint_event_node`
  command: `add_blueprint_event_node`
  family: `Node`
  classification: `stable`
  action: `keep`
  reason: Core graph-authoring primitive rather than a project-specific wrapper.

- tool: `add_blueprint_function_node`
  command: `add_blueprint_function_node`
  family: `Node`
  classification: `stable`
  action: `keep`
  reason: General node insertion capability at the right abstraction level for long-term graph editing.

- tool: `add_blueprint_variable`
  command: `add_blueprint_variable`
  family: `Node`
  classification: `stable`
  action: `keep`
  reason: Canonical graph/data-authoring primitive with broad reuse.

- tool: `connect_blueprint_nodes`
  command: `connect_blueprint_nodes`
  family: `Node`
  classification: `stable`
  action: `keep`
  reason: Foundational graph wiring primitive and one of the basic building blocks of Blueprint editing.

- tool: `find_blueprint_nodes`
  command: `find_blueprint_nodes`
  family: `Node`
  classification: `stable`
  action: `keep`
  reason: Broad graph inspection primitive that supports both editing and analysis workflows.

- tool: `inspect_blueprint_graph`
  command: `inspect_blueprint_graph`
  family: `Node`
  classification: `candidate`
  action: `keep`
  reason: Strong analytical value and promising abstraction, but still newer and should prove repeat use before joining the stable surface.

- tool: `add_blueprint_input_action_node`
  command: `add_blueprint_input_action_node`
  family: `Node`
  classification: `candidate`
  action: `keep`
  reason: Useful workflow shortcut, but still narrower than the more general event/function node primitives.

- tool: `add_blueprint_get_self_component_reference`
  command: `add_blueprint_get_self_component_reference`
  family: `Node`
  classification: `candidate`
  action: `keep`
  reason: Helpful convenience node insertion, but specialized enough that it should stay under observation.

- tool: `add_blueprint_self_reference`
  command: `add_blueprint_self_reference`
  family: `Node`
  classification: `candidate`
  action: `keep`
  reason: Narrow helper around a common node pattern; likely useful, but not yet strong enough as a long-term standalone primitive.

- tool: `create_umg_widget_blueprint`
  command: `create_umg_widget_blueprint`
  family: `UMG`
  classification: `stable`
  action: `keep`
  reason: Foundational asset-creation primitive for widget workflows.

- tool: `add_text_block_to_widget`
  command: `add_text_block_to_widget`
  family: `UMG`
  classification: `candidate`
  action: `keep`
  reason: Useful and concrete, but still a widget-type-specific helper that may later converge into a more generic widget insertion surface.

- tool: `add_button_to_widget`
  command: `add_button_to_widget`
  family: `UMG`
  classification: `candidate`
  action: `keep`
  reason: Same pattern as text block insertion: valuable, but still a narrow typed shortcut rather than a clearly final abstraction.

- tool: `bind_widget_event`
  command: `bind_widget_event`
  family: `UMG`
  classification: `candidate`
  action: `keep`
  reason: Important workflow capability, but should prove stable naming/behavior over more tasks before promotion.

- tool: `set_text_block_binding`
  command: `set_text_block_binding`
  family: `UMG`
  classification: `candidate`
  action: `keep`
  reason: Binding support is useful, but this specific text-block-focused API is still fairly narrow.

- tool: `add_widget_to_viewport`
  command: `add_widget_to_viewport`
  family: `UMG`
  classification: `candidate`
  action: `keep`
  reason: Likely useful beyond one task, but runtime viewport placement is a distinct surface that should mature before becoming stable.

- tool: `add_enhanced_input_mapping`
  command: `add_enhanced_input_mapping`
  family: `Project`
  classification: `candidate`
  action: `keep`
  reason: Clearly useful project-editing capability, but currently narrow and should prove repeat demand before being treated as stable core surface.

- tool: `(unexposed) start_mock_task`
  command: `start_mock_task`
  family: `System`
  classification: `temporary`
  action: `deprecate`
  reason: Test/mock command with no matching public MCP tool; it should stay out of the long-term extension surface unless an explicit test harness strategy is documented.

## Family Summary

- `Blueprint`
  Stable primitives are in good shape.
  `set_static_mesh_properties` and `set_physics_properties` should stay watchlisted as convenience wrappers around broader mutation capabilities.

- `Editor`
  Current surface reads like a coherent stable editing core and is the strongest candidate for the long-term extension baseline.

- `Node`
  The general graph-editing primitives look stable.
  The specialized self/input/component-reference helpers should remain `candidate` until repeated workflows prove they deserve standalone permanence.

- `UMG`
  This family is useful but still reads as partly "typed convenience commands."
  It should stay under active convergence review so it does not fragment into one-command-per-widget-type.

- `Project`
  `add_enhanced_input_mapping` is valuable but currently too narrow to call stable.

- `System`
  `start_mock_task` should not be treated as part of the public extension surface.

## Immediate Reorganization Guidance

- Keep current `stable` families as the baseline public extension surface:
  `Blueprint`: `create_blueprint`, `add_component_to_blueprint`, `compile_blueprint`, `set_blueprint_property`, `set_component_property`
  `Editor`: all current public editor tools
  `Node`: `add_blueprint_event_node`, `add_blueprint_function_node`, `add_blueprint_variable`, `connect_blueprint_nodes`, `find_blueprint_nodes`
  `UMG`: `create_umg_widget_blueprint`

- Treat these as `candidate` and require explicit end-of-task lifecycle review when touched:
  `Blueprint`: `set_static_mesh_properties`, `set_physics_properties`
  `Node`: `inspect_blueprint_graph`, `add_blueprint_input_action_node`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`
  `UMG`: `add_text_block_to_widget`, `add_button_to_widget`, `bind_widget_event`, `set_text_block_binding`, `add_widget_to_viewport`
  `Project`: `add_enhanced_input_mapping`

- Treat `start_mock_task` as `temporary` and non-public by default.
