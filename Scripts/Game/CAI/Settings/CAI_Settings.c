//! Identifies one tunable of CAI_Settings. Zeus attributes read and write settings through these.
enum ECAI_Setting
{
	ENABLED,
	SKILL,
	AIM_ERROR,
	NIGHT_AIM_PENALTY,
	FIRE_TEMPO,
	PERCEPTION,
	SUPPRESSED_PERCEPTION,
	SUPPRESSION,
	TARGET_EVAL_INTERVAL,
	DECISION_DELAY,
	SHARE_TARGETS,
	SHARE_RANGE,
	SHARE_INTERVAL,
	ARTILLERY,
	ARTILLERY_RANGE,
	ARTILLERY_ROUNDS,
	ARTILLERY_COOLDOWN,
	ARTILLERY_SAFE_DISTANCE,
	ARTILLERY_MIN_TARGETS,
	HUNT,
	HUNT_RADIUS,
	ASSAULT_SND,
	LAUNCHER_CHANCE,
	AUTO_FIRE_RANGE,
	RIFLE_BURST,
	MG_BURST
}

//------------------------------------------------------------------------------------------------
//! One set of AI tuning values.
//! Used for the mission-wide settings (CAI_SettingsComponent on the game mode) and for per-group overrides (SCR_AIGroup).
//! Attribute defaults only apply when the object is read from a prefab or config, so script-created
//! instances must come from CreateDefault() or CreateVanilla(). Keep SetDefaults() in sync with the attribute defaults.
[BaseContainerProps()]
class CAI_Settings
{
	[Attribute("1", UIWidgets.CheckBox, "Master switch. When off, AI behaves exactly like vanilla.")]
	bool m_bEnabled;

	[Attribute("0", UIWidgets.ComboBox, "Forces this combat skill on AI. NONE keeps the skill each unit prefab already has.", "", ParamEnumArray.FromEnum(EAISkill))]
	EAISkill m_eSkillOverride;

	[Attribute("0.6", UIWidgets.Slider, "Multiplier of the random aiming error. Lower is more accurate. 1 = vanilla, 0 = no error at all.", "0 2 0.05")]
	float m_fAimErrorScale;

	[Attribute("1.5", UIWidgets.Slider, "Aiming error multiplier against poorly lit targets. 2 = vanilla, 1 = darkness doesn't hurt accuracy.", "1 3 0.1")]
	float m_fNightAimPenalty;

	[Attribute("1.35", UIWidgets.Slider, "How quickly AI fires. Higher means less time settling the aim between shots and bursts. 1 = vanilla.", "0.25 2 0.05")]
	float m_fFireTempo;

	[Attribute("1.5", UIWidgets.Slider, "Multiplier of how fast AI visually detects and identifies targets. 1 = vanilla.", "0.25 4 0.05")]
	float m_fPerceptionScale;

	[Attribute("0.7", UIWidgets.Slider, "Detection ability while suppressed. 0.4 = vanilla (suppressed AI is nearly blind), 1 = same as when calm.", "0.1 2.5 0.05")]
	float m_fSuppressedPerception;

	[Attribute("0.6", UIWidgets.Slider, "Multiplier of suppression received from bullet impacts, fly-bys and explosions. Lower keeps AI fighting under fire. 1 = vanilla.", "0 2 0.05")]
	float m_fSuppressionScale;

	[Attribute("0.3", UIWidgets.Slider, "Seconds between re-evaluations of the best target and weapon. 0.5 = vanilla. Lower reacts faster but costs more server time.", "0.1 0.5 0.05")]
	float m_fTargetEvalInterval;

	[Attribute("2", UIWidgets.Slider, "Seconds a group waits after first contact before it commits fireteams to attack or investigate. 5 = vanilla.", "0 10 0.5")]
	float m_fDecisionDelay;

	[Attribute("1", UIWidgets.CheckBox, "Groups report identified enemies to nearby friendly groups, which then move to investigate and join the fight.")]
	bool m_bShareTargets;

	[Attribute("500", UIWidgets.Slider, "Friendly groups within this distance (metres) receive contact reports.", "50 2000 50")]
	float m_fShareRange;

	[Attribute("8", UIWidgets.Slider, "Seconds between contact reports from a group that is in contact.", "2 60 1")]
	float m_fShareInterval;

