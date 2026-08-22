import unreal
unreal.EditorLoadingAndSavingUtils.load_map("/Game/00_ProjectNA/02_Level/Level_NAMainGame")
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in sub.get_all_level_actors():
    if "PlasmaCutter" not in a.get_name():
        continue
    unreal.log_warning("=== ACTOR %s class=%s root=%s" % (a.get_name(), a.get_class().get_name(), a.root_component.get_name() if a.root_component else None))
    for c in a.get_components_by_class(unreal.SceneComponent):
        p = c.get_attach_parent()
        unreal.log_warning("   %-34s %-26s parent=%-22s socket=%s flags_inst=%s" % (
            c.get_name(), c.get_class().get_name(), p.get_name() if p else None, c.get_attach_socket_name(), ""))
