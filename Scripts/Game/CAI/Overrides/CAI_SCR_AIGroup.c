modded class SCR_AIGroup : ChimeraAIGroup
{
	//! The Banzai charge waypoint. Game Master places it from the command bar, scripts through CAI_StartBanzai.
	static const ResourceName CAI_BANZAI_WAYPOINT_PREFAB = "{6B7C41D9A3E5F012}PrefabsEditable/Auto/AI/Waypoints/E_CAI_AIWaypoint_Banzai.et";
	protected static const string CAI_BANZAI_WAYPOINT_FILE = "E_CAI_AIWaypoint_Banzai.et";
	protected static const float CAI_BANZAI_ARRIVAL_RADIUS = 5;	//!< Metres from the position at which a soldier's own run ends
	protected static const float CAI_BANZAI_TIMEOUT_MS = 300000;	//!< A charge which never arrives is called off after this time

	protected static ref Resource s_CAI_BanzaiWaypointResource; //!< Kept loaded, a prefab must outlive the spawn call

	[Attribute(desc: "COALITION AI: settings for this group only. Leave empty to use the mission-wide settings.")]
	protected ref CAI_Settings m_CAI_Override;

	// Banzai charge, see CAI_UpdateBanzai
	protected bool m_bCAI_Banzai;
	protected AIWaypoint m_CAI_BanzaiWaypoint;	//!< Current waypoint, if it is a banzai waypoint
	protected AIWaypoint m_CAI_CheckedWaypoint;	//!< Current waypoint at the last check, so its prefab is looked at only once
	protected float m_fCAI_BanzaiEndTime_ms;	//!< World time
	protected ref SCR_AISettingBase m_CAI_BanzaiSpeedSetting;
	protected ref SCR_AISettingBase m_CAI_BanzaiStanceSetting;
	protected ref SCR_AISettingBase m_CAI_BanzaiGroupSpeedSetting;
	protected ref array<ref SCR_AIMoveIndividuallyBehavior> m_aCAI_BanzaiBehaviors = {}; //!< One per soldier, see CAI_SetBanzai

	// All-out assault while the current waypoint is Search and Destroy, see CAI_UpdateAssault
	protected bool m_bCAI_Assault;
	protected ref SCR_AISettingBase m_CAI_AssaultSpeedSetting;

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
	// A waypoint like any other: while it is the group's current waypoint, the whole group sprints straight at it,
	// each soldier on his own instead of in formation, standing, ignoring suppression and cover. Sprinting soldiers
	// can't aim, so nobody fires until they arrive
	// (see the modded SCR_AICombatMoveLogic_Attack). The charge ends when the waypoint completes or is replaced.

	//------------------------------------------------------------------------------------------------
	static bool CAI_IsBanzaiWaypoint(AIWaypoint waypoint)
	{
		if (!waypoint)
			return false;

		EntityPrefabData prefabData = waypoint.GetPrefabData();
		if (!prefabData)
			return false;

		string prefabName = prefabData.GetPrefabName();
		return prefabName.EndsWith(CAI_BANZAI_WAYPOINT_FILE);
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. For scripts: gives the group a banzai waypoint at the position, in front of its other waypoints.
	//! \return The waypoint, or null if it could not be created
	AIWaypoint CAI_StartBanzai(vector position)
	{
		if (!s_CAI_BanzaiWaypointResource || !s_CAI_BanzaiWaypointResource.IsValid())
			s_CAI_BanzaiWaypointResource = Resource.Load(CAI_BANZAI_WAYPOINT_PREFAB);

		if (!s_CAI_BanzaiWaypointResource.IsValid())
			return null;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = position;

		AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(s_CAI_BanzaiWaypointResource, GetGame().GetWorld(), spawnParams));
		if (!waypoint)
			return null;

		AddWaypointAt(waypoint, 0);
		CAI_UpdateBanzai();

		return waypoint;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Calls off the charge under way by removing its waypoint. The group continues with its next waypoint.
	void CAI_StopBanzai()
	{
		AIWaypoint waypoint = m_CAI_BanzaiWaypoint;
		if (waypoint)
		{
			RemoveWaypoint(waypoint);
			SCR_EntityHelper.DeleteEntityAndChildren(waypoint);
		}

		CAI_UpdateBanzai();
	}

	//------------------------------------------------------------------------------------------------
	//! \return true while the group is charging. Up to 2 s late when a charge starts, immediate when it ends.
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
	//! Called every 2 s by the group's perception. Starts and ends the charge as the current waypoint changes.
	void CAI_UpdateBanzai()
	{
		AIWaypoint current = GetCurrentWaypoint();
		float worldTime_ms = GetGame().GetWorld().GetWorldTime();

		// Second condition: the banzai waypoint was deleted, and with it both pointers turned null
		if (current != m_CAI_CheckedWaypoint || (m_bCAI_Banzai && !m_CAI_BanzaiWaypoint))
		{
			m_CAI_CheckedWaypoint = current;

			if (CAI_IsBanzaiWaypoint(current))
			{
				m_CAI_BanzaiWaypoint = current;
				m_fCAI_BanzaiEndTime_ms = worldTime_ms + CAI_BANZAI_TIMEOUT_MS;
			}
			else
			{
				m_CAI_BanzaiWaypoint = null;
			}

			CAI_SetBanzai(m_CAI_BanzaiWaypoint != null);
			return;
		}

		if (m_bCAI_Banzai && worldTime_ms > m_fCAI_BanzaiEndTime_ms)
			CAI_StopBanzai();
	}

	//------------------------------------------------------------------------------------------------
	protected void CAI_SetBanzai(bool banzai)
	{
		if (banzai == m_bCAI_Banzai)
			return;

		m_bCAI_Banzai = banzai;

		// Sprint and stay standing, whatever the soldier is doing
		SCR_AIGroupSettingsComponent settingsComp = SCR_AIGroupSettingsComponent.Cast(FindComponent(SCR_AIGroupSettingsComponent));
		if (settingsComp)
		{
			if (banzai)
			{
				m_CAI_BanzaiSpeedSetting = SCR_AICharacterMovementSpeedSetting.Create(SCR_EAISettingOrigin.EDITOR, SCR_EAIBehaviorCause.ALWAYS, EMovementType.SPRINT);
				m_CAI_BanzaiStanceSetting = SCR_AICharacterStanceSetting.Create(SCR_EAISettingOrigin.EDITOR, SCR_EAIBehaviorCause.ALWAYS, ECharacterStance.STAND);
				m_CAI_BanzaiGroupSpeedSetting = SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.EDITOR, EMovementType.SPRINT);
				settingsComp.AddSetting(m_CAI_BanzaiSpeedSetting, false, false);
				settingsComp.AddSetting(m_CAI_BanzaiStanceSetting, false, false);
				settingsComp.AddSetting(m_CAI_BanzaiGroupSpeedSetting, false, false);
			}
			else
			{
				if (m_CAI_BanzaiSpeedSetting)
					settingsComp.RemoveSetting(m_CAI_BanzaiSpeedSetting);

				if (m_CAI_BanzaiStanceSetting)
					settingsComp.RemoveSetting(m_CAI_BanzaiStanceSetting);

				if (m_CAI_BanzaiGroupSpeedSetting)
					settingsComp.RemoveSetting(m_CAI_BanzaiGroupSpeedSetting);
			}
		}

		if (!banzai)
		{
			m_CAI_BanzaiSpeedSetting = null;
			m_CAI_BanzaiStanceSetting = null;
			m_CAI_BanzaiGroupSpeedSetting = null;
		}

		// Formation: a Move waypoint makes the leader walk the path and everyone else keep their place around him,
		// at the formation's pace. So every soldier on foot gets his own order to run to the position, which
		// outranks keeping formation. Soldiers who join the group during the charge follow in formation.
		foreach (SCR_AIMoveIndividuallyBehavior behavior : m_aCAI_BanzaiBehaviors)
		{
			EAIActionState state = behavior.GetActionState();
			if (state != EAIActionState.COMPLETED && state != EAIActionState.FAILED)
				behavior.Fail();
		}
		m_aCAI_BanzaiBehaviors.Clear();

		if (banzai)
		{
			vector position = m_CAI_BanzaiWaypoint.GetOrigin();
			array<AIAgent> agents = {};
			GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(agent);
				if (!chimeraAgent || !chimeraAgent.m_UtilityComponent)
					continue;

				// Crews stay with their vehicle, the group's move order drives it there
				IEntity character = chimeraAgent.GetControlledEntity();
				if (!character)
					continue;

				CompartmentAccessComponent compartmentAccess = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
				if (compartmentAccess && compartmentAccess.IsInCompartment())
					continue;

				SCR_AIMoveIndividuallyBehavior behavior = new SCR_AIMoveIndividuallyBehavior(chimeraAgent.m_UtilityComponent, null, position, radius: CAI_BANZAI_ARRIVAL_RADIUS);
				chimeraAgent.m_UtilityComponent.AddAction(behavior);
				m_aCAI_BanzaiBehaviors.Insert(behavior);
			}
		}
		// Soldiers refresh their cached state, which is where they become immune to suppression
		CAI_SettingsManager.Invalidate();
	}

	//------------------------------------------------------------------------------------------------
	// Search and Destroy assault
	//
	// While the group's current waypoint is Search and Destroy (given by a mission, Game Master or Hunt), and the
	// setting allows it, the group assaults all-out. The effects are spread over the modded AI classes, which ask
	// CAI_IsAssaulting(): no suppression and full fire tempo (soldiers), no decision delay (cluster processor),
	// charge into the waypoint area and short waits in cover (SCR_AICombatMoveLogic_Attack).

	//------------------------------------------------------------------------------------------------
	//! Cached state, at most 2 s old. Cheap enough for code which runs often.
	bool CAI_IsAssaulting()
	{
		return m_bCAI_Assault;
	}

	//------------------------------------------------------------------------------------------------
	//! \return false if the group is not assaulting
	bool CAI_GetAssaultArea(out vector outPosition, out float outRadius)
	{
		if (!m_bCAI_Assault)
			return false;

		AIWaypoint waypoint = GetCurrentWaypoint();
		if (!waypoint)
			return false;

		outPosition = waypoint.GetOrigin();
		outRadius = waypoint.GetCompletionRadius();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Called every 2 s by the group's perception. Starts and ends the assault as the current waypoint or the setting changes.
	void CAI_UpdateAssault()
	{
		bool assault = SCR_SearchAndDestroyWaypoint.Cast(GetCurrentWaypoint()) && CAI_GetSettings().m_bAssaultOnSearchAndDestroy;
		if (assault == m_bCAI_Assault)
			return;

		m_bCAI_Assault = assault;

		// Never slower than a run while assaulting
		SCR_AIGroupSettingsComponent settingsComp = SCR_AIGroupSettingsComponent.Cast(FindComponent(SCR_AIGroupSettingsComponent));
		if (settingsComp)
		{
			if (assault)
			{
				m_CAI_AssaultSpeedSetting = SCR_AICharacterMovementSpeedSetting_Range.Create(SCR_EAISettingOrigin.EDITOR, SCR_EAIBehaviorCause.ALWAYS, EMovementType.RUN, EMovementType.SPRINT);
				settingsComp.AddSetting(m_CAI_AssaultSpeedSetting, false, false);
			}
			else if (m_CAI_AssaultSpeedSetting)
			{
				settingsComp.RemoveSetting(m_CAI_AssaultSpeedSetting);
			}
		}

		if (!assault)
			m_CAI_AssaultSpeedSetting = null;

		// Soldiers refresh their cached state
		CAI_SettingsManager.Invalidate();
	}
}