	[Attribute("1", UIWidgets.CheckBox, "Groups crewing mortars or other static artillery answer contact reports with a fire mission on the reported enemies. Needs contact reports.")]
	bool m_bArtillerySupport;

	[Attribute("2000", UIWidgets.Slider, "Artillery only fires at reported enemies within this distance (metres) of the artillery group.", "200 5000 100")]
	float m_fArtilleryRange;

	[Attribute("4", UIWidgets.Slider, "Rounds fired per fire mission.", "1 20 1")]
	int m_iArtilleryRounds;

	[Attribute("120", UIWidgets.Slider, "Seconds an artillery group waits between fire missions. A mission still running after this time is called off.", "10 600 10")]
	float m_fArtilleryCooldown;

	[Attribute("150", UIWidgets.Slider, "No fire mission if a friendly AI group or friendly player is within this distance (metres) of the target.", "50 500 10")]
	float m_fArtillerySafeDistance;

	[Attribute("3", UIWidgets.Slider, "A fire mission needs at least this many identified enemies in the reported cluster.", "1 10 1")]
	int m_iArtilleryMinTargets;

	[Attribute("0", UIWidgets.CheckBox, "Groups without waypoints keep moving to search and destroy the nearest enemy player, wherever that player is. They know the position without having seen the player.")]
	bool m_bHunt;

	[Attribute("1000", UIWidgets.Slider, "Hunting groups only go after enemy players within this distance (metres).", "100 5000 100")]
	float m_fHuntRadius;

	[Attribute("1", UIWidgets.CheckBox, "While a group's current waypoint is Search and Destroy it assaults all-out: charges into the waypoint area firing on the move, can't be suppressed, fires at full tempo, commits to contacts without delay and pushes at targets instead of waiting in cover. Hunting groups use this waypoint too.")]
	bool m_bAssaultOnSearchAndDestroy;

	[Attribute("50", UIWidgets.Slider, "Chance in percent that a soldier carrying a rocket launcher uses it against an infantry target he can see, decided once per target. 0 = vanilla, launchers are kept for vehicles.", "0 100 5")]
	int m_iLauncherVsInfantryChance;

	[Attribute("150", UIWidgets.Slider, "Rifles which can fire automatic or burst do so against visible targets within this distance (metres), and fire single shots beyond it. 50 = vanilla.", "50 400 10")]
	float m_fAutoFireRange;

	[Attribute("6", UIWidgets.Slider, "Longest automatic burst of a rifle, in rounds. Each burst is 60 to 100 percent of this. 0 = vanilla, about 1 to 4 rounds.", "0 15 1")]
	int m_iRifleBurstRounds;

	[Attribute("15", UIWidgets.Slider, "Longest burst of a machine gun or automatic rifle such as the M249 or RPK, in rounds. Each burst is 60 to 100 percent of this. 0 = vanilla, about 2 to 7 rounds.", "0 40 1")]
	int m_iMachineGunBurstRounds;

	// Values of the unmodified game, used when the mod is disabled
	static const float VANILLA_AIM_ERROR = 1;
	static const float VANILLA_NIGHT_AIM_PENALTY = 2;
	static const float VANILLA_FIRE_TEMPO = 1;
	static const float VANILLA_PERCEPTION = 1;
	static const float VANILLA_SUPPRESSED_PERCEPTION = 0.4;
	static const float VANILLA_SUPPRESSION = 1;
	static const float VANILLA_TARGET_EVAL_INTERVAL = 0.5;
	static const float VANILLA_DECISION_DELAY = 5;
	static const float VANILLA_AUTO_FIRE_RANGE = 50;

	//------------------------------------------------------------------------------------------------
	//! The mod's own tuning. Must match the attribute defaults above.
	void SetDefaults()
	{
		m_bEnabled = true;
		m_eSkillOverride = EAISkill.NONE;
		m_fAimErrorScale = 0.6;
		m_fNightAimPenalty = 1.5;
		m_fFireTempo = 1.35;
		m_fPerceptionScale = 1.5;
		m_fSuppressedPerception = 0.7;
		m_fSuppressionScale = 0.6;
		m_fTargetEvalInterval = 0.3;
		m_fDecisionDelay = 2;
		m_bShareTargets = true;
		m_fShareRange = 500;
		m_fShareInterval = 8;
		m_bArtillerySupport = true;
		m_bAssaultOnSearchAndDestroy = true;
		m_iLauncherVsInfantryChance = 50;
		m_fAutoFireRange = 150;
		m_iRifleBurstRounds = 6;
		m_iMachineGunBurstRounds = 15;
		m_fArtilleryRange = 2000;
		m_iArtilleryRounds = 4;
		m_fArtilleryCooldown = 120;
		m_fArtillerySafeDistance = 150;
		m_iArtilleryMinTargets = 3;
		m_bHunt = false;
		m_fHuntRadius = 1000;
	}

