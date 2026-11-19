//! Aiming error of AI soldiers
modded class SCR_AIGetAimErrorOffset : AITaskScripted
{
	//------------------------------------------------------------------------------------------------
	//! Random spread of the aim point, vanilla derives it from skill only
	override float GetRandomFactor(EAISkill skill, float mu)
	{
		float factor = super.GetRandomFactor(skill, mu);

		if (m_CombatComponent)
			factor *= m_CombatComponent.CAI_GetSettings().m_fAimErrorScale;

		return factor;
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla doubles the error against poorly lit targets
	override float GetTargetIlluminationFactor(BaseTarget tgt)
	{
		float factor = super.GetTargetIlluminationFactor(tgt);

		if (factor > 1 && m_CombatComponent)
			return m_CombatComponent.CAI_GetSettings().m_fNightAimPenalty;

		return factor;
	}
}
