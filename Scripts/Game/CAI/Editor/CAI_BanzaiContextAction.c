//! Game Master right-click command: every selected AI group charges the clicked position.
//! Listed in Configs/Editor/ActionLists/Context/TempEdit.conf, where it is set to run on the server.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class CAI_BanzaiContextAction : SCR_SelectedEntitiesContextAction
{
	//------------------------------------------------------------------------------------------------
	//! \return The AI group of a selected group or of a selected AI character
	protected SCR_AIGroup GetGroup(SCR_EditableEntityComponent selectedEntity)
	{
		if (!selectedEntity)
			return null;

		EEditableEntityType type = selectedEntity.GetEntityType();
		if (type == EEditableEntityType.CHARACTER)
		{
			if (selectedEntity.HasEntityState(EEditableEntityState.PLAYER))
				return null;

			selectedEntity = selectedEntity.GetAIGroup();
			if (!selectedEntity)
				return null;
		}
		else if (type != EEditableEntityType.GROUP)
		{
			return null;
		}

		return SCR_AIGroup.Cast(selectedEntity.GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetGroup(selectedEntity) != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetGroup(selectedEntity) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Runs once per selected entity, so several selected soldiers of one group restart the same charge
	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		SCR_AIGroup group = GetGroup(selectedEntity);
		if (!group)
			return;

		// Already charging at this position
		if (group.CAI_IsBanzai() && vector.DistanceSqXZ(group.CAI_GetBanzaiPosition(), cursorWorldPosition) < 1)
			return;

		group.CAI_StartBanzai(cursorWorldPosition);
	}
}