	//------------------------------------------------------------------------------------------------
	//! Values which make every hook of this mod behave like the unmodified game
	void SetVanilla()
	{
		m_bEnabled = false;
		m_eSkillOverride = EAISkill.NONE;
		m_fAimErrorScale = VANILLA_AIM_ERROR;
		m_fNightAimPenalty = VANILLA_NIGHT_AIM_PENALTY;
		m_fFireTempo = VANILLA_FIRE_TEMPO;
		m_fPerceptionScale = VANILLA_PERCEPTION;
		m_fSuppressedPerception = VANILLA_SUPPRESSED_PERCEPTION;
		m_fSuppressionScale = VANILLA_SUPPRESSION;
		m_fTargetEvalInterval = VANILLA_TARGET_EVAL_INTERVAL;
		m_fDecisionDelay = VANILLA_DECISION_DELAY;
		m_bShareTargets = false;
		m_fShareRange = 500;
		m_fShareInterval = 8;
		m_bArtillerySupport = false;
		m_bAssaultOnSearchAndDestroy = false;
		m_iLauncherVsInfantryChance = 0;
		m_fAutoFireRange = VANILLA_AUTO_FIRE_RANGE;
		m_iRifleBurstRounds = 0;
		m_iMachineGunBurstRounds = 0;
		m_fArtilleryRange = 2000;
		m_iArtilleryRounds = 4;
		m_fArtilleryCooldown = 120;
		m_fArtillerySafeDistance = 150;
		m_iArtilleryMinTargets = 3;
		m_bHunt = false;
		m_fHuntRadius = 1000;
	}

	//------------------------------------------------------------------------------------------------
	static CAI_Settings CreateDefault()
	{
		CAI_Settings settings = new CAI_Settings();
		settings.SetDefaults();
		return settings;
	}

	//------------------------------------------------------------------------------------------------
	static CAI_Settings CreateVanilla()
	{
		CAI_Settings settings = new CAI_Settings();
		settings.SetVanilla();
		return settings;
	}

	//------------------------------------------------------------------------------------------------
	void CopyFrom(notnull CAI_Settings other)
	{
		m_bEnabled = other.m_bEnabled;
		m_eSkillOverride = other.m_eSkillOverride;
		m_fAimErrorScale = other.m_fAimErrorScale;
		m_fNightAimPenalty = other.m_fNightAimPenalty;
		m_fFireTempo = other.m_fFireTempo;
		m_fPerceptionScale = other.m_fPerceptionScale;
		m_fSuppressedPerception = other.m_fSuppressedPerception;
		m_fSuppressionScale = other.m_fSuppressionScale;
		m_fTargetEvalInterval = other.m_fTargetEvalInterval;
		m_fDecisionDelay = other.m_fDecisionDelay;
		m_bShareTargets = other.m_bShareTargets;
		m_fShareRange = other.m_fShareRange;
		m_fShareInterval = other.m_fShareInterval;
		m_bArtillerySupport = other.m_bArtillerySupport;
		m_fArtilleryRange = other.m_fArtilleryRange;
		m_iArtilleryRounds = other.m_iArtilleryRounds;
		m_fArtilleryCooldown = other.m_fArtilleryCooldown;
		m_fArtillerySafeDistance = other.m_fArtillerySafeDistance;
		m_iArtilleryMinTargets = other.m_iArtilleryMinTargets;
		m_bHunt = other.m_bHunt;
		m_fHuntRadius = other.m_fHuntRadius;
		m_bAssaultOnSearchAndDestroy = other.m_bAssaultOnSearchAndDestroy;
		m_iLauncherVsInfantryChance = other.m_iLauncherVsInfantryChance;
		m_fAutoFireRange = other.m_fAutoFireRange;
		m_iRifleBurstRounds = other.m_iRifleBurstRounds;
		m_iMachineGunBurstRounds = other.m_iMachineGunBurstRounds;
	}

