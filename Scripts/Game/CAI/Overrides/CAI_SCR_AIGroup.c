modded class SCR_AIGroup : ChimeraAIGroup
{
	protected static const ResourceName CAI_BANZAI_WAYPOINT_PREFAB = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
	protected static const float CAI_BANZAI_COMPLETION_RADIUS = 10;	//!< Metres from the position at which the charge is over
	protected static const float CAI_BANZAI_TIMEOUT_MS = 300000;		//!< A charge which never arrives is called off after this time

	protected static ref Resource s_CAI_BanzaiWaypointResource; //!< Kept loaded, a prefab must outlive the spawn call

	[Attribute(desc: "COALITION AI: settings for this group only. Leave empty to use the mission-wide settings.")]
	protected ref CAI_Settings m_CAI_Override;

	// Banzai charge, see CAI_StartBanzai
	protected bool m_bCAI_Banzai;
	protected AIWaypoint m_CAI_BanzaiWaypoint;
	protected float m_fCAI_BanzaiEndTime_ms; //!< World time
	protected ref SCR_AISettingBase m_CAI_BanzaiSpeedSetting;
	protected ref SCR_AISettingBase m_CAI_BanzaiStanceSetting;

	//------------------------------------------------------------------------------------------------
	//! \return This group's own settings, or null when it uses the mission-wide ones
	CAI_Settings CAI_GetOverride()
	{
		return m_CAI_Override;
	}

	//------------------------------------------------------------------------------------------------
	//! Gives the group its own settings, starting as a copy of the mission-wide ones.
	//! \return The existing override if the group already had one
	CAI_Settings CAI_CreateOverride()
	{
		if (m_CAI_Override)
			return m_CAI_Override;

		m_CAI_Override = new CAI_Settings();
		m_CAI_Override.CopyFrom(CAI_SettingsManager.GetGlobal());
		CAI_SettingsManager.Invalidate();

		return m_CAI_Override;
	}

	//------------------------------------------------------------------------------------------------
	//! Back to the mission-wide settings
	void CAI_ClearOverride()
	{
		if (!m_CAI_Override)
			return;

		m_CAI_Override = null;
		CAI_SettingsManager.Invalidate();
	}

	//------------------------------------------------------------------------------------------------
	//! Settings this group actually uses right now. Never returns null.
	CAI_Settings CAI_GetSettings()
	{
		return CAI_SettingsManager.Resolve(m_CAI_Override);
	}

	//------------------------------------------------------------------------------------------------
	// Banzai charge
	//
	// The whole group runs straight at a position, standing, ignoring suppression and cover.
	// Soldiers with a target keep shooting at it while they run (see the modded SCR_AICombatMoveLogic_Attack).
	// It is a Move waypoint put in front of the group's other waypoints, and lasts while that waypoint is the current one.

	//------------------------------------------------------------------------------------------------
	//! Server only. Starts a charge at the position, replacing a charge already under way.
	void CAI_StartBanzai(vector position)
	{
		CAI_StopBanzai();

		if (!s_CAI_BanzaiWaypointResource || !s_CAI_BanzaiWaypointResource.IsValid())
			s_CAI_BanzaiWaypointResource = Resource.Load(CAI_BANZAI_WAYPOINT_PREFAB);

		if (!s_CAI_BanzaiWaypointResource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = position;

		AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(s_CAI_BanzaiWaypointResource, GetGame().GetWorld(), spawnParams));
		if (!waypoint)
			return;

		waypoint.SetCompletionRadius(CAI_BANZAI_COMPLETION_RADIUS);

		// Run or sprint, and never crouch or go prone, whatever the soldier is doing
		SCR_AIGroupSettingsComponent settingsComp = SCR_AIGroupSettingsComponent.Cast(FindComponent(SCR_AIGroupSettingsComponent));
		if (settingsComp)
		{
			m_CAI_BanzaiSpeedSetting = SCR_AICharacterMovementSpeedSetting_Range.Create(SCR_EAISettingOrigin.EDITOR, SCR_EAIBehaviorCause.ALWAYS, EMovementType.RUN, EMovementType.SPRINT);
			m_CAI_BanzaiStanceSetting = SCR_AICharacterStanceSetting.Create(SCR_EAISettingOrigin.EDITOR, SCR_EAIBehaviorCause.ALWAYS, ECharacterStance.STAND);
			settingsComp.AddSetting(m_CAI_BanzaiSpeedSetting, false, false);
			settingsComp.AddSetting(m_CAI_BanzaiStanceSetting, false, false);
		}

		m_CAI_BanzaiWaypoint = waypoint;
		m_fCAI_BanzaiEndTime_ms = GetGame().GetWorld().GetWorldTime() + CAI_BANZAI_TIMEOUT_MS;
		m_bCAI_Banzai = true;

		// In front of the waypoints the group already has, they continue afterwards
		AddWaypointAt(waypoint, 0);

		// Soldiers refresh their cached state, which is where they become immune to suppression
		CAI_SettingsManager.Invalidate();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Ends the charge, the group goes back to normal behaviour and its other waypoints.
	void CAI_StopBanzai()
	{
		if (!m_bCAI_Banzai)
			return;

		m_bCAI_Banzai = false;

		SCR_AIGroupSettingsComponent settingsComp = SCR_AIGroupSettingsComponent.Cast(FindComponent(SCR_AIGroupSettingsComponent));
		if (settingsComp)
		{
			if (m_CAI_BanzaiSpeedSetting)
				settingsComp.RemoveSetting(m_CAI_BanzaiSpeedSetting);

			if (m_CAI_BanzaiStanceSetting)
				settingsComp.RemoveSetting(m_CAI_BanzaiStanceSetting);
		}

		m_CAI_BanzaiSpeedSetting = null;
		m_CAI_BanzaiStanceSetting = null;

		// The waypoint is already gone from the group if the charge arrived
		if (m_CAI_BanzaiWaypoint)
		{
			array<AIWaypoint> waypoints = {};
			GetWaypoints(waypoints);
			if (waypoints.Contains(m_CAI_BanzaiWaypoint))
				RemoveWaypoint(m_CAI_BanzaiWaypoint);

			SCR_EntityHelper.DeleteEntityAndChildren(m_CAI_BanzaiWaypoint);
			m_CAI_BanzaiWaypoint = null;
		}

		CAI_SettingsManager.Invalidate();
	}

	//------------------------------------------------------------------------------------------------
	//! \return true while the group is charging
	bool CAI_IsBanzai()
	{
		return m_bCAI_Banzai && m_CAI_BanzaiWaypoint && GetCurrentWaypoint() == m_CAI_BanzaiWaypoint;
	}

	//------------------------------------------------------------------------------------------------
	//! Only valid while CAI_IsBanzai() is true
	vector CAI_GetBanzaiPosition()
	{
		return m_CAI_BanzaiWaypoint.GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	//! Called every 2 s by the group's perception. Cleans up once the charge arrived, was replaced by other orders, or timed out.
	void CAI_UpdateBanzai()
	{
		if (!m_bCAI_Banzai)
			return;

		if (!CAI_IsBanzai() || GetGame().GetWorld().GetWorldTime() > m_fCAI_BanzaiEndTime_ms)
			CAI_StopBanzai();
	}
}
