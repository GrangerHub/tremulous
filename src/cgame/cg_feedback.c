// cg_feedback.c  -- send any game feedback to the user (rumble, keyboard color, ....)

#include "cg_local.h"

typedef struct
{
	qboolean active;
} hapticeffect_t;

hapticeffect_t hapticEffects[CG_FB_EFFECT_COUNT];

static void CG_UpdateHapticState(int id, qboolean active)
{
	if (hapticEffects[id].active != active)
	{
		if (active)
			trap_IN_RunHapticEffect(id);
		else
			trap_IN_StopHapticEffect(id);

		hapticEffects[id].active = active;
	}
}

void CG_UpdateFeedbacks( void )
{
	static float		lastHealth;
	static qboolean	wasAlive;
	playerState_t		*cent = &(cg.predictedPlayerState);
	weapon_t				weaponNum = cg.snap->ps.weapon;
	qboolean 				alive, playing;
	qboolean				lcannon, lcannonWarn, tyrantTrample;
	qboolean				poisonClouded;
	int							health = cent->stats[ STAT_HEALTH ];
	float 					damage;

	playing = ( !cg.intermissionStarted && !(cent->pm_flags & PMF_FOLLOW) );
	alive = ( health > 0 && !(cent->eFlags & EF_DEAD) && playing);

	lcannon =
			( alive && weaponNum == WP_LUCIFER_CANNON &&
				cent->stats[ STAT_MISC ] > 0 );

	lcannonWarn =
			( alive && weaponNum == WP_LUCIFER_CANNON &&
      ( cent->eFlags & EF_WARN_CHARGE ) &&
      cent->stats[ STAT_TEAM ] != TEAM_ALIENS );

	tyrantTrample =
			( alive && weaponNum == WP_ALEVEL4 &&
				cent->stats[ STAT_MISC ] > 0 &&
				cent->stats[ STAT_STATE ] & SS_CHARGING );

	poisonClouded =
			( alive && cent->eFlags & EF_POISONCLOUDED );

	CG_UpdateHapticState(CG_FB_EFFECT_LCANNON, lcannon);
	CG_UpdateHapticState(CG_FB_EFFECT_LCANNONWARN, lcannonWarn);
	CG_UpdateHapticState(CG_FB_EFFECT_TYRANTTRAMPLE, tyrantTrample);
	CG_UpdateHapticState(CG_FB_EFFECT_POISONED, poisonClouded);

	if ( health != lastHealth )
	{
		if ( alive && health != cent->stats[STAT_MAX_HEALTH] && health < lastHealth)
		{
			damage = (float)(lastHealth - health) / cent->stats[STAT_MAX_HEALTH];

			if ( damage <= 0.055 )
				trap_IN_RunHapticEffect(CG_FB_EFFECT_SMALLDAMAGE);
			else if ( damage <= 0.355 )
				trap_IN_RunHapticEffect(CG_FB_EFFECT_DAMAGE);
			else
				trap_IN_RunHapticEffect(CG_FB_EFFECT_CRITICALDAMAGE);
		}
		lastHealth = health;
	}
	if (wasAlive != alive)
	{
		if (wasAlive && !alive && playing)
			trap_IN_RunHapticEffect(CG_FB_EFFECT_DEAD);
		wasAlive = alive;
	}
}
