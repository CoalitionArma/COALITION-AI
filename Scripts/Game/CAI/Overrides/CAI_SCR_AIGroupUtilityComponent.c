modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	//------------------------------------------------------------------------------------------------
	//! A group which is charging or falling back doesn't stop to lay suppressive fire on a cluster of enemies
	override protected bool ShouldSuppressCluster(SCR_AITargetClusterState clusterState)
	{
		if (m_Owner && (m_Owner.CAI_IsBanzai() || m_Owner.CAI_IsFallingBack()))
			return false;

		return super.ShouldSuppressCluster(clusterState);
	}
}
