modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	//------------------------------------------------------------------------------------------------
	//! A charging group doesn't stop to lay suppressive fire on a cluster of enemies
	override protected bool ShouldSuppressCluster(SCR_AITargetClusterState clusterState)
	{
		if (m_Owner && m_Owner.CAI_IsBanzai())
			return false;

		return super.ShouldSuppressCluster(clusterState);
	}
}
