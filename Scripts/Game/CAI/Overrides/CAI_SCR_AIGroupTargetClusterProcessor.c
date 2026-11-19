//! What a group does about a cluster of enemies it found, and how quickly it decides
modded class SCR_AIGroupTargetClusterProcessor : Managed
{
	//------------------------------------------------------------------------------------------------
	//! Vanilla waits INITIAL_DECISION_DELAY_MS (5 s) in the NONE state before it attacks or investigates.
	//! The delay is a constant, so the cluster's timer is run faster or slower instead.
	override void UpdateCluster(SCR_AIGroupTargetCluster c, SCR_AITargetClusterState s, float deltaTime_ms)
	{
		if (s && s.m_eState == EAITargetClusterState.NONE && m_Utility && m_Utility.m_Owner)
		{
			float delay_s = m_Utility.m_Owner.CAI_GetSettings().m_fDecisionDelay;

			// Search and Destroy assault: commit to contacts at once
			if (m_Utility.m_Owner.CAI_IsAssaulting())
				delay_s = 0;

			if (delay_s != CAI_Settings.VANILLA_DECISION_DELAY)
				deltaTime_ms *= CAI_Settings.VANILLA_DECISION_DELAY / Math.Max(delay_s, 0.05);
		}

		super.UpdateCluster(c, s, deltaTime_ms);
	}

	//------------------------------------------------------------------------------------------------
	//! A charging group keeps everyone on the charge, no fireteams are sent to attack or investigate clusters
	override EAITargetClusterState EvaluateNewDesiredState(SCR_AITargetClusterState s)
	{
		if (m_Utility && m_Utility.m_Owner && m_Utility.m_Owner.CAI_IsBanzai())
			return EAITargetClusterState.NONE;

		return super.EvaluateNewDesiredState(s);
	}
}