	//------------------------------------------------------------------------------------------------
	//! Reads any setting as a float. Bools are 0 or 1, the skill is its enum value.
	float GetValue(ECAI_Setting setting)
	{
		switch (setting)
		{
			case ECAI_Setting.ENABLED:					return BoolToFloat(m_bEnabled);
			case ECAI_Setting.SKILL:					return m_eSkillOverride;
			case ECAI_Setting.AIM_ERROR:				return m_fAimErrorScale;
			case ECAI_Setting.NIGHT_AIM_PENALTY:		return m_fNightAimPenalty;
			case ECAI_Setting.FIRE_TEMPO:				return m_fFireTempo;
			case ECAI_Setting.PERCEPTION:				return m_fPerceptionScale;
			case ECAI_Setting.SUPPRESSED_PERCEPTION:	return m_fSuppressedPerception;
			case ECAI_Setting.SUPPRESSION:				return m_fSuppressionScale;
			case ECAI_Setting.TARGET_EVAL_INTERVAL:		return m_fTargetEvalInterval;
			case ECAI_Setting.DECISION_DELAY:			return m_fDecisionDelay;
			case ECAI_Setting.SHARE_TARGETS:			return BoolToFloat(m_bShareTargets);
			case ECAI_Setting.SHARE_RANGE:				return m_fShareRange;
			case ECAI_Setting.SHARE_INTERVAL:			return m_fShareInterval;
			case ECAI_Setting.ARTILLERY:				return BoolToFloat(m_bArtillerySupport);
			case ECAI_Setting.ARTILLERY_RANGE:			return m_fArtilleryRange;
			case ECAI_Setting.ARTILLERY_ROUNDS:			return m_iArtilleryRounds;
			case ECAI_Setting.ARTILLERY_COOLDOWN:		return m_fArtilleryCooldown;
			case ECAI_Setting.ARTILLERY_SAFE_DISTANCE:	return m_fArtillerySafeDistance;
			case ECAI_Setting.ARTILLERY_MIN_TARGETS:	return m_iArtilleryMinTargets;
			case ECAI_Setting.HUNT:						return BoolToFloat(m_bHunt);
			case ECAI_Setting.HUNT_RADIUS:				return m_fHuntRadius;
			case ECAI_Setting.ASSAULT_SND:				return BoolToFloat(m_bAssaultOnSearchAndDestroy);
			case ECAI_Setting.LAUNCHER_CHANCE:			return m_iLauncherVsInfantryChance;
			case ECAI_Setting.AUTO_FIRE_RANGE:			return m_fAutoFireRange;
			case ECAI_Setting.RIFLE_BURST:				return m_iRifleBurstRounds;
			case ECAI_Setting.MG_BURST:					return m_iMachineGunBurstRounds;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Writes any setting from a float, clamped to its valid range.
	//! Call CAI_SettingsManager.Invalidate() afterwards so AI picks the change up.
	void SetValue(ECAI_Setting setting, float value)
	{
		switch (setting)
		{
			case ECAI_Setting.ENABLED:					m_bEnabled = value != 0; break;
			case ECAI_Setting.SKILL:					m_eSkillOverride = SanitizeSkill(value); break;
			case ECAI_Setting.AIM_ERROR:				m_fAimErrorScale = Math.Clamp(value, 0, 2); break;
			case ECAI_Setting.NIGHT_AIM_PENALTY:		m_fNightAimPenalty = Math.Clamp(value, 1, 3); break;
			case ECAI_Setting.FIRE_TEMPO:				m_fFireTempo = Math.Clamp(value, 0.25, 2); break;
			case ECAI_Setting.PERCEPTION:				m_fPerceptionScale = Math.Clamp(value, 0.25, 4); break;
			case ECAI_Setting.SUPPRESSED_PERCEPTION:	m_fSuppressedPerception = Math.Clamp(value, 0.1, 2.5); break;
			case ECAI_Setting.SUPPRESSION:				m_fSuppressionScale = Math.Clamp(value, 0, 2); break;
			case ECAI_Setting.TARGET_EVAL_INTERVAL:		m_fTargetEvalInterval = Math.Clamp(value, 0.1, 0.5); break;
			case ECAI_Setting.DECISION_DELAY:			m_fDecisionDelay = Math.Clamp(value, 0, 10); break;
			case ECAI_Setting.SHARE_TARGETS:			m_bShareTargets = value != 0; break;
			case ECAI_Setting.SHARE_RANGE:				m_fShareRange = Math.Clamp(value, 50, 2000); break;
			case ECAI_Setting.SHARE_INTERVAL:			m_fShareInterval = Math.Clamp(value, 2, 60); break;
			case ECAI_Setting.ARTILLERY:				m_bArtillerySupport = value != 0; break;
			case ECAI_Setting.ARTILLERY_RANGE:			m_fArtilleryRange = Math.Clamp(value, 200, 5000); break;
			case ECAI_Setting.ARTILLERY_ROUNDS:			m_iArtilleryRounds = Math.Round(Math.Clamp(value, 1, 20)); break;
			case ECAI_Setting.ARTILLERY_COOLDOWN:		m_fArtilleryCooldown = Math.Clamp(value, 10, 600); break;
			case ECAI_Setting.ARTILLERY_SAFE_DISTANCE:	m_fArtillerySafeDistance = Math.Clamp(value, 50, 500); break;
			case ECAI_Setting.ARTILLERY_MIN_TARGETS:	m_iArtilleryMinTargets = Math.Round(Math.Clamp(value, 1, 10)); break;
			case ECAI_Setting.HUNT:						m_bHunt = value != 0; break;
			case ECAI_Setting.HUNT_RADIUS:				m_fHuntRadius = Math.Clamp(value, 100, 5000); break;
			case ECAI_Setting.ASSAULT_SND:				m_bAssaultOnSearchAndDestroy = value != 0; break;
			case ECAI_Setting.LAUNCHER_CHANCE:			m_iLauncherVsInfantryChance = Math.Round(Math.Clamp(value, 0, 100)); break;
			case ECAI_Setting.AUTO_FIRE_RANGE:			m_fAutoFireRange = Math.Clamp(value, 50, 400); break;
			case ECAI_Setting.RIFLE_BURST:				m_iRifleBurstRounds = Math.Round(Math.Clamp(value, 0, 15)); break;
			case ECAI_Setting.MG_BURST:					m_iMachineGunBurstRounds = Math.Round(Math.Clamp(value, 0, 40)); break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float BoolToFloat(bool value)
	{
		if (value)
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla's aiming code only knows the exact enum values, anything else would mean zero aiming error
	protected EAISkill SanitizeSkill(float value)
	{
		int skill = value;
		switch (skill)
		{
			case EAISkill.NOOB:
			case EAISkill.ROOKIE:
			case EAISkill.REGULAR:
			case EAISkill.VETERAN:
			case EAISkill.EXPERT:
			case EAISkill.CYLON:
				return skill;
		}

		return EAISkill.NONE;
	}
}

//------------------------------------------------------------------------------------------------
//! Finds the settings that apply to an AI and tells AI when they changed.
class CAI_SettingsManager
{
	protected static int s_iRevision = 1;
	protected static ref CAI_Settings s_Vanilla;
	protected static ref CAI_Settings s_Default;

	//------------------------------------------------------------------------------------------------
	//! AI caches its settings together with this number and looks them up again when it changes
	static int GetRevision()
	{
		return s_iRevision;
	}

	//------------------------------------------------------------------------------------------------
	//! Call after changing any settings object, or after adding or removing a group override
	static void Invalidate()
	{
		s_iRevision++;
	}

	//------------------------------------------------------------------------------------------------
	//! Read only! Shared by every AI the mod is disabled for.
	static CAI_Settings GetVanilla()
	{
		if (!s_Vanilla)
			s_Vanilla = CAI_Settings.CreateVanilla();

		return s_Vanilla;
	}

	//------------------------------------------------------------------------------------------------
	//! Mission-wide settings: the game mode's CAI_SettingsComponent, or the mod defaults when the mission has none
	static CAI_Settings GetGlobal()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			return gameMode.CAI_GetSettings();

		// World without a scripted game mode
		if (!s_Default)
			s_Default = CAI_Settings.CreateDefault();

		return s_Default;
	}

	//------------------------------------------------------------------------------------------------
	//! Settings an AI should actually use: its group's override if there is one, otherwise the mission-wide ones.
	//! Never returns null.
	static CAI_Settings Resolve(CAI_Settings groupOverride)
	{
		CAI_Settings settings = groupOverride;
		if (!settings)
			settings = GetGlobal();

		if (!settings || !settings.m_bEnabled)
			return GetVanilla();

		return settings;
	}
}
