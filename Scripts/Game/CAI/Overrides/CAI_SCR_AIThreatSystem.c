//! Scales how much suppression a soldier takes from incoming fire.
//! The scale is pushed in by SCR_AICombatComponent.CAI_Apply whenever the soldier's settings change.
modded class SCR_AIThreatSystem
{
	protected float m_fCAI_SuppressionScale = 1;

	//------------------------------------------------------------------------------------------------
	void CAI_SetSuppressionScale(float scale)
	{
		m_fCAI_SuppressionScale = Math.Max(scale, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla counts suppression in whole bullets, so the fractional part is applied as a chance
	protected int CAI_ScaleCount(int count)
	{
		if (m_fCAI_SuppressionScale == 1)
			return count;

		float scaled = count * m_fCAI_SuppressionScale;
		int whole = Math.Floor(scaled);
		if (Math.RandomFloat01() < scaled - whole)
			whole++;

		return whole;
	}

	//------------------------------------------------------------------------------------------------
	override void ThreatBulletImpact(int count)
	{
		count = CAI_ScaleCount(count);
		if (count > 0)
			super.ThreatBulletImpact(count);
	}

	//------------------------------------------------------------------------------------------------
	override void ThreatProjectileFlyby(int count)
	{
		count = CAI_ScaleCount(count);
		if (count > 0)
			super.ThreatProjectileFlyby(count);
	}

	//------------------------------------------------------------------------------------------------
	override void ThreatExplosion(float distance)
	{
		// The increment is computed inside vanilla, so a scale below 1 becomes a chance to shrug the explosion off
		if (m_fCAI_SuppressionScale < 1 && Math.RandomFloat01() >= m_fCAI_SuppressionScale)
			return;

		super.ThreatExplosion(distance);

		// A scale above 1 becomes a chance to count it twice
		if (m_fCAI_SuppressionScale > 1 && Math.RandomFloat01() < m_fCAI_SuppressionScale - 1)
			super.ThreatExplosion(distance);
	}
}
