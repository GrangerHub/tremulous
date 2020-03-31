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
	playerState_t			*cent = &(cg.snap->ps);
	weapon_t      weaponNum = cg.snap->ps.weapon;

	qboolean lcannon =
			(weaponNum == WP_LUCIFER_CANNON && cent->stats[ STAT_MISC ] > 0);

	qboolean lcannonWarn =
			( weaponNum == WP_LUCIFER_CANNON &&
      ( cent->eFlags & EF_WARN_CHARGE ) &&
      cent->stats[ STAT_TEAM ] != TEAM_ALIENS );

	qboolean tyrantSprint =
			( weaponNum == WP_ALEVEL4 && cent->stats[ STAT_MISC ] > 0
			&& cent->stats[ STAT_STATE ] & SS_CHARGING );

	CG_UpdateHapticState(CG_FB_EFFECT_LCANNON, lcannon);
	CG_UpdateHapticState(CG_FB_EFFECT_LCANNONWARN, lcannonWarn);
	CG_UpdateHapticState(CG_FB_EFFECT_TYRANTTRAMPLE, tyrantSprint);
	// lisk poisoned
	// mara electized (decreschendo) / tesla
	// rant sprint
	// hive
}
