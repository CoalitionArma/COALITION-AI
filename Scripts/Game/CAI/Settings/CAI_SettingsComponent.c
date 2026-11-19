[ComponentEditorProps(category: "COALITION/AI", description: "Mission-wide COALITION AI settings. Add to the game mode entity. Game Masters can change every value in the game mode attributes.")]
class CAI_SettingsComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Holds the mission-wide AI settings.
//! Missions without this component still get the mod's default tuning (see SCR_BaseGameMode.CAI_GetSettings).
class CAI_SettingsComponent : SCR_BaseGameModeComponent
{
	[Attribute(desc: "AI tuning used by every AI group that has no override of its own.")]
	protected ref CAI_Settings m_Settings;

	//------------------------------------------------------------------------------------------------
	//! Never returns null
	CAI_Settings GetSettings()
	{
		// Component was added but the settings object was left empty in the prefab
		if (!m_Settings)
			m_Settings = CAI_Settings.CreateDefault();

		return m_Settings;
	}
}
