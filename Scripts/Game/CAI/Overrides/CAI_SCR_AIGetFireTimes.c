//! Timing of AI shots and bursts
modded class SCR_AIGetFireTimes : AITaskScripted
{
	protected bool m_bCAI_BurstOverridden;

	//------------------------------------------------------------------------------------------------
	//! Burst length. Vanilla computes how long to hold the trigger from m_fBurstSize when that is above 1 (weapons
	//! with a fixed burst), and from a short random value otherwise. The member is only refreshed when the weapon or
	//! fire mode changes, so setting it here decides the length of the next burst. Single-shot fire ignores it.
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		ENodeResult result = super.EOnTaskSimulate(owner, dt);

		if (result != ENodeResult.SUCCESS || !m_CombatComponent)
			return result;

		CAI_Settings settings = m_CombatComponent.CAI_GetSettings();
		int maxRounds;
		if (m_eLastWeaponType == EWeaponType.WT_MACHINEGUN)
			maxRounds = settings.m_iMachineGunBurstRounds;
		else if (m_eLastWeaponType == EWeaponType.WT_RIFLE)
			maxRounds = settings.m_iRifleBurstRounds;

		if (maxRounds > 1)
		{
			int minRounds = Math.Max(Math.Round(0.6 * maxRounds), 2);
			m_fBurstSize = Math.RandomIntInclusive(minRounds, Math.Max(maxRounds, minRounds));
			m_bCAI_BurstOverridden = true;
		}
		else if (m_bCAI_BurstOverridden)
		{
			// Setting was switched off, back to what the fire mode says
			if (m_LastFireMode)
				m_fBurstSize = m_LastFireMode.GetBurstSize();

			m_bCAI_BurstOverridden = false;
		}

		return result;
	}
	//------------------------------------------------------------------------------------------------
	//! Vanilla returns 1 - fireRateCoef, where the coef comes from the group or from SetFireRateCoef.
	//! Our tempo multiplies that coef, so group orders such as saving ammo while suppressing still count.
	override float GetFireRateFactor()
	{
		float factor = super.GetFireRateFactor();

		if (!m_CombatComponent)
			return factor;

		float tempo = m_CombatComponent.CAI_GetSettings().m_fFireTempo;

		// Search and Destroy assault: fastest fire vanilla allows
		if (m_CombatComponent.CAI_IsAssaulting())
			tempo = Math.Max(tempo, MAX_FIRE_RATE_COEF);

		if (tempo == CAI_Settings.VANILLA_FIRE_TEMPO)
			return factor;

		float fireRateCoef = (1 - factor) * tempo;
		return 1 - Math.Clamp(fireRateCoef, MIN_FIRE_RATE_COEF, MAX_FIRE_RATE_COEF);
	}
}
