// g_turret.c

#include "g_local.h"

void infantry_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage);
void infantry_stand(edict_t *self);
void monster_use(edict_t *self, edict_t *other, edict_t *activator);
qboolean FindTarget(edict_t *self);
void SpawnTargetingSystem(edict_t *turret); /* FS: Coop: Rogue specific */

void
AnglesNormalize(vec3_t vec)
{
	while (vec[0] > 360)
	{
		vec[0] -= 360;
	}

	while (vec[0] < 0)
	{
		vec[0] += 360;
	}

	while (vec[1] > 360)
	{
		vec[1] -= 360;
	}

	while (vec[1] < 0)
	{
		vec[1] += 360;
	}
}

float
SnapToEights(float x)
{
	x *= 8.0;

	if (x > 0.0)
	{
		x += 0.5;
	}
	else
	{
		x -= 0.5;
	}

	return 0.125 * (int)x;
}

void
turret_blocked(edict_t *self, edict_t *other)
{
	edict_t *attacker;

	if (!self || !other)
	{
		return;
	}

	if (other->takedamage)
	{
		if (self->teammaster->owner)
		{
			attacker = self->teammaster->owner;
		}
		else
		{
			attacker = self->teammaster;
		}

		T_Damage(other, self, attacker, vec3_origin, other->s.origin,
				vec3_origin, self->teammaster->dmg, 10, 0, MOD_CRUSH);
	}
}

/*
 * QUAKED turret_breach (0 0 0) ?
 * This portion of the turret can change both pitch and yaw.
 * The model  should be made with a flat pitch.
 * It (and the associated base) need to be oriented towards 0.
 * Use "angle" to set the starting angle.
 *
 * "speed"		default 50
 * "dmg"		default 10
 * "angle"		point this forward
 * "target"	point this at an info_notnull at the muzzle tip
 * "minpitch"	min acceptable pitch angle : default -30
 * "maxpitch"	max acceptable pitch angle : default 30
 * "minyaw"	min acceptable yaw angle   : default 0
 * "maxyaw"	max acceptable yaw angle   : default 360
 */

