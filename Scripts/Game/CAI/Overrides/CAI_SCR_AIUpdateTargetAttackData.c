//! Chooses how a soldier fires at his target: bursts, single shots, suppressive fire or not at all
modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{
	//------------------------------------------------------------------------------------------------
	//! Vanilla lets rifles fire bursts only within BURST_FIRE_MAX_DISTANCE (50 m) and single shots beyond.
	//! Machine guns always fire bursts and are not affected.
	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		int fireTree = super.ResolveFireTree(target, visible, weaponReady, fireRate);

		if (fireTree != FIRE_TREE_SINGLE || !m_bWeaponHasBurstOrAuto || !m_CombatComponent || !target)
			return fireTree;

		if (target.GetDistance() < m_CombatComponent.CAI_GetSettings().m_fAutoFireRange)
			return FIRE_TREE_BURST;

		return fireTree;
	}
}
