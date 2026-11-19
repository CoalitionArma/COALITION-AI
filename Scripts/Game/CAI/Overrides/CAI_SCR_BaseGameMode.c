modded class SCR_BaseGameMode : BaseGameMode
{
	protected CAI_SettingsComponent m_CAI_SettingsComponent;
	protected bool m_bCAI_SettingsComponentSearched;
	protected ref CAI_Settings m_CAI_FallbackSettings; //!< Used when the mission has no CAI_SettingsComponent

	//------------------------------------------------------------------------------------------------
	//! Mission-wide AI settings. Never returns null.
	//! After changing values on the returned object, call CAI_SettingsManager.Invalidate().
	CAI_Settings CAI_GetSettings()
	{
		if (!m_bCAI_SettingsComponentSearched)
		{
			m_CAI_SettingsComponent = CAI_SettingsComponent.Cast(FindComponent(CAI_SettingsComponent));
			m_bCAI_SettingsComponentSearched = true;
		}

		if (m_CAI_SettingsComponent)
			return m_CAI_SettingsComponent.GetSettings();

		if (!m_CAI_FallbackSettings)
			m_CAI_FallbackSettings = CAI_Settings.CreateDefault();

		return m_CAI_FallbackSettings;
	}
}
