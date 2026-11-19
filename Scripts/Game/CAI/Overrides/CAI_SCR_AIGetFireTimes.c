//! Timing of AI shots and bursts
modded class SCR_AIGetFireTimes : AITaskScripted
{
	//------------------------------------------------------------------------------------------------
	//! Vanilla returns 1 - fireRateCoef, where the coef comes from the group or from SetFireRateCoef.
	//! Our tempo multiplies that coef, so group orders such as saving ammo while suppressing still count.
	override float GetFireRateFactor()
	{
		float factor = super.GetFireRateFactor();

		if (!m_CombatComponent)
			return factor;

		float tempo = m_CombatComponent.CAI_GetSettings().m_fFireTempo;
		if (tempo == CAI_Settings.VANILLA_FIRE_TEMPO)
			return factor;

		float fireRateCoef = (1 - factor) * tempo;
		return 1 - Math.Clamp(fireRateCoef, MIN_FIRE_RATE_COEF, MAX_FIRE_RATE_COEF);
	}
}