void
turret_breach_fire(edict_t *self)
{
	vec3_t f, r, u;
	vec3_t start;
	int damage;
	int speed;

	if (!self)
	{
		return;
	}

	AngleVectors(self->s.angles, f, r, u);
	VectorMA(self->s.origin, self->move_origin[0], f, start);
	VectorMA(start, self->move_origin[1], r, start);
	VectorMA(start, self->move_origin[2], u, start);

	if((game.gametype == zaero_coop) && (EMPNukeCheck(self, start))) /* FS: Zaero specific game dll changes */
	{
		gi.sound (self, CHAN_AUTO, gi.soundindex("items/empnuke/emp_missfire.wav"), 1, ATTN_NORM, 0);
		return;
	}

	damage = 100 + random() * 50;
	speed = 550 + 50 * skill->intValue;
	fire_rocket(self->teammaster->owner, start, f, damage, speed, 150, damage);
	gi.positioned_sound(start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

void
turret_breach_think(edict_t *self)
{
	edict_t *ent;
	vec3_t current_angles;
	vec3_t delta;

	if (!self)
	{
		return;
	}

	VectorCopy(self->s.angles, current_angles);
	AnglesNormalize(current_angles);

	AnglesNormalize(self->move_angles);

	if (self->move_angles[PITCH] > 180)
	{
		self->move_angles[PITCH] -= 360;
	}

	/* clamp angles to mins & maxs */
	if (self->move_angles[PITCH] > self->pos1[PITCH])
	{
		self->move_angles[PITCH] = self->pos1[PITCH];
	}
	else if (self->move_angles[PITCH] < self->pos2[PITCH])
	{
		self->move_angles[PITCH] = self->pos2[PITCH];
	}

	if ((self->move_angles[YAW] < self->pos1[YAW]) ||
		(self->move_angles[YAW] > self->pos2[YAW]))
	{
		float dmin, dmax;

		dmin = fabs(self->pos1[YAW] - self->move_angles[YAW]);

		if (dmin < -180)
		{
			dmin += 360;
		}
		else if (dmin > 180)
		{
			dmin -= 360;
		}

		dmax = fabs(self->pos2[YAW] - self->move_angles[YAW]);

		if (dmax < -180)
		{
			dmax += 360;
		}
		else if (dmax > 180)
		{
			dmax -= 360;
		}

		if (fabs(dmin) < fabs(dmax))
		{
			self->move_angles[YAW] = self->pos1[YAW];
		}
		else
		{
			self->move_angles[YAW] = self->pos2[YAW];
		}
	}

	VectorSubtract(self->move_angles, current_angles, delta);

	if (delta[0] < -180)
	{
		delta[0] += 360;
	}
	else if (delta[0] > 180)
	{
		delta[0] -= 360;
	}

	if (delta[1] < -180)
	{
		delta[1] += 360;
	}
	else if (delta[1] > 180)
	{
		delta[1] -= 360;
	}

	delta[2] = 0;

	if (delta[0] > self->speed * FRAMETIME)
	{
		delta[0] = self->speed * FRAMETIME;
	}

	if (delta[0] < -1 * self->speed * FRAMETIME)
	{
		delta[0] = -1 * self->speed * FRAMETIME;
	}

	if (delta[1] > self->speed * FRAMETIME)
	{
		delta[1] = self->speed * FRAMETIME;
	}

	if (delta[1] < -1 * self->speed * FRAMETIME)
	{
		delta[1] = -1 * self->speed * FRAMETIME;
	}

	VectorScale(delta, 1.0 / FRAMETIME, self->avelocity);

	self->nextthink = level.time + FRAMETIME;

	for (ent = self->teammaster; ent; ent = ent->teamchain)
	{
		ent->avelocity[1] = self->avelocity[1];
	}

	/* if we have adriver, adjust his velocities */
	if (self->owner)
	{
		float angle;
		float target_z;
		float diff;
		vec3_t target;
		vec3_t dir;

		/* angular is easy, just copy ours */
		self->owner->avelocity[0] = self->avelocity[0];
		self->owner->avelocity[1] = self->avelocity[1];

		/* x & y */
		angle = self->s.angles[1] + self->owner->move_origin[1];
		angle *= (M_PI * 2 / 360);
		target[0] = SnapToEights(self->s.origin[0] + cos(angle) * self->owner->move_origin[0]);
		target[1] = SnapToEights(self->s.origin[1] + sin(angle) * self->owner->move_origin[0]);
		target[2] = self->owner->s.origin[2];

		VectorSubtract(target, self->owner->s.origin, dir);
		self->owner->velocity[0] = dir[0] * 1.0 / FRAMETIME;
		self->owner->velocity[1] = dir[1] * 1.0 / FRAMETIME;

		/* z */
		angle = self->s.angles[PITCH] * (M_PI * 2 / 360);
		target_z = SnapToEights(self->s.origin[2] + self->owner->move_origin[0] * tan(angle) + self->owner->move_origin[2]);

		diff = target_z - self->owner->s.origin[2];
		self->owner->velocity[2] = diff * 1.0 / FRAMETIME;

		if (self->spawnflags & 65536)
		{
			turret_breach_fire(self);
			self->spawnflags &= ~65536;
		}
	}
}

void
turret_breach_finish_init(edict_t *self)
{
	if (!self)
	{
		return;
	}

	/* get and save info for muzzle location */
	if (!self->target)
	{
		gi.dprintf(DEVELOPER_MSG_GAME, "%s at %s needs a target\n", self->classname, vtos(self->s.origin));
	}
	else
	{
		self->target_ent = G_PickTarget(self->target);

		if (self->target_ent) /* FS: Coop: Rogue specific, but I think this is a potential fix.  Probably OK as-is. */
		{
			VectorSubtract(self->target_ent->s.origin, self->s.origin, self->move_origin);
			G_FreeEdict(self->target_ent);
		}
		else
		{
			gi.dprintf(DEVELOPER_MSG_GAME, "could not find target entity for %s at %s\n", self->classname, vtos(self->s.origin));
		}
	}

	self->teammaster->dmg = self->dmg;
	self->think = turret_breach_think;
	self->think(self);
}

void
SP_turret_breach(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (!self->speed)
	{
		self->speed = 50;
	}

	if (!self->dmg)
	{
		self->dmg = 10;
	}

	if (!st.minpitch)
	{
		st.minpitch = -30;
	}

	if (!st.maxpitch)
	{
		st.maxpitch = 30;
	}

	if (!st.maxyaw)
	{
		st.maxyaw = 360;
	}

	self->pos1[PITCH] = -1 * st.minpitch;
	self->pos1[YAW] = st.minyaw;
	self->pos2[PITCH] = -1 * st.maxpitch;
	self->pos2[YAW] = st.maxyaw;

	self->ideal_yaw = self->s.angles[YAW];
	self->move_angles[YAW] = self->ideal_yaw;

	self->blocked = turret_blocked;

	self->think = turret_breach_finish_init;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

/*
 * QUAKED turret_base (0 0 0) ?
 * This portion of the turret changes yaw only.
 * MUST be teamed with a turret_breach.
 */

void
SP_turret_base(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->blocked = turret_blocked;
	gi.linkentity(self);
}

/*
 * QUAKED turret_driver (1 .5 0) (-16 -16 -24) (16 16 32)
 * Must NOT be on the team with the rest of the turret parts.
 * Instead it must target the turret_breach.
 */
void
turret_driver_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, vec3_t point /* unused */)
{
	edict_t *ent;

	if (!self || !inflictor || !attacker)
	{
		return;
	}

	/* level the gun */
	self->target_ent->move_angles[0] = 0;

	/* remove the driver from the end of them team chain */
	for (ent = self->target_ent->teammaster;
		 ent->teamchain != self;
		 ent = ent->teamchain)
	{
	}

	ent->teamchain = NULL;
	self->teammaster = NULL;
	self->flags &= ~FL_TEAMSLAVE;

	self->target_ent->owner = NULL;
	self->target_ent->teammaster->owner = NULL;

	infantry_die(self, inflictor, attacker, damage);
}

void
turret_driver_think(edict_t *self)
{
	vec3_t target;
	vec3_t dir;
	float reaction_time;

	if (!self)
	{
		return;
	}

	self->nextthink = level.time + FRAMETIME;

	if (self->enemy && (!self->enemy->inuse || (self->enemy->health <= 0)))
	{
		self->enemy = NULL;
	}

	if (!self->enemy)
	{
		if (!FindTarget(self))
		{
			return;
		}

		self->monsterinfo.trail_time = level.time;
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	}
	else
	{
		if (visible(self, self->enemy))
		{
			if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
			{
				self->monsterinfo.trail_time = level.time;
				self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			}
		}
		else
		{
			self->monsterinfo.aiflags |= AI_LOST_SIGHT;
			return;
		}
	}

	if (self->enemy)
	{
		/* let the turret know where we want it to aim */
		VectorCopy(self->enemy->s.origin, target);
		target[2] += self->enemy->viewheight;
		VectorSubtract(target, self->target_ent->s.origin, dir);
		vectoangles(dir, self->target_ent->move_angles);
	}

	/* decide if we should shoot */
	if (level.time < self->monsterinfo.attack_finished)
	{
		return;
	}

	reaction_time = (3 - skill->intValue) * 1.0;

	if ((level.time - self->monsterinfo.trail_time) < reaction_time)
	{
		return;
	}

	self->monsterinfo.attack_finished = level.time + reaction_time + 1.0;
	self->target_ent->spawnflags |= 65536;
}

void
turret_driver_link(edict_t *self)
{
	vec3_t vec;
	edict_t *ent;

	if (!self)
	{
		return;
	}

	self->think = turret_driver_think;
	self->nextthink = level.time + FRAMETIME;

	self->target_ent = G_PickTarget(self->target);
	self->target_ent->owner = self;
	self->target_ent->teammaster->owner = self;
	VectorCopy(self->target_ent->s.angles, self->s.angles);

	vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
	vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
	vec[2] = 0;
	self->move_origin[0] = VectorLength(vec);

	VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
	vectoangles(vec, vec);
	AnglesNormalize(vec);
	self->move_origin[1] = vec[1];

	self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

	/* add the driver to the end of them team chain */
	for (ent = self->target_ent->teammaster; ent->teamchain; ent = ent->teamchain)
	{
	}

	ent->teamchain = self;
	self->teammaster = self->target_ent->teammaster;
	self->flags |= FL_TEAMSLAVE;
}

void
SP_turret_driver(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->intValue)
	{
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BBOX;

	if(game.gametype == vanilla_coop || game.gametype == zaero_coop) /* FS: Coop: New animations, make sure download gets them. */
	{
		self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	}
	else
	{
		self->s.modelindex = gi.modelindex("models/monsters/infantry2/tris.md2");
	}

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);

	self->health = 100;
	self->gib_health = 0;
	self->mass = 200;
	self->viewheight = 24;

	self->die = turret_driver_die;
	self->monsterinfo.stand = infantry_stand;

	self->flags |= FL_NO_KNOCKBACK;

	if((game.gametype != zaero_coop) || ( (game.gametype == zaero_coop) && (!(self->spawnflags & 16))) ) /* FS: Zaero specific game dll changes */
	{
		level.total_monsters++;
	}

	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->use = monster_use;
	self->clipmask = MASK_MONSTERSOLID;
	VectorCopy(self->s.origin, self->s.old_origin);
	self->monsterinfo.aiflags |= AI_STAND_GROUND | AI_DUCKED;

	if (st.item)
	{
		self->item = FindItemByClassname(st.item);

		if (!self->item)
		{
			gi.dprintf(DEVELOPER_MSG_GAME, "%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
		}
	}

	self->think = turret_driver_link;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}

/*
 * invisible turret drivers so we can have unmanned turrets.
 * originally designed to shoot at func_trains and such, so they
 * fire at the center of the bounding box, rather than the entity's
 * origin. */

void
turret_brain_think(edict_t *self) /* FS: Coop: Rogue specific */
{
	vec3_t target;
	vec3_t dir;
	vec3_t endpos = { 0 };
	float reaction_time;
	trace_t trace;

	if (!self)
	{
		return;
	}

	self->nextthink = level.time + FRAMETIME;

	if (self->enemy)
	{
		if (!self->enemy->inuse)
		{
			self->enemy = NULL;
		}
		else if (self->enemy->takedamage && (self->enemy->health <= 0))
		{
			self->enemy = NULL;
		}
	}

	if (!self->enemy)
	{
		if (!FindTarget(self))
		{
			return;
		}

		self->monsterinfo.trail_time = level.time;
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	}
	else
	{
		VectorAdd(self->enemy->absmax, self->enemy->absmin, endpos);
		VectorScale(endpos, 0.5, endpos);

		trace = gi.trace(self->target_ent->s.origin, vec3_origin, vec3_origin,
				endpos, self->target_ent, MASK_SHOT);

		if ((trace.fraction == 1) || (trace.ent == self->enemy))
		{
			if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
			{
				self->monsterinfo.trail_time = level.time;
				self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			}
		}
		else
		{
			self->monsterinfo.aiflags |= AI_LOST_SIGHT;
			return;
		}
	}

	/* let the turret know where we want it to aim */
	VectorCopy(endpos, target);
	VectorSubtract(target, self->target_ent->s.origin, dir);
	vectoangles(dir, self->target_ent->move_angles);

	/* decide if we should shoot */
	if (level.time < self->monsterinfo.attack_finished)
	{
		return;
	}

	if (self->delay)
	{
		reaction_time = self->delay;
	}
	else
	{
		reaction_time = (3 - skill->intValue) * 1.0;
	}

	if ((level.time - self->monsterinfo.trail_time) < reaction_time)
	{
		return;
	}

	self->monsterinfo.attack_finished = level.time + reaction_time + 1.0;
	self->target_ent->spawnflags |= 65536;
}

void
turret_brain_link(edict_t *self) /* FS: Coop: Rogue specific */
{
	vec3_t vec;
	edict_t *ent;

	if (!self)
	{
		return;
	}

	if (self->killtarget)
	{
		self->enemy = G_PickTarget(self->killtarget);
	}

	self->think = turret_brain_think;
	self->nextthink = level.time + FRAMETIME;

	self->target_ent = G_PickTarget(self->target);
	self->target_ent->owner = self;
	self->target_ent->teammaster->owner = self;
	VectorCopy(self->target_ent->s.angles, self->s.angles);

	vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
	vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
	vec[2] = 0;
	self->move_origin[0] = VectorLength(vec);

	VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
	vectoangles(vec, vec);
	AnglesNormalize(vec);
	self->move_origin[1] = vec[1];

	self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

	/* add the driver to the end of them team chain */
	for (ent = self->target_ent->teammaster; ent->teamchain; ent = ent->teamchain)
	{
	}

	ent->teamchain = self;
	self->teammaster = self->target_ent->teammaster;
	self->flags |= FL_TEAMSLAVE;
}

void
turret_brain_deactivate(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */) /* FS: Coop: Rogue specific */
{
	if (!self)
	{
		return;
	}

	self->think = NULL;
	self->nextthink = 0;
}

void
turret_brain_activate(edict_t *self, edict_t *other /* unused */, edict_t *activator) /* FS: Coop: Rogue specific */
{
	if (!self || !activator)
	{
		return;
	}

	if (!self->enemy)
	{
		self->enemy = activator;
	}

	/* wait at least 3 seconds to fire. */
	self->monsterinfo.attack_finished = level.time + 3;
	self->use = turret_brain_deactivate;

	self->think = turret_brain_link;
	self->nextthink = level.time + FRAMETIME;
}

/*
 * QUAKED turret_invisible_brain (1 .5 0) (-16 -16 -16) (16 16 16)
 * Invisible brain to drive the turret.
 *
 * Does not search for targets. If targeted, can only be turned on once
 * and then off once. After that they are completely disabled.
 *
 * "delay" the delay between firing (default ramps for skill level)
 * "Target" the turret breach
 * "Killtarget" the item you want it to attack.
 * Target the brain if you want it activated later, instead of immediately. It will wait 3 seconds
 * before firing to acquire the target.
 */
void
SP_turret_invisible_brain(edict_t *self) /* FS: Coop: Rogue specific */
{
	if (!self)
	{
		return;
	}

	if (!self->killtarget)
	{
		gi.dprintf(DEVELOPER_MSG_GAME, "turret_invisible_brain with no killtarget!\n");
		G_FreeEdict(self);
		return;
	}

	if (!self->target)
	{
		gi.dprintf(DEVELOPER_MSG_GAME, "turret_invisible_brain with no target!\n");
		G_FreeEdict(self);
		return;
	}

	if (self->targetname)
	{
		self->use = turret_brain_activate;
	}
	else
	{
		self->think = turret_brain_link;
		self->nextthink = level.time + FRAMETIME;
	}

	self->movetype = MOVETYPE_PUSH;
	gi.linkentity(self);
}
