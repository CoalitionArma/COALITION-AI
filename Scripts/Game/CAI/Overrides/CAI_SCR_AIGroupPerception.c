//! Group level features, all driven by the group's perception update (every 2 s):
//! - Contact reports: a group that has identified enemies passes their positions to friendly groups in range.
//!   Receivers get them the same way they get a heard gunshot, so vanilla's cluster logic sends fireteams to investigate.
//! - Artillery support: a friendly group crewing static artillery answers a contact report with a fire mission.
//! - Hunt: a group without orders keeps a Search and Destroy waypoint on the nearest enemy player.
//! - Banzai charge: only its clean-up is driven from here, see SCR_AIGroup.CAI_StartBanzai.
modded class SCR_AIGroupPerception : Managed
{
	protected static const ResourceName CAI_HUNT_WAYPOINT_PREFAB = "{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et";
	protected static const float CAI_HUNT_INTERVAL_S = 15;			//!< How often a hunting group looks for its target again
	protected static const float CAI_HUNT_MOVE_THRESHOLD_SQ = 2500;	//!< Target must move this far (50 m, squared) before the waypoint is replaced

	//! Every group perception that has updated at least once.
	//! Weak pointers: entries of deleted groups turn null and are removed in CAI_ShareTargets and CAI_Register.
	protected static ref array<SCR_AIGroupPerception> s_aCAI_Perceptions = {};
	protected static ref Resource s_CAI_HuntWaypointResource; //!< Kept loaded, a prefab must outlive the spawn call

	protected bool m_bCAI_Registered;
	protected float m_fCAI_NextShareTime;		//!< PerceptionManager time, seconds. Same for the times below.

	protected ref SCR_AIStaticArtilleryActivity m_CAI_FireMission;
	protected float m_fCAI_NextFireMissionTime;

	protected AIWaypoint m_CAI_HuntWaypoint;	//!< Waypoint we spawned for hunting, null when not hunting
	protected float m_fCAI_NextHuntTime;

	//------------------------------------------------------------------------------------------------
	//! Called by the group every PERCEPTION_UPDATE_TIMER_MS (2 s)
	override void Update()
	{
		super.Update();

		if (!m_Group)
			return;

		if (!m_bCAI_Registered)
			CAI_Register();

		m_Group.CAI_UpdateBanzai();

		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return;

		float timeNow = pm.GetTime();

		if (m_CAI_FireMission)
			CAI_UpdateFireMission(timeNow);

		CAI_ShareTargets(timeNow);

		if (timeNow >= m_fCAI_NextHuntTime)
		{
			m_fCAI_NextHuntTime = timeNow + CAI_HUNT_INTERVAL_S;
			CAI_UpdateHunt();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CAI_Register()
	{
		// Groups are created rarely, good moment to drop entries of deleted ones
		for (int i = s_aCAI_Perceptions.Count() - 1; i >= 0; i--)
		{
			if (!s_aCAI_Perceptions[i])
				s_aCAI_Perceptions.Remove(i);
		}

		s_aCAI_Perceptions.Insert(this);
		m_bCAI_Registered = true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CAI_IsFriendlyFaction(Faction myFaction, Faction otherFaction)
	{
		if (!myFaction || !otherFaction)
			return false;

		return myFaction == otherFaction || myFaction.IsFactionFriendly(otherFaction);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if the group has a mortar or other static artillery it crews or can crew
	bool CAI_HasArtillery()
	{
		if (!m_Utility || !m_Utility.m_VehicleMgr || m_Utility.m_VehicleMgr.GetVehiclesCount() == 0)
			return false;

		array<ref SCR_AIGroupVehicle> vehicles = {};
		m_Utility.m_VehicleMgr.GetAllVehicles(vehicles);
		foreach (SCR_AIGroupVehicle vehicle : vehicles)
		{
			SCR_AIVehicleUsageComponent usageComp = vehicle.GetVehicleUsageComponent();
			if (usageComp && usageComp.GetVehicleType() == EAIVehicleType.STATIC_ARTILLERY)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Contact reports

	//------------------------------------------------------------------------------------------------
	//! \return true if this group should get contact reports from a group of senderFaction at senderPos
	bool CAI_CanReceiveFrom(Faction senderFaction, vector senderPos, float rangeSq)
	{
		if (!m_Group || m_Group.GetAgentsCount() == 0)
			return false;

		if (!CAI_IsFriendlyFaction(m_Group.GetFaction(), senderFaction))
			return false;

		if (vector.DistanceSqXZ(m_Group.GetCenterOfMass(), senderPos) > rangeSq)
			return false;

		if (!m_Group.CAI_GetSettings().m_bShareTargets)
			return false;

		// Artillery crews stay on their guns instead of walking off to investigate
		return !CAI_HasArtillery();
	}

	//------------------------------------------------------------------------------------------------
	protected void CAI_ShareTargets(float timeNow)
	{
		if (m_aTargets.IsEmpty() || timeNow < m_fCAI_NextShareTime)
			return;

		CAI_Settings settings = m_Group.CAI_GetSettings();
		if (!settings.m_bShareTargets)
			return;

		// Only report enemies we see right now. Targets stop being IDENTIFIED 10 s after they were last seen.
		bool hasContact;
		foreach (SCR_AITargetInfo target : m_aTargets)
		{
			if (target.m_eCategory == EAITargetInfoCategory.IDENTIFIED && target.m_Entity)
			{
				hasContact = true;
				break;
			}
		}

		// The timer is not advanced without contact, so a new contact is reported at the next update
		if (!hasContact)
			return;

		m_fCAI_NextShareTime = timeNow + settings.m_fShareInterval;

		Faction myFaction = m_Group.GetFaction();
		if (!myFaction)
			return;

		vector myPos = m_Group.GetCenterOfMass();
		float rangeSq = settings.m_fShareRange * settings.m_fShareRange;

		// Biggest threat we can see, as a target for artillery
		bool requestFireMission;
		vector fireMissionPos;
		int fireMissionTargetCount;
		if (m_MostDangerousCluster && m_MostDangerousCluster.m_State && m_MostDangerousCluster.m_State.m_iCountIdentified > 0)
		{
			requestFireMission = true;
			fireMissionPos = m_MostDangerousCluster.m_State.GetCenterPosition();
			fireMissionTargetCount = m_MostDangerousCluster.m_State.m_iCountIdentified;
		}

		for (int i = s_aCAI_Perceptions.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroupPerception receiver = s_aCAI_Perceptions[i];
			if (!receiver)
			{
				s_aCAI_Perceptions.Remove(i);
				continue;
			}

			// One fire mission per report is enough. An artillery group may also fire at what it sees itself.
			if (requestFireMission && receiver.CAI_RequestFireMission(myFaction, fireMissionPos, fireMissionTargetCount, timeNow))
				requestFireMission = false;

			if (receiver == this || !receiver.CAI_CanReceiveFrom(myFaction, myPos, rangeSq))
				continue;

			foreach (SCR_AITargetInfo target : m_aTargets)
			{
				if (target.m_eCategory != EAITargetInfoCategory.IDENTIFIED || !target.m_Entity)
					continue;

				// Not endangering: receivers investigate, but don't treat it as being shot at
				receiver.AddOrUpdateGunshot(target.m_Entity, target.m_vWorldPos, target.m_Faction, target.m_fTimestamp, false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Artillery support

	//------------------------------------------------------------------------------------------------
	//! Asks this group to shell targetPos. The decision uses this group's settings, not the caller's.
	//! \return true if a fire mission was started
	bool CAI_RequestFireMission(Faction callerFaction, vector targetPos, int targetCount, float timeNow)
	{
		if (!m_Group || !m_Utility || m_Group.GetAgentsCount() == 0)
			return false;

		if (m_CAI_FireMission || timeNow < m_fCAI_NextFireMissionTime)
			return false;

		Faction myFaction = m_Group.GetFaction();
		if (!CAI_IsFriendlyFaction(myFaction, callerFaction))
			return false;

		CAI_Settings settings = m_Group.CAI_GetSettings();
		if (!settings.m_bArtillerySupport || targetCount < settings.m_iArtilleryMinTargets)
			return false;

		float safeDistanceSq = settings.m_fArtillerySafeDistance * settings.m_fArtillerySafeDistance;
		float distanceSq = vector.DistanceSqXZ(m_Group.GetCenterOfMass(), targetPos);
		if (distanceSq < safeDistanceSq || distanceSq > settings.m_fArtilleryRange * settings.m_fArtilleryRange)
			return false;

		if (!CAI_HasArtillery())
			return false;

		// Busy with a fire mission from an Artillery Support waypoint
		if (m_Utility.FindActionOfType(SCR_AIStaticArtilleryActivity))
			return false;

		if (!CAI_IsTargetAreaClear(myFaction, targetPos, safeDistanceSq))
			return false;

		// Vanilla activity, same one the Artillery Support waypoint uses. It ends itself after the given number of shots.
		m_CAI_FireMission = new SCR_AIStaticArtilleryActivity(m_Utility, null, targetPos, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, settings.m_iArtilleryRounds);
		m_Utility.AddAction(m_CAI_FireMission);
		m_fCAI_NextFireMissionTime = timeNow + settings.m_fArtilleryCooldown;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if this group has members within the radius of pos and is friendly to faction
	bool CAI_IsFriendlyGroupNear(Faction faction, vector pos, float radiusSq)
	{
		if (!m_Group || m_Group.GetAgentsCount() == 0)
			return false;

		if (!CAI_IsFriendlyFaction(m_Group.GetFaction(), faction))
			return false;

		return vector.DistanceSqXZ(m_Group.GetCenterOfMass(), pos) < radiusSq;
	}

	//------------------------------------------------------------------------------------------------
	//! \return false if a friendly AI group or a living friendly player is within the radius of targetPos
	protected bool CAI_IsTargetAreaClear(Faction myFaction, vector targetPos, float radiusSq)
	{
		foreach (SCR_AIGroupPerception perception : s_aCAI_Perceptions)
		{
			if (perception && perception.CAI_IsFriendlyGroupNear(myFaction, targetPos, radiusSq))
				return false;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		array<int> players = {};
		playerManager.GetPlayers(players);
		foreach (int playerId : players)
		{
			vector playerPos;
			Faction playerFaction;
			if (!CAI_GetLivingPlayer(playerManager, playerId, playerPos, playerFaction))
				continue;

			if (CAI_IsFriendlyFaction(myFaction, playerFaction) && vector.DistanceSqXZ(playerPos, targetPos) < radiusSq)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Calls off a fire mission which is still running after the cooldown, e.g. because the target can't be reached
	protected void CAI_UpdateFireMission(float timeNow)
	{
		EAIActionState state = m_CAI_FireMission.GetActionState();
		if (state == EAIActionState.COMPLETED || state == EAIActionState.FAILED)
		{
			m_CAI_FireMission = null;
			return;
		}

		if (timeNow < m_fCAI_NextFireMissionTime)
			return;

		m_CAI_FireMission.Complete();
		m_CAI_FireMission = null;
	}

	//------------------------------------------------------------------------------------------------
	// Hunt

	//------------------------------------------------------------------------------------------------
	//! \return false if the player has no living character
	protected bool CAI_GetLivingPlayer(PlayerManager playerManager, int playerId, out vector outPos, out Faction outFaction)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(playerManager.GetPlayerControlledEntity(playerId));
		if (!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		if (!factionComp)
			return false;

		outPos = character.GetOrigin();
		outFaction = factionComp.GetAffiliatedFaction();
		return outFaction != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Position of the nearest living enemy player within the radius
	protected bool CAI_FindHuntTarget(float radius, out vector outPos)
	{
		Faction myFaction = m_Group.GetFaction();
		if (!myFaction)
			return false;

		vector myPos = m_Group.GetCenterOfMass();
		float bestDistanceSq = radius * radius;
		bool found;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		array<int> players = {};
		playerManager.GetPlayers(players);
		foreach (int playerId : players)
		{
			vector playerPos;
			Faction playerFaction;
			if (!CAI_GetLivingPlayer(playerManager, playerId, playerPos, playerFaction))
				continue;

			if (!myFaction.IsFactionEnemy(playerFaction))
				continue;

			float distanceSq = vector.DistanceSqXZ(myPos, playerPos);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				outPos = playerPos;
				found = true;
			}
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	//! Called every CAI_HUNT_INTERVAL_S
	protected void CAI_UpdateHunt()
	{
		CAI_Settings settings = m_Group.CAI_GetSettings();

		// Not for groups led by a player, empty groups, or artillery crews
		if (!settings.m_bHunt || m_Group.IsSlave() || m_Group.GetAgentsCount() == 0 || CAI_HasArtillery())
		{
			CAI_RemoveHuntWaypoint();
			return;
		}

		// Groups which were given other waypoints follow those
		array<AIWaypoint> waypoints = {};
		m_Group.GetWaypoints(waypoints);
		bool hasHuntWaypoint = m_CAI_HuntWaypoint && waypoints.Contains(m_CAI_HuntWaypoint);
		if (waypoints.Count() > 1 || (waypoints.Count() == 1 && !hasHuntWaypoint))
		{
			CAI_RemoveHuntWaypoint();
			return;
		}

		vector targetPos;
		if (!CAI_FindHuntTarget(settings.m_fHuntRadius, targetPos))
		{
			CAI_RemoveHuntWaypoint();
			return;
		}

		// Still close enough to where we are already heading
		if (hasHuntWaypoint && vector.DistanceSqXZ(m_CAI_HuntWaypoint.GetOrigin(), targetPos) < CAI_HUNT_MOVE_THRESHOLD_SQ)
			return;

		CAI_RemoveHuntWaypoint();

		if (!s_CAI_HuntWaypointResource || !s_CAI_HuntWaypointResource.IsValid())
			s_CAI_HuntWaypointResource = Resource.Load(CAI_HUNT_WAYPOINT_PREFAB);

		if (!s_CAI_HuntWaypointResource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = targetPos;

		AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(s_CAI_HuntWaypointResource, GetGame().GetWorld(), spawnParams));
		if (!waypoint)
			return;

		m_CAI_HuntWaypoint = waypoint;
		m_Group.AddWaypoint(waypoint);
	}

	//------------------------------------------------------------------------------------------------
	protected void CAI_RemoveHuntWaypoint()
	{
		if (!m_CAI_HuntWaypoint)
			return;

		array<AIWaypoint> waypoints = {};
		m_Group.GetWaypoints(waypoints);
		if (waypoints.Contains(m_CAI_HuntWaypoint))
			m_Group.RemoveWaypoint(m_CAI_HuntWaypoint);

		SCR_EntityHelper.DeleteEntityAndChildren(m_CAI_HuntWaypoint);
		m_CAI_HuntWaypoint = null;
	}
}
