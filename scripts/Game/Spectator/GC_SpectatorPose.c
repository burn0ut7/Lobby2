//! Local animation-graph presentation for one observed character. This never
//! sends character input, changes possession, moves the entity or runs actions.
class GC_SpectatorPose
{
	protected ChimeraCharacter m_Character;
	protected CharacterAnimationComponent m_Animation;
	protected TAnimGraphVariable m_iStance = -1;
	protected TAnimGraphVariable m_iLean = -1;
	protected TAnimGraphVariable m_iADS = -1;
	protected TAnimGraphVariable m_iLow = -1;

	//------------------------------------------------------------------------------------------------
	bool Attach(ChimeraCharacter character)
	{
		if (!character || !character.GetCharacterController())
			return false;
		CharacterAnimationComponent animation = character.GetCharacterController().GetAnimationComponent();
		if (!animation)
			return false;

		m_iStance = animation.BindVariableInt("Stance");
		m_iLean = animation.BindVariableFloat("Lean");
		m_iADS = animation.BindVariableBool("ADS");
		m_iLow = animation.BindVariableBool("Low");
		if (m_iStance < 0 || m_iLean < 0 || m_iADS < 0 || m_iLow < 0)
			return false;

		m_Character = character;
		m_Animation = animation;
		m_Animation.SetAnimationLayerFPP();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Apply(int stance, float lean, bool ads, bool raised)
	{
		if (!m_Character || !m_Animation)
			return;

		// Shared variables reach the weapon graph as well as the character graph.
		// Ordinary graph setters can be overwritten by the native command update.
		m_Animation.SetSharedVariableInt(m_iStance, stance, true);
		m_Animation.SetSharedVariableFloat(m_iLean, lean, true);
		m_Animation.SetSharedVariableBool(m_iADS, ads, true);
		m_Animation.SetSharedVariableBool(m_iLow, !raised && !ads, true);
	}

	//------------------------------------------------------------------------------------------------
	void Detach()
	{
		if (m_Character && m_Animation)
		{
			CharacterControllerComponent controller = m_Character.GetCharacterController();
			if (controller && controller.GetInputContext())
				Apply(controller.GetStance(), controller.GetInputContext().GetLean(), controller.IsWeaponADS(), controller.IsWeaponRaised());
			m_Animation.SetAnimationLayerTPP();
		}
		m_Animation = null;
		m_Character = null;
	}
}
