function onSceneStart_scene_zone_a()
	setCorridorInvisible()
end

function onScenePartialStart_scene_zone_a()
	--onSceneStart_scene_zone_a();
	movePlayerToRefPos("zone_a_in_suelo001", i_ref_pos);
	--execScriptDelayed("toDoor(toEntity(getEntityByName(\"zone_a_in_marco_puerta001\")):getCompByName(\"door\")):open();", 0.5);
end

function onScenePartialEnd_scene_zone_a()
	i_ref_pos = getPlayerLocalCoordinatesInReferenceTo("zone_a_outsuelo002");
end

function onTriggerEnter_ZON_trigger_opendoor_zonea_player()
	execScriptDelayed("toDoor(toEntity(getEntityByName(\"zone_a_in_marco_puerta001\")):getCompByName(\"door\")):open()", 0.5);
	getEntityByName("ZON_trigger_opendoor_zonea"):destroy();
end

function transition_zone_a_to_coliseum(button_handle)
	execScriptDelayed("disableButton(" .. button_handle .. ", false)", 1);
	makeVisibleByTag("corridor", true);
	toDoor(toEntity(getEntityByName("zone_a_outmarco_puerta002")):getCompByName("door")):open();
end

function enable_button_exit(button_handle)
	execScriptDelayed("toButton(toEntity(getEntityByName(\"Button End Scene\")):getCompByName(\"button\")):setCanBePressed(true)",2.25);
	if(cinematicsEnabled and not cinematic_tower_activatedExecuted) then
		execScriptDelayed("cinematic_tower_activated()",0.5);
	end
	zone_a_door_activated = true;
	execScriptDelayed("disableButton(" .. button_handle .. ", false)", 1);
end

function onTriggerEnter_zonea_trigger_smartass_player()
	
	if(not zone_a_door_activated) then
		subClear();
		activateSubtitles(22);
		execScriptDelayed("deactivateSubtitles();", 8);
		zone_a_door_activated = true;
	end
	temp = getEntityByName("zonea_trigger_smartass");
	temp:destroy();
end

function onTriggerEnter_ZON_Trigger_Enter_ZoneA_player()
	zonea_a_door = toDoor(toEntity(getEntityByName("zone_a_in_marco_puerta001")):getCompByName("door"));
	zonea_a_door:setClosedScript("setCorridorInvisible()");
	zonea_a_door:close();
	if(cinematicsEnabled and not cinematic_enter_zone_aExecuted) then
		cinematic_enter_zone_a();
	end
	getEntityByName("ZON_Trigger_Enter_ZoneA"):destroy();
end

function cinematic_enter_zone_a()

	subClear();
	execScriptDelayed("activateSubtitles(20);", 0.27);
	execScriptDelayed("activateSubtitles(21);", 5);
	execScriptDelayed("deactivateSubtitles();", 9.5);
	temp = getEntityByName("intro_trigger_muralla");
	temp:destroy();

	setInBlackScreen(0.25);
	execScriptDelayed("setOutBlackScreen(0.25);",0.3);
	execScriptDelayed("move(\"The Player\", VEC3(-40,0,-90),VEC3(-41,0,-90));",3);
	execScriptDelayed("resetMainCameras();",3.5);
	execScriptDelayed("blendInCamera(\"Camera_Cinematic_ZoneA_Tower\", 0.0, \"cinematic\", \"\")", 0.27);

	execScriptDelayed("blendInCamera(\"Camera_Cinematic_Sonar_2\", 10.0, \"cinematic\", \"\")", 5);
	execScriptDelayed("blendInCamera(\"Camera_Cinematic_Sonar_1\", 0.0, \"cinematic\", \"\")", 5);
	execScriptDelayed("blendOutCamera(\"Camera_Cinematic_ZoneA_Tower\", 0)", 5.25);

	execScriptDelayed("setInBlackScreen(0.25)",9);
	execScriptDelayed("blendOutCamera(\"Camera_Cinematic_Sonar_2\", 0)", 9.25);
	execScriptDelayed("blendOutCamera(\"Camera_Cinematic_Sonar_1\", 0)", 9.25);
	execScriptDelayed("setOutBlackScreen(0.25);",9.75);

	setCinematicPlayerState(true, "inhibitor_capsules", "");
	execScriptDelayed("setCinematicPlayerState(false, \"\")", 10);
	cinematic_enter_zone_aExecuted = true;

end

function cinematic_tower_activated()
	gameManager.isCinematicMode = true;
	setInBlackScreen(0.25);
	execScriptDelayed("setOutBlackScreen(0.25);",0.3);
	subClear();
	execScriptDelayed("activateSubtitles(21);", 2.25);
	execScriptDelayed("deactivateSubtitles();", 6);

	execScriptDelayed("blendInCamera(\"Camera_Cinematic_Exit_Door_2\", 9, \"cinematic\", \"quintinout\")", 1.5);
	execScriptDelayed("blendInCamera(\"Camera_Cinematic_Exit_Door_1\", 0.0, \"cinematic\", \"\")", 0.27);

	execScriptDelayed("setInBlackScreen(0.25)",11.5);
	execScriptDelayed("blendOutCamera(\"Camera_Cinematic_Exit_Door_1\", 0)", 11.75);
	execScriptDelayed("blendOutCamera(\"Camera_Cinematic_Exit_Door_2\", 0)", 11.75);
	execScriptDelayed("setOutBlackScreen(0.25);",12.25);
	cinematic_tower_activatedExecuted = true;
	setCinematicPlayerState(true, "inhibitor_capsules", "");
	execScriptDelayed("setCinematicPlayerState(false, \"\")", 12.75);
	execScriptDelayed("gameManager.isCinematicMode = false;", 12.8);
end

function onTriggerEnter_ZON_Trigger_Exit_ZoneA_player()
	getEntityByName("ZON_Trigger_Exit_ZoneA"):destroy();
	tdoor = toDoor(toEntity(getEntityByName("zone_a_outmarco_puerta002")):getCompByName("door"));
	tdoor:setClosedScript("destroyZoneAPreloadCol()");
	tdoor:close();
end

function destroyZoneAPreloadCol()
	destroyPartialScene();
	execScriptDelayed("preloadScene(\"scene_coliseo_2\")", 0.1);
end