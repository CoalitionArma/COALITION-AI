//! Combat movement of a soldier who is attacking a target.
//! During a banzai charge (SCR_AIGroup.CAI_StartBanzai) vanilla's cover-to-cover logic is replaced by a straight run
//! at the charge position, aiming at the target, so the attack tree keeps firing while the soldier moves.
modded class SCR_AICombatMoveLogic_Attack : SCR_AICombatMoveLogicBase
{
	protected static const float CAI_BANZAI_ARRIVAL_DIST_SQ = 100;	//!< Within 10 m of the position the soldier fights normally
	protected static const float CAI_BANZAI_MOVE_DURATION_S = 8;	//!< Length of one leg of the run, a new one starts when it ends

	//------------------------------------------------------------------------------------------------
	override protected ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		// Same throttle as vanilla (0.5 s), checked first so nothing below runs every tick
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;

		vector banzaiPos;
		if (!m_CombatComp || !m_MyEntity || !m_CombatComp.CAI_GetBanzaiPosition(banzaiPos))
			return super.EOnTaskSimulate(owner, dt);

		if (vector.DistanceSqXZ(m_MyEntity.GetOrigin(), banzaiPos) < CAI_BANZAI_ARRIVAL_DIST_SQ)
			return super.EOnTaskSimulate(owner, dt);

		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;

		if (!OnUpdate(owner, dt))
			return ENodeResult.FAIL;

		if (!m_State || !m_Utility || !m_CharacterController)
			return ENodeResult.FAIL;

		// Combat movement is not used in turrets and vehicles
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;

		// Already running at the position
		if (m_State.IsExecutingRequest())
		{
			SCR_AICombatMoveRequest_Move currentRequest = SCR_AICombatMoveRequest_Move.Cast(m_State.GetRequest());
			if (currentRequest && !currentRequest.m_bTryFindCover && currentRequest.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS
				&& vector.DistanceSqXZ(currentRequest.m_vMovePos, banzaiPos) < 1)
				return ENodeResult.RUNNING;
		}

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = banzaiPos;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_bTryFindCover = false;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.STAND;
		rq.m_eMovementType = EMovementType.RUN; // Sprinting soldiers can't aim
		rq.m_fMoveDuration_s = CAI_BANZAI_MOVE_DURATION_S;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType)
			&& IsAimingAndMovingAllowedForWeapon(m_CombatComp.GetSelectedWeaponType());
		rq.m_bAimAtTargetEnd = true;

		m_State.ApplyNewRequest(rq);

		return ENodeResult.RUNNING;
	}
}
