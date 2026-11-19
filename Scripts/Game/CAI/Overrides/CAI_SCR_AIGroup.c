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

	// Breaking off after heavy losses or vehicle damage, see CAI_UpdateMorale
	protected static const ResourceName CAI_FALLBACK_WAYPOINT_PREFAB = "{3C790BB71C5CD960}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_ForcedMove.et";
	protected static const int CAI_MIN_STRENGTH_TO_BREAK = 3;		//!< Smaller groups never break from casualties
	protected static const float CAI_SMOKE_DISTANCE = 20;			//!< How far ahead of the group smoke is thrown
	protected static const float CAI_ASSAULT_SMOKE_RANGE_SQ = 22500;	//!< Assaults starting within 150 m of their waypoint throw smoke
	protected static ref Resource s_CAI_FallbackWaypointResource;
	protected int m_iCAI_MaxStrength;			//!< Most soldiers the group ever had
	protected bool m_bCAI_Broken;				//!< The group broke off once, it won't again
	protected AIWaypoint m_CAI_FallbackWaypoint;
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

			if (CAI_GetSettings().m_bSmokeBeforeCharge)
				CAI_ThrowSmoke(position);

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

		// Smoke only helps if the fight is about to start
		if (assault && CAI_GetSettings().m_bSmokeBeforeCharge)
		{
			vector waypointPos = GetCurrentWaypoint().GetOrigin();
			if (vector.DistanceSqXZ(GetCenterOfMass(), waypointPos) < CAI_ASSAULT_SMOKE_RANGE_SQ)
				CAI_ThrowSmoke(waypointPos);
		}

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

	//------------------------------------------------------------------------------------------------
	// Smoke

	//------------------------------------------------------------------------------------------------
	//! Has up to two soldiers who carry smoke grenades throw them a short way ahead of the group, towards the position.
	//! Uses vanilla's smoke cover logic, which never takes more than half of the group off the fight.
	void CAI_ThrowSmoke(vector towardsPosition)
	{
		SCR_AIGroupUtilityComponent utility = GetGroupUtilityComponent();
		if (!utility)
			return;

		vector myPos = GetCenterOfMass();
		vector direction = towardsPosition - myPos;
		direction[1] = 0;
		if (direction.LengthSq() < 1)
			return;

		direction.Normalize();
		vector smokePos = myPos + direction * CAI_SMOKE_DISTANCE;
		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);

		array<AIAgent> avoidAgents = {};
		array<AIAgent> excludeAgents = {};
		SCR_AIActivitySmokeCoverFeature smokeFeature = new SCR_AIActivitySmokeCoverFeature();
		smokeFeature.Execute(utility, smokePos, SCR_AIActivitySmokeCoverFeatureProperties.NONE, avoidAgents, excludeAgents, 2);
	}

	//------------------------------------------------------------------------------------------------
	// Breaking off
	//
	// A group in contact which has lost too many soldiers, or whose vehicle is badly damaged, throws smoke, drops
	// its orders and falls back away from the enemy on a Forced Move waypoint. It does so once.

	//------------------------------------------------------------------------------------------------
	//! \return true while the group is on its way to the position it falls back to
	bool CAI_IsFallingBack()
	{
		return m_CAI_FallbackWaypoint && GetCurrentWaypoint() == m_CAI_FallbackWaypoint;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true while the group holds a building on a Garrison waypoint
	bool CAI_IsGarrisoned()
	{
		return CAI_GarrisonWaypoint.Cast(GetCurrentWaypoint()) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Called every 2 s by the group's perception.
	//! \param inContact the group has identified enemies
	//! \param enemyPos their position, only used when inContact
	void CAI_UpdateMorale(bool inContact, vector enemyPos)
	{
		int strength = GetAgentsCount();
		if (strength > m_iCAI_MaxStrength)
			m_iCAI_MaxStrength = strength;

		// Groups led by a player follow him, and a banzai charge is not called off
		if (m_bCAI_Broken || !inContact || strength == 0 || IsSlave() || CAI_IsBanzai())
			return;

		CAI_Settings settings = CAI_GetSettings();

		bool breakOff;
		if (settings.m_iBreakCasualties > 0 && m_iCAI_MaxStrength >= CAI_MIN_STRENGTH_TO_BREAK)
		{
			float lossPercent = 100.0 * (m_iCAI_MaxStrength - strength) / m_iCAI_MaxStrength;
			breakOff = lossPercent >= settings.m_iBreakCasualties;
		}

		if (!breakOff && settings.m_iVehicleRetreatDamage > 0)
			breakOff = CAI_HasDamagedVehicle(settings.m_iVehicleRetreatDamage);

		if (breakOff)
			CAI_FallBack(enemyPos, settings.m_fFallbackDistance, settings.m_bSmokeBeforeCharge);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if a vehicle the group uses, other than a static weapon, has taken at least this much damage
	protected bool CAI_HasDamagedVehicle(int damagePercent)
	{
		SCR_AIGroupUtilityComponent utility = GetGroupUtilityComponent();
		if (!utility || !utility.m_VehicleMgr || utility.m_VehicleMgr.GetVehiclesCount() == 0)
			return false;

		array<ref SCR_AIGroupVehicle> vehicles = {};
		utility.m_VehicleMgr.GetAllVehicles(vehicles);
		foreach (SCR_AIGroupVehicle vehicle : vehicles)
		{
			IEntity entity = vehicle.GetEntity();
			SCR_AIVehicleUsageComponent usageComp = vehicle.GetVehicleUsageComponent();
			if (!entity || !usageComp)
				continue;

			EAIVehicleType vehicleType = usageComp.GetVehicleType();
			if (vehicleType == EAIVehicleType.STATIC_WEAPON || vehicleType == EAIVehicleType.STATIC_ARTILLERY)
				continue;

			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager && 100 * (1 - damageManager.GetHealthScaled()) >= damagePercent)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Makes the group break off now: smoke, all orders dropped, Forced Move away from the enemy.
	void CAI_FallBack(vector enemyPos, float distance, bool throwSmoke = true)
	{
		m_bCAI_Broken = true;

		vector myPos = GetCenterOfMass();
		vector direction = myPos - enemyPos;
		direction[1] = 0;
		if (direction.LengthSq() < 1)
			direction = vector.Forward;

		direction.Normalize();
		vector position = myPos + direction * distance;
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);

		if (throwSmoke)
			CAI_ThrowSmoke(enemyPos);

		if (!s_CAI_FallbackWaypointResource || !s_CAI_FallbackWaypointResource.IsValid())
			s_CAI_FallbackWaypointResource = Resource.Load(CAI_FALLBACK_WAYPOINT_PREFAB);

		if (!s_CAI_FallbackWaypointResource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = position;

		AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(s_CAI_FallbackWaypointResource, GetGame().GetWorld(), spawnParams));
		if (!waypoint)
			return;

		// The orders it had would only lead it back into the fight. The waypoints themselves are left in the world.
		array<AIWaypoint> waypoints = {};
		GetWaypoints(waypoints);
		foreach (AIWaypoint oldWaypoint : waypoints)
		{
			if (oldWaypoint)
				RemoveWaypoint(oldWaypoint);
		}

		m_CAI_FallbackWaypoint = waypoint;
		AddWaypoint(waypoint);
	}
}
