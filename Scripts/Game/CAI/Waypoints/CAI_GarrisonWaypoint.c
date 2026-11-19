class CAI_GarrisonWaypointClass : SCR_AIWaypointClass
{
}

//! Waypoint which makes the group occupy the building it is placed on and stay there.
//! It never completes on its own. Give the group another order, or delete the waypoint, to release it.
class CAI_GarrisonWaypoint : SCR_AIWaypoint
{
	//------------------------------------------------------------------------------------------------
	override SCR_AIWaypointState CreateWaypointState(SCR_AIGroupUtilityComponent groupUtilityComp)
	{
		return new CAI_GarrisonWaypointState(groupUtilityComp, this);
	}
}

//------------------------------------------------------------------------------------------------
//! Finds the positions inside the building the same way Clear building does, then gives every soldier one of them,
//! spread as far apart as the building allows. Soldiers fight from where they are (SCR_AIGroup.CAI_IsGarrisoned stops
//! the group from sending fireteams out) and are sent back to their spot when a fight has moved them off it.
class CAI_GarrisonWaypointState : CAI_ClearBuildingWaypointState
{
	protected static const int HOLD_STEP_MS = 5000;
	protected static const float SPOT_RADIUS = 1;		//!< A soldier has reached his spot within this distance
	protected static const float HOLD_RADIUS_SQ = 16;	//!< Further than this (4 m, squared) from his spot he is sent back

	// Parallel arrays, one entry per soldier
	protected ref array<AIAgent> m_aGarrisonAgents = {};
	protected ref array<vector> m_aGarrisonSpots = {};
	protected ref array<ref SCR_AIMoveIndividuallyBehavior> m_aGarrisonBehaviors = {};

	//------------------------------------------------------------------------------------------------
	override protected bool StartWork()
	{
		array<AIAgent> agents = {};
		m_Utility.m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (GetUtility(agent))
				m_aGarrisonAgents.Insert(agent);
		}

		int agentCount = m_aGarrisonAgents.Count();
		if (agentCount == 0)
		{
			PrintFormat("%1%2: no soldiers on foot", LOG_PREFIX, Type());
			return false;
		}

		// Spots as far from each other as possible: start with the one nearest to the group, then keep adding the
		// position which is furthest from all spots chosen so far.
		array<vector> spots = {};
		vector groupPos = m_Utility.m_Owner.GetCenterOfMass();
		int first;
		float firstDistSq = float.MAX;
		foreach (int p, vector position : m_aPositions)
		{
			float distSq = vector.DistanceSq(position, groupPos);
			if (distSq < firstDistSq)
			{
				firstDistSq = distSq;
				first = p;
			}
		}
		spots.Insert(m_aPositions[first]);
		m_aPositions.Remove(first);

		while (spots.Count() < agentCount && !m_aPositions.IsEmpty())
		{
			int best;
			float bestMinDistSq = -1;
			foreach (int c, vector candidate : m_aPositions)
			{
				float minDistSq = float.MAX;
				foreach (vector spot : spots)
				{
					minDistSq = Math.Min(minDistSq, vector.DistanceSq(candidate, spot));
				}

				if (minDistSq > bestMinDistSq)
				{
					bestMinDistSq = minDistSq;
					best = c;
				}
			}
			spots.Insert(m_aPositions[best]);
			m_aPositions.Remove(best);
		}

		// More soldiers than spots: the spots are shared
		for (int i = 0; i < agentCount; i++)
		{
			m_aGarrisonSpots.Insert(spots[i % spots.Count()]);
			m_aGarrisonBehaviors.Insert(null);
			SendToSpot(i);
		}

		PrintFormat("%1%2: %3 soldiers sent to %4 spots", LOG_PREFIX, Type(), agentCount, spots.Count());

		GetGame().GetCallqueue().CallLater(HoldStep, HOLD_STEP_MS, true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SendToSpot(int index)
	{
		SCR_AIUtilityComponent utility = GetUtility(m_aGarrisonAgents[index]);
		if (!utility)
			return;

		SCR_AIMoveIndividuallyBehavior behavior = new SCR_AIMoveIndividuallyBehavior(utility, null, m_aGarrisonSpots[index], radius: SPOT_RADIUS);
		utility.AddAction(behavior);
		m_aGarrisonBehaviors[index] = behavior;
	}

	//------------------------------------------------------------------------------------------------
	//! Runs every HOLD_STEP_MS: sends soldiers back who are away from their spot and not busy fighting
	protected void HoldStep()
	{
		if (!m_bRunning)
			return;

		foreach (int i, AIAgent agent : m_aGarrisonAgents)
		{
			SCR_AIUtilityComponent utility = GetUtility(agent);
			if (!utility)
				continue;

			SCR_AIMoveIndividuallyBehavior behavior = m_aGarrisonBehaviors[i];
			if (behavior)
			{
				EAIActionState state = behavior.GetActionState();
				if (state != EAIActionState.COMPLETED && state != EAIActionState.FAILED)
					continue;
			}

			if (utility.m_CombatComponent && utility.m_CombatComponent.GetCurrentTarget())
				continue;

			if (vector.DistanceSq(utility.GetOrigin(), m_aGarrisonSpots[i]) > HOLD_RADIUS_SQ)
				SendToSpot(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void Stop()
	{
		if (m_bRunning)
		{
			ArmaReforgerScripted game = GetGame();
			if (game && game.GetCallqueue())
				game.GetCallqueue().Remove(HoldStep);

			foreach (SCR_AIMoveIndividuallyBehavior behavior : m_aGarrisonBehaviors)
			{
				if (!behavior)
					continue;

				EAIActionState state = behavior.GetActionState();
				if (state != EAIActionState.COMPLETED && state != EAIActionState.FAILED)
					behavior.Fail();
			}

			m_aGarrisonBehaviors.Clear();
			m_aGarrisonAgents.Clear();
			m_aGarrisonSpots.Clear();
		}

		super.Stop();
	}
}
