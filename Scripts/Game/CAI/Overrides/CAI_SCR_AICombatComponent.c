modded class SCR_AICombatComponent : ScriptComponent
{
	protected ref CAI_Settings m_CAI_Settings;	//!< Settings this soldier currently uses, see CAI_Refresh
	protected int m_iCAI_Revision = -1;			//!< CAI_SettingsManager revision m_CAI_Settings was resolved at
	protected AIGroup m_CAI_Group;				//!< Group m_CAI_Settings was resolved for
	protected bool m_bCAI_SkillOverridden;		//!< True while our skill override is applied

	//------------------------------------------------------------------------------------------------
	//! Settings this soldier currently uses. Never returns null.
	//! The cache is refreshed once per behavior evaluation, not here.
	CAI_Settings CAI_GetSettings()
	{
		if (!m_CAI_Settings)
			return CAI_SettingsManager.GetVanilla();

		return m_CAI_Settings;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true while this soldier's group is in a banzai charge (SCR_AIGroup.CAI_StartBanzai)
	bool CAI_GetBanzaiPosition(out vector outPosition)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(m_CAI_Group);
		if (!group || !group.CAI_IsBanzai())
			return false;

		outPosition = group.CAI_GetBanzaiPosition();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Looks the settings up again if they were edited or this soldier changed group
	protected void CAI_Refresh(SCR_AIThreatSystem threatSystem)
	{
		if (!m_Agent)
		{
			AIControlComponent controlComp = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
			if (controlComp)
				m_Agent = SCR_ChimeraAIAgent.Cast(controlComp.GetAIAgent());
		}

		AIGroup group;
		if (m_Agent)
			group = m_Agent.GetParentGroup();

		int revision = CAI_SettingsManager.GetRevision();
		if (m_CAI_Settings && revision == m_iCAI_Revision && group == m_CAI_Group)
			return;

		m_iCAI_Revision = revision;
		m_CAI_Group = group;

		CAI_Settings groupOverride;
		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (scrGroup)
			groupOverride = scrGroup.CAI_GetOverride();

		m_CAI_Settings = CAI_SettingsManager.Resolve(groupOverride);

		CAI_Apply(threatSystem);
	}

	//------------------------------------------------------------------------------------------------
	//! Pushes settings into the systems that keep their own copy of a value
	protected void CAI_Apply(SCR_AIThreatSystem threatSystem)
	{
		// Skill. Only reset it if we were the ones who changed it, scenarios and other mods set skill too.
		EAISkill skill = m_CAI_Settings.m_eSkillOverride;
		if (skill != EAISkill.NONE)
		{
			SetAISkill(skill);
			m_bCAI_SkillOverridden = true;
		}
		else if (m_bCAI_SkillOverridden)
		{
			ResetAISkill();
			m_bCAI_SkillOverridden = false;
		}

		if (threatSystem)
		{
			// A soldier in a banzai charge can't be suppressed. Starting and ending a charge invalidates the settings, which gets us here.
			vector banzaiPos;
			if (CAI_GetBanzaiPosition(banzaiPos))
				threatSystem.CAI_SetSuppressionScale(0);
			else
				threatSystem.CAI_SetSuppressionScale(m_CAI_Settings.m_fSuppressionScale);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Called by SCR_AIUtilityComponent at every behavior evaluation
	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		CAI_Refresh(threatSystem);

		float scale = m_CAI_Settings.m_fPerceptionScale;
		float suppressedFactor = m_CAI_Settings.m_fSuppressedPerception;

		// Nothing to change, leave it to vanilla
		if (scale == CAI_Settings.VANILLA_PERCEPTION && suppressedFactor == CAI_Settings.VANILLA_SUPPRESSED_PERCEPTION)
		{
			super.UpdatePerceptionFactor(perceptionComp, threatSystem);
			return;
		}

		// Same as vanilla, except for the configurable THREATENED factor and the overall scale
		float perceptionFactor;
		switch (threatSystem.GetState())
		{
			case EAIThreatState.SAFE:
				perceptionFactor = PERCEPTION_FACTOR_SAFE; break;
			case EAIThreatState.VIGILANT:
				perceptionFactor = PERCEPTION_FACTOR_VIGILANT; break;
			case EAIThreatState.ALERTED:
				perceptionFactor = PERCEPTION_FACTOR_ALERTED; break;
			case EAIThreatState.THREATENED:
				perceptionFactor = suppressedFactor; break;
		}

		perceptionFactor *= m_fEquipmentPerceptionFactor;
		perceptionFactor *= m_fPerceptionFactor;
		perceptionFactor *= scale;

		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla re-evaluates target and weapon every 0.5 s. Pull the next evaluation closer if settings ask for it.
	override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
		out bool outRetreatTargetChanged, out bool outCompartmentChanged)
	{
		super.EvaluateWeaponAndTarget(outWeaponEvent, outSelectedTargetChanged, outPrevTarget, outCurrentTarget, outRetreatTargetChanged, outCompartmentChanged);

		if (!m_CAI_Settings)
			return;

		float interval_ms = 1000 * m_CAI_Settings.m_fTargetEvalInterval;
		if (interval_ms >= WEAPON_TARGET_UPDATE_PERIOD_MS)
			return;

		float nextEvaluation_ms = GetGame().GetWorld().GetWorldTime() + Math.Max(interval_ms, 100);
		if (nextEvaluation_ms < m_fNextWeaponTargetEvaluation_ms)
			m_fNextWeaponTargetEvaluation_ms = nextEvaluation_ms;
	}
}
