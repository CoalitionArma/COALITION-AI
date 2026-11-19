//! Scales how much suppression a soldier takes from incoming fire.
//! The scale is pushed in by SCR_AICombatComponent.CAI_Apply whenever the soldier's settings change.
modded class SCR_AIThreatSystem
{
	protected float m_fCAI_SuppressionScale = 1;
	protected bool m_bCAI_CapThreat;

	protected static const float CAI_GRENADE_THREAT_CAP = 0.5; //!< Below vanilla's limit of 0.6 for throwing frag grenades

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

	//------------------------------------------------------------------------------------------------
	//! Vanilla forbids frag grenades to a soldier whose threat is above 0.6. The combat component switches this on
	//! only while it selects weapon and target, so that check sees a lower threat and everything else the real one.
	void CAI_SetGrenadeThreatCap(bool enabled)
	{
		m_bCAI_CapThreat = enabled;
	}

	//------------------------------------------------------------------------------------------------
	override float GetThreatMeasure()
	{
		float threat = super.GetThreatMeasure();

		if (m_bCAI_CapThreat && threat > CAI_GRENADE_THREAT_CAP)
			return CAI_GRENADE_THREAT_CAP;

		return threat;
	}
}
