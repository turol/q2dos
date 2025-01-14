// g_phys.c

#include "g_local.h"

#define MAX_CLIP_PLANES 5
#define STOP_EPSILON 0.1
#define sv_friction 6
#define sv_waterfriction 1

void SV_Physics_NewToss(edict_t *ent);
void adjustRiders(edict_t *ent); /* FS: Zaero specific game dll changes */

typedef struct
{
	edict_t *ent;
	vec3_t origin;
	vec3_t angles;
	float deltayaw;
} pushed_t;
pushed_t pushed[MAX_EDICTS], *pushed_p;

edict_t *obstacle;

/*
 * pushmove objects do not obey gravity, and do not interact with each other or
 * trigger fields, but block normal movement and push normal objects when they move.
 *
 * onground is set for toss objects when they come to a complete rest. it is set for
 * steping or walking objects
 *
 *  - doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
 *  - bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
 *  - corpses are SOLID_NOT and MOVETYPE_TOSS
 *  - crates are SOLID_BBOX and MOVETYPE_TOSS
 *  - walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
 *  - flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY
 *  - solid_edge items only clip against bsp models.
 *
 */

edict_t *
SV_TestEntityPosition(edict_t *ent)
{
	trace_t trace;
	int mask;

	if (!ent)
	{
		return NULL;
	}

	if (ent->clipmask)
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID;
	}

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs,
			ent->s.origin, ent, mask);

	if (trace.startsolid)
	{
		if (game.gametype == vanilla_coop || game.gametype == zaero_coop) /* FS: Coop: Vanilla and Zaero specific */
		{
			if ((ent->svflags & SVF_DEADMONSTER) && (trace.ent->client || (trace.ent->svflags & SVF_MONSTER)))
			{
				return NULL;
			}
		}

		return g_edicts;
	}

	return NULL;
}

void
SV_CheckVelocity(edict_t *ent)
{
	int i;

	if (!ent)
	{
		return;
	}

	if (game.gametype == vanilla_coop || game.gametype == zaero_coop) /* FS: Coop: Vanilla and Zaero specific */
	{
		if (VectorLength(ent->velocity) > sv_maxvelocity->value)
		{
			VectorNormalize(ent->velocity);
			VectorScale(ent->velocity, sv_maxvelocity->value, ent->velocity);
		}
	}
	else
	{
		/* bound velocity */
		for (i = 0; i < 3; i++)
		{
			if (ent->velocity[i] > sv_maxvelocity->value)
			{
				ent->velocity[i] = sv_maxvelocity->value;
			}
			else if (ent->velocity[i] < -sv_maxvelocity->value)
			{
				ent->velocity[i] = -sv_maxvelocity->value;
			}
		}
	}
}

/*
 * Runs thinking code for
 * this frame if necessary
 */
qboolean
SV_RunThink(edict_t *ent)
{
	float thinktime;

	if (!ent || !ent->inuse)
	{
		return false;
	}

	thinktime = ent->nextthink;

	if (thinktime <= 0)
	{
		return true;
	}

	if (thinktime > level.time + 0.001)
	{
		return true;
	}

	ent->nextthink = 0;

	if (!ent->think)
	{
		gi.error("NULL ent->think");
	}

	ent->think(ent);

	return false;
}

/*
 * Two entities have touched, so
 * run their touch functions
 */
void
SV_Impact(edict_t *e1, trace_t *trace)
{
	edict_t *e2;

	if (!e1 || !trace)
	{
		return;
	}

	e2 = trace->ent;

	if (e1->touch && (e1->solid != SOLID_NOT))
	{
		e1->touch(e1, e2, &trace->plane, trace->surface);
	}

	if (e2->touch && (e2->solid != SOLID_NOT))
	{
		e2->touch(e2, e1, NULL, NULL);
	}
}

/*
 * Slide off of the impacting object
 * returns the blocked flags (1 = floor,
 * 2 = step / wall)
 */

int
ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff;
	float change;
	int i, blocked;

	blocked = 0;

	if (normal[2] > 0)
	{
		blocked |= 1; /* floor */
	}

	if (!normal[2])
	{
		blocked |= 2; /* step */
	}

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if ((out[i] > -STOP_EPSILON) && (out[i] < STOP_EPSILON))
		{
			out[i] = 0;
		}
	}

	return blocked;
}

/*
 * The basic solid body movement clip that slides
 * along multiple planes. Returns the clipflags if
 * the velocity was modified (hit something solid)
 * 1 = floor
 * 2 = wall / step
 * 4 = dead stop
 */
int
SV_FlyMove(edict_t *ent, float time, int mask)
{
	edict_t *hit;
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;
	int i, j;
	trace_t trace;
	vec3_t end;
	float time_left;
	int blocked;

	if (!ent)
	{
		return 0;
	}

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->velocity, original_velocity);
	VectorCopy(ent->velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	ent->groundentity = NULL;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
		{
			end[i] = ent->s.origin[i] + time_left * ent->velocity[i];
		}

		trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, mask);

		if (trace.allsolid)
		{
			/* entity is trapped in another solid */
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{
			/* actually covered some distance */
			VectorCopy(trace.endpos, ent->s.origin);
			VectorCopy(ent->velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
		{
			break; /* moved the entire distance */
		}

		hit = trace.ent;

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1; /* floor */

			if (hit->solid == SOLID_BSP)
			{
				ent->groundentity = hit;
				ent->groundentity_linkcount = hit->linkcount;
			}
		}

		if (!trace.plane.normal[2])
		{
			blocked |= 2; /* step */
		}

		/* run the impact function */
		SV_Impact(ent, &trace);

		if (!ent->inuse)
		{
			break; /* removed by the impact function */
		}

		time_left -= time_left * trace.fraction;

		/* cliped to another plane */
		if (numplanes >= MAX_CLIP_PLANES)
		{
			/* this shouldn't really happen */
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		/* modify original_velocity so it parallels all of the clip planes */
		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);

			for (j = 0; j < numplanes; j++)
			{
				if ((j != i) && !VectorCompare(planes[i], planes[j]))
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
					{
						break; /* not ok */
					}
				}
			}

			if (j == numplanes)
			{
				break;
			}
		}

		if (i != numplanes)
		{
			/* go along this plane */
			VectorCopy(new_velocity, ent->velocity);
		}
		else
		{
			/* go along the crease */
			if (numplanes != 2)
			{
				VectorCopy(vec3_origin, ent->velocity);
				return 7;
			}

			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->velocity);
			VectorScale(dir, d, ent->velocity);
		}

		/* if original velocity is against the original velocity,
		   stop dead to avoid tiny occilations in sloping corners */
		if (DotProduct(ent->velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, ent->velocity);
			return blocked;
		}
	}

	return blocked;
}

void
SV_AddGravity(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if ((game.gametype == rogue_coop) && (ent->gravityVector[2] > 0)) /* FS: Coop: Rogue specific */
	{
		VectorMA(ent->velocity, ent->gravity * sv_gravity->value * FRAMETIME,
				ent->gravityVector, ent->velocity);
	}
	else
	{
		ent->velocity[2] -= ent->gravity * sv_gravity->value * FRAMETIME;
	}
}

/*
 * Returns the actual bounding box of a bmodel.
 * This is a big improvement over what q2 normally
 * does with rotating bmodels - q2 sets absmin,
 * absmax to a cube that will completely contain
 * the bmodel at *any* rotation on *any* axis, whether
 * the bmodel can actually rotate to that angle or not.
 * This leads to a lot of false block tests in SV_Push
 * if another bmodel is in the vicinity.
 */
void
RealBoundingBox(edict_t *ent, vec3_t mins, vec3_t maxs)
{
	vec3_t forward, left, up, f1, l1, u1;
	vec3_t p[8];
	int i, j, k, j2, k4;

	if (!ent)
	{
		return;
	}

	for (k = 0; k < 2; k++)
	{
		k4 = k * 4;

		if (k)
		{
			p[k4][2] = ent->maxs[2];
		}
		else
		{
			p[k4][2] = ent->mins[2];
		}

		p[k4 + 1][2] = p[k4][2];
		p[k4 + 2][2] = p[k4][2];
		p[k4 + 3][2] = p[k4][2];

		for (j = 0; j < 2; j++)
		{
			j2 = j * 2;

			if (j)
			{
				p[j2 + k4][1] = ent->maxs[1];
			}
			else
			{
				p[j2 + k4][1] = ent->mins[1];
			}

			p[j2 + k4 + 1][1] = p[j2 + k4][1];

			for (i = 0; i < 2; i++)
			{
				if (i)
				{
					p[i + j2 + k4][0] = ent->maxs[0];
				}
				else
				{
					p[i + j2 + k4][0] = ent->mins[0];
				}
			}
		}
	}

	AngleVectors(ent->s.angles, forward, left, up);

	for (i = 0; i < 8; i++)
	{
		VectorScale(forward, p[i][0], f1);
		VectorScale(left, -p[i][1], l1);
		VectorScale(up, p[i][2], u1);
		VectorAdd(ent->s.origin, f1, p[i]);
		VectorAdd(p[i], l1, p[i]);
		VectorAdd(p[i], u1, p[i]);
	}

	VectorCopy(p[0], mins);
	VectorCopy(p[0], maxs);

	for (i = 1; i < 8; i++)
	{
		if (mins[0] > p[i][0])
		{
			mins[0] = p[i][0];
		}

		if (mins[1] > p[i][1])
		{
			mins[1] = p[i][1];
		}

		if (mins[2] > p[i][2])
		{
			mins[2] = p[i][2];
		}

		if (maxs[0] < p[i][0])
		{
			maxs[0] = p[i][0];
		}

		if (maxs[1] < p[i][1])
		{
			maxs[1] = p[i][1];
		}

		if (maxs[2] < p[i][2])
		{
			maxs[2] = p[i][2];
		}
	}
}

/*
 * Does not change the entities velocity at all
 */
trace_t
SV_PushEntity(edict_t *ent, vec3_t push)
{
	trace_t trace;
	vec3_t start;
	vec3_t end;
	int mask;

	if (!ent) /* FS: Coop: Rogue specific.  Probably OK as-is. */
	{
		memset(&trace,0,sizeof(trace_t));
		return trace;
	}

	VectorCopy(ent->s.origin, start);
	VectorAdd(start, push, end);

retry:
	if (ent->clipmask)
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID;
	}

	trace = gi.trace(start, ent->mins, ent->maxs, end, ent, mask);

	if (game.gametype == vanilla_coop || game.gametype == zaero_coop) /* FS: Coop: Vanilla and Zaero specific */
	{
		if (trace.startsolid || trace.allsolid)
		{
			mask ^= CONTENTS_DEADMONSTER;
			trace = gi.trace (start, ent->mins, ent->maxs, end, ent, mask);
		}
	}

	VectorCopy(trace.endpos, ent->s.origin);
	if (ent->inuse)
		gi.linkentity(ent);

	if (trace.fraction != 1.0)
	{
		SV_Impact(ent, &trace);

		/* if the pushed entity went away and the pusher is still there */
		if (!trace.ent->inuse && ent->inuse)
		{
			/* move the pusher back and try again */
			VectorCopy(start, ent->s.origin);
			gi.linkentity(ent);
			goto retry;
		}
	}

	if (game.gametype == rogue_coop) /* FS: Coop: Rogue specific */
	{
		ent->gravity = 1.0;
	}

	if (ent->inuse)
	{
		G_TouchTriggers(ent);
	}

	return trace;
}

/*
 * Objects need to be moved back on a failed push,
 * otherwise riders would continue to slide.
 */
qboolean
SV_Push(edict_t *pusher, vec3_t move, vec3_t amove)
{
	int i, e;
	edict_t *check, *block;
	pushed_t *p;
	vec3_t org, org2, move2, forward, right, up;
	vec3_t realmins, realmaxs;

	if (!pusher)
	{
		return false;
	}

	/* clamp the move to 1/8 units, so the position
	   will be accurate for client side prediction */
	for (i = 0; i < 3; i++)
	{
		float temp;
		temp = move[i] * 8.0;

		if (temp > 0.0)
		{
			temp += 0.5;
		}
		else
		{
			temp -= 0.5;
		}

		move[i] = 0.125 * (int)temp;
	}

	/* we need this for pushing things later */
	VectorSubtract(vec3_origin, amove, org);
	AngleVectors(org, forward, right, up);

	/* save the pusher's original position */
	pushed_p->ent = pusher;
	VectorCopy(pusher->s.origin, pushed_p->origin);
	VectorCopy(pusher->s.angles, pushed_p->angles);

	if (pusher->client)
	{
		pushed_p->deltayaw = pusher->client->ps.pmove.delta_angles[YAW];
	}

	pushed_p++;

	/* move the pusher to it's final position */
	VectorAdd(pusher->s.origin, move, pusher->s.origin);
	VectorAdd(pusher->s.angles, amove, pusher->s.angles);
	gi.linkentity(pusher);

	/* Create a real bounding box for
	   rotating brush models. */
	RealBoundingBox(pusher, realmins, realmaxs);

	/* see if any solid entities are inside the final position */
	check = g_edicts + 1;

	for (e = 1; e < globals.num_edicts; e++, check++)
	{
		if (!check->inuse)
		{
			continue;
		}

		if ((check->movetype == MOVETYPE_PUSH) ||
			(check->movetype == MOVETYPE_STOP) ||
			(check->movetype == MOVETYPE_NONE) ||
			(check->movetype == MOVETYPE_NOCLIP))
		{
			continue;
		}

		if (!check->area.prev)
		{
			continue; /* not linked in anywhere */
		}

		/* if the entity is standing on the pusher, it will definitely be moved */
		if (check->groundentity != pusher)
		{
			/* see if the ent needs to be tested */
			if ((check->absmin[0] >= realmaxs[0]) ||
				(check->absmin[1] >= realmaxs[1]) ||
				(check->absmin[2] >= realmaxs[2]) ||
				(check->absmax[0] <= realmins[0]) ||
				(check->absmax[1] <= realmins[1]) ||
				(check->absmax[2] <= realmins[2]))
			{
				continue;
			}

			/* see if the ent's bbox is inside the pusher's final position */
			if (!SV_TestEntityPosition(check))
			{
				continue;
			}
		}

		if ((pusher->movetype == MOVETYPE_PUSH) ||
			(check->groundentity == pusher))
		{
			/* move this entity */
			pushed_p->ent = check;
			VectorCopy(check->s.origin, pushed_p->origin);
			VectorCopy(check->s.angles, pushed_p->angles);
			pushed_p++;

			/* try moving the contacted entity */
			VectorAdd(check->s.origin, move, check->s.origin);

			if (check->client)
			{
				check->client->ps.pmove.delta_angles[YAW] += amove[YAW];
			}

			/* figure movement due to the pusher's amove */
			VectorSubtract(check->s.origin, pusher->s.origin, org);
			org2[0] = DotProduct(org, forward);
			org2[1] = -DotProduct(org, right);

			/* Quirk for blocking Elevators when
			   running under amd64. This is most
			   likey  caused by a too high float
			   precision. -_-  */
			/* FS: Coop: Xatrix specific hack */
			if (((pusher->s.number == 285) &&
				 (Q_strcasecmp(level.mapname, "xcompnd2") == 0)) ||
				((pusher->s.number == 520) &&
				 (Q_strcasecmp(level.mapname, "xsewer2") == 0)))
			{
				org2[2] = DotProduct(org, up) + 2;
			}
			else
			{
				org2[2] = DotProduct(org, up);
			}

			VectorSubtract(org2, org, move2);
			VectorAdd(check->s.origin, move2, check->s.origin);

			/* may have pushed them off an edge */
			if (check->groundentity != pusher)
			{
				check->groundentity = NULL;
			}

			block = SV_TestEntityPosition(check);

			if (!block)
			{
				/* pushed ok */
				gi.linkentity(check);
				if (game.gametype == zaero_coop)
				{
					adjustRiders(check);
				}

				/* impact? */
				continue;
			}

			/* if it is ok to leave in the old position, do it
			   this is only relevent for riding entities, not pushed */
			VectorSubtract(check->s.origin, move, check->s.origin);
			block = SV_TestEntityPosition(check);

			if (!block)
			{
				pushed_p--;
				continue;
			}
		}

		/* save off the obstacle so we can call the block function */
		obstacle = check;

		/* move back any entities we already moved
		   go backwards, so if the same entity was pushed
		   twice, it goes back to the original position */
		for (p = pushed_p - 1; p >= pushed; p--)
		{
			VectorCopy(p->origin, p->ent->s.origin);
			VectorCopy(p->angles, p->ent->s.angles);

			if (p->ent->client)
			{
				p->ent->client->ps.pmove.delta_angles[YAW] = p->deltayaw;
			}

			gi.linkentity(p->ent);
		}

		return false;
	}

	/* see if anything we moved has touched a trigger */
	for (p = pushed_p - 1; p >= pushed; p--)
	{
		G_TouchTriggers(p->ent);
	}

	return true;
}

/*
 * Bmodel objects don't interact with each
 * other, but push all box objects
 */
void
SV_Physics_Pusher(edict_t *ent)
{
	vec3_t move, amove;
	edict_t *part, *mv;

	if (!ent)
	{
		return;
	}

	/* if not a team captain, so movement
	   will be handled elsewhere */
	if (ent->flags & FL_TEAMSLAVE)
	{
		return;
	}

	/* make sure all team slaves can move before commiting any moves
	   or calling any think functionsif the move is blocked, all moved
	   objects will be backed out */
	pushed_p = pushed;

	for (part = ent; part; part = part->teamchain)
	{
		if (part->velocity[0] || part->velocity[1] || part->velocity[2] ||
			part->avelocity[0] || part->avelocity[1] || part->avelocity[2])
		{
			/* object is moving */
			VectorScale(part->velocity, FRAMETIME, move);
			VectorScale(part->avelocity, FRAMETIME, amove);

			if (!SV_Push(part, move, amove))
			{
				break; /* move was blocked */
			}
		}
	}

	if (pushed_p > &pushed[MAX_EDICTS-1])
	{
		gi.error("pushed_p > &pushed[MAX_EDICTS-1], memory corrupted");
	}

	if (part)
	{
		/* the move failed, bump all nextthink times and back out moves */
		for (mv = ent; mv; mv = mv->teamchain)
		{
			if (mv->nextthink > 0)
			{
				mv->nextthink += FRAMETIME;
			}
		}

		/* if the pusher has a "blocked" function, call it
		   otherwise, just stay in place until the obstacle
		   is gone */
		if (part->blocked)
		{
			part->blocked(part, obstacle);
		}
	}
	else
	{
		/* the move succeeded, so call all think functions */
		for (part = ent; part; part = part->teamchain)
		{
			/* prevent entities that are on trains that have gone away from thinking! */
			if ((game.gametype != rogue_coop) || (game.gametype == rogue_coop && part->inuse)) /* FS: Coop: Rogue specific */
			{
				SV_RunThink(part);
			}
		}
	}
}

/*
 * Non moving objects can only think
 */
void
SV_Physics_None(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	/* regular thinking */
	SV_RunThink(ent);
}

/*
 * A moving object that doesn't obey physics
 */
void
SV_Physics_Noclip(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	/* regular thinking */
	if (!SV_RunThink(ent))
	{
		return;
	}

	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);
	VectorMA(ent->s.origin, FRAMETIME, ent->velocity, ent->s.origin);

	gi.linkentity(ent);
}

/*
 * Toss, bounce, and fly movement.  When onground, do nothing.
 */
void
SV_Physics_Toss(edict_t *ent)
{
	trace_t trace;
	vec3_t move;
	float backoff;
	edict_t *slave;
	qboolean wasinwater;
	qboolean isinwater;
	vec3_t old_origin;
	float speed = 0.0f; /* FS: Zaero specific */

	if (!ent)
	{
		return;
	}

	/* regular thinking */
	SV_RunThink(ent);

	/* if not a team captain, so movement will be handled elsewhere */
	if (ent->flags & FL_TEAMSLAVE)
	{
		return;
	}

	if (ent->velocity[2] > 0)
	{
		ent->groundentity = NULL;
	}

	/* check for the groundentity going away */
	if (ent->groundentity)
	{
		if (!ent->groundentity->inuse)
		{
			ent->groundentity = NULL;
		}
	}

	/* if onground, return without moving */
	if (game.gametype == rogue_coop) /* FS: Coop: Rogue specific */
	{
		if (ent->groundentity && (ent->gravity > 0.0))
		{
			return;
		}
	}
	else
	{
		if (ent->groundentity)
		{
			return;
		}
	}
	VectorCopy(ent->s.origin, old_origin);

	SV_CheckVelocity(ent);

	/* add gravity */
	if ((ent->movetype != MOVETYPE_FLY) &&
		(ent->movetype != MOVETYPE_FLYMISSILE) &&
		(ent->movetype != MOVETYPE_WALLBOUNCE) && /* FS: Coop: Xatrix specific -- MOVETYPE_WALLBOUNCE */
		(ent->movetype != MOVETYPE_BOUNCEFLY)) /* FS: Coop: Xatrix specific -- MOVETYPE_BOUNCEFLY */	
	{
		SV_AddGravity(ent);
	}

	/* move angles */
	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);

	/* move origin */
	VectorScale(ent->velocity, FRAMETIME, move);
	trace = SV_PushEntity(ent, move);

	if (!ent->inuse)
	{
		return;
	}

	if (trace.fraction < 1)
	{
		if (ent->movetype == MOVETYPE_WALLBOUNCE) /* FS: Coop: Xatrix specific */
		{
			backoff = 2.0;
		}
		else if(ent->movetype == MOVETYPE_BOUNCEFLY) /* FS: Zaero specific game dll changes */
		{
			backoff = 2;
		}
		else if (ent->movetype == MOVETYPE_BOUNCE)
		{
			backoff = 1.5;
		}
		else
		{
			backoff = 1;
		}

		if(ent->movetype == MOVETYPE_BOUNCEFLY) /* FS: Zaero specific game dll changes */
		{
			speed = VectorLength(ent->velocity);
		}

		ClipVelocity(ent->velocity, trace.plane.normal, ent->velocity, backoff);

		if (ent->movetype == MOVETYPE_WALLBOUNCE) /* FS: Coop: Xatrix specific */
		{
			vectoangles(ent->velocity, ent->s.angles);
		}

		if(ent->movetype == MOVETYPE_BOUNCEFLY) /* FS: Zaero specific game dll changes */
		{
			VectorNormalize (ent->velocity);
			VectorScale (ent->velocity, speed, ent->velocity);
		}

		/* stop if on ground */
		if ((trace.plane.normal[2] > 0.7) &&
			(ent->movetype != MOVETYPE_WALLBOUNCE) && /* FS: Coop: Xatrix specific -- MOVETYPE_WALLBOUNCE */
			(ent->movetype != MOVETYPE_BOUNCEFLY)) /* FS: Coop: Zaero specific -- MOVETYPE_BOUNCEFLY */
		{
			if ((ent->velocity[2] < 60) || (ent->movetype != MOVETYPE_BOUNCE))
			{
				ent->groundentity = trace.ent;
				ent->groundentity_linkcount = trace.ent->linkcount;
				VectorCopy(vec3_origin, ent->velocity);
				VectorCopy(vec3_origin, ent->avelocity);
			}
		}
	}

	/* check for water transition */
	wasinwater = (ent->watertype & MASK_WATER);
	ent->watertype = gi.pointcontents(ent->s.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
	{
		ent->waterlevel = 1;
	}
	else
	{
		ent->waterlevel = 0;
	}

	if (!wasinwater && isinwater)
	{
		/* don't play splash sound for entities already in water on level start */
		if (level.framenum > 3)
		{
			gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
		}
	}
	else if (wasinwater && !isinwater)
	{
		gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
	}

	/* move teamslaves */
	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		VectorCopy(ent->s.origin, slave->s.origin);
		gi.linkentity(slave);
	}
}

/*
 * Monsters freefall when they don't have a ground entity, otherwise
 * all movement is done with discrete steps.
 *
 * This is also used for objects that have become still on the ground, but
 * will fall if the floor is pulled out from under them.
 */
void
SV_AddRotationalFriction(edict_t *ent)
{
	int n;
	float adjustment;

	if (!ent)
	{
		return;
	}

	VectorMA(ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles);
	adjustment = FRAMETIME * sv_stopspeed->value * sv_friction;

	for (n = 0; n < 3; n++)
	{
		if (ent->avelocity[n] > 0)
		{
			ent->avelocity[n] -= adjustment;

			if (ent->avelocity[n] < 0)
			{
				ent->avelocity[n] = 0;
			}
		}
		else
		{
			ent->avelocity[n] += adjustment;

			if (ent->avelocity[n] > 0)
			{
				ent->avelocity[n] = 0;
			}
		}
	}
}

void
SV_Physics_Step(edict_t *ent)
{
	qboolean wasonground;
	qboolean hitsound = false;
	float *vel;
	float speed, newspeed, control;
	float friction;
	edict_t *groundentity;
	int mask;

	if (!ent)
	{
		return;
	}

	/* airborn monsters should always check for ground */
	if (!ent->groundentity)
	{
		M_CheckGround(ent);
	}

	groundentity = ent->groundentity;

	SV_CheckVelocity(ent);

	if (groundentity)
	{
		wasonground = true;
	}
	else
	{
		wasonground = false;
	}

	if (ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
	{
		SV_AddRotationalFriction(ent);
	}

	/* add gravity except:
	     flying monsters
	     swimming monsters who are in the water */
	if (!wasonground)
	{
		if (!(ent->flags & FL_FLY))
		{
			if (!((ent->flags & FL_SWIM) && (ent->waterlevel > 2)))
			{
				if (ent->velocity[2] < sv_gravity->value * -0.1)
				{
					hitsound = true;
				}

				if (ent->waterlevel == 0)
				{
					SV_AddGravity(ent);
				}
			}
		}
	}

	/* friction for flying monsters that have been given vertical velocity */
	if ((ent->flags & FL_FLY) && (ent->velocity[2] != 0))
	{
		speed = fabs(ent->velocity[2]);
		control = speed < sv_stopspeed->value ? sv_stopspeed->value : speed;
		friction = sv_friction / 3;
		newspeed = speed - (FRAMETIME * control * friction);

		if (newspeed < 0)
		{
			newspeed = 0;
		}

		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	/* friction for flying monsters that have been given vertical velocity */
	if ((ent->flags & FL_SWIM) && (ent->velocity[2] != 0))
	{
		speed = fabs(ent->velocity[2]);
		control = speed < sv_stopspeed->value ? sv_stopspeed->value : speed;
		newspeed = speed - (FRAMETIME * control * sv_waterfriction * ent->waterlevel);

		if (newspeed < 0)
		{
			newspeed = 0;
		}

		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	if (ent->velocity[2] || ent->velocity[1] || ent->velocity[0])
	{
		/* apply friction */
		if ((wasonground) || (ent->flags & (FL_SWIM | FL_FLY)))
		{
			if (!((ent->health <= 0.0) && !M_CheckBottom(ent)))
			{
				vel = ent->velocity;
				speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);

				if (speed)
				{
					friction = sv_friction;

					control = speed < sv_stopspeed->value ? sv_stopspeed->value : speed;
					newspeed = speed - FRAMETIME * control * friction;

					if (newspeed < 0)
					{
						newspeed = 0;
					}

					newspeed /= speed;

					vel[0] *= newspeed;
					vel[1] *= newspeed;
				}
			}
		}

		if (ent->svflags & SVF_MONSTER)
		{
			mask = MASK_MONSTERSOLID;
		}
		else
		{
			mask = MASK_SOLID;
		}

		SV_FlyMove(ent, FRAMETIME, mask);

		gi.linkentity(ent);

		if (game.gametype == rogue_coop) /* FS: Coop: Rogue specific */
		{
			ent->gravity = 1.0;
		}

		G_TouchTriggers(ent);

		if (!ent->inuse)
		{
			return;
		}

		if (ent->groundentity)
		{
			if (!wasonground)
			{
				if (hitsound)
				{
					gi.sound(ent, 0, gi.soundindex("world/land.wav"), 1, 1, 0);
				}
			}
		}
	}

	if (!ent->inuse) /* FS: Coop: Rogue specific -- "g_touchtrigger free problem".  Looks like a workaround/fix.  Probably OK as-is. */
	{
		return;
	}

	/* regular thinking */
	SV_RunThink(ent);
}

void SV_Physics_FallFloat (edict_t *ent) /* FS: Zaero specific game dll changes */
{
	float gravVal = ent->gravity * sv_gravity->value * FRAMETIME;
	qboolean wasonground = false;
	qboolean hitsound = false;
	
	// check velocity
	SV_CheckVelocity (ent);

	wasonground = (ent->groundentity == NULL);
	if (ent->velocity[2] < sv_gravity->value*-0.1)
		hitsound = true;

	if (!ent->waterlevel)
	{
		vec3_t min, max;
		trace_t tr;
		vec3_t end;
		vec3_t normal;
		vec3_t gravity;

		VectorCopy(ent->mins, min);
		VectorCopy(ent->maxs, max);
		
		VectorCopy(ent->s.origin, end);
		end[2] -= 0.25; // down 4

		tr = gi.trace(ent->s.origin, min, max, end, ent, MASK_SHOT);
		if (tr.plane.normal[2] > 0.7) // on solid ground
		{
			ent->groundentity = tr.ent;
			VectorCopy(tr.endpos, ent->s.origin);
			VectorSet(ent->velocity, 0, 0, 0);
		}
		else if (tr.fraction < 1.0 && tr.plane.normal[2] <= 0.7) // on steep slope
		{
			VectorCopy(tr.plane.normal, normal);
			VectorSet(gravity, 0, 0, -gravVal);
			VectorMA(gravity, gravVal, normal, ent->velocity);
			ent->groundentity = NULL;
		}
		else // in freefall
		{
			ent->velocity[2] -= gravVal;
			ent->groundentity = NULL;
		}
	}
	else
	{
		// where's the midpoint? above or below the water?
		const double WATER_MASS = 500.0;
		vec3_t accel;
		double percentBelow = 0.0;
		double massOfObject = 0.0;
		double massOfVolumeWater = 0.0;
		double massOfWater = 0.0;
		double massDiff = 0.0;
		double i = 0.0;
		vec3_t volume;

		// TODO if we're not grounded on the bottom of the lake...

		// calculate massPerCubicMetre
		VectorScale(ent->size, 1.0/32.0, volume);
		massOfObject = ent->mass;
		massOfVolumeWater = WATER_MASS * (volume[0] * volume[1] * volume[2]);

		// how much of ourself is actually in the water?
		percentBelow = 1.0;
		for (i = 0.0; i <= 1.0; i += 0.05)
		{
			vec3_t midpoint;
			int watertype;
		
			VectorAdd(ent->s.origin, ent->mins, midpoint);
			VectorMA(midpoint, i, ent->maxs, midpoint);
			watertype = gi.pointcontents (midpoint);

			if (!(watertype & MASK_WATER))
			{
				percentBelow = i - 0.05;
				break;
			}
		}
		if (percentBelow < 0.05) // safety net
			percentBelow = 0.0;
		massOfWater = percentBelow * massOfVolumeWater;
		massDiff = massOfWater - massOfObject; // difference between
		VectorClear(accel);
		VectorSet(accel, 0, 0, gravVal * (massDiff / massOfVolumeWater));
		VectorScale(ent->velocity, 0.7, ent->velocity);
		if (VectorLength(accel) > 4)
			VectorAdd(ent->velocity, accel, ent->velocity);
	}

	if (ent->velocity[0] || ent->velocity[1] || ent->velocity[2])
	{
		qboolean isinwater = false;
		qboolean wasinwater = false;
		vec3_t old_origin;
		VectorCopy (ent->s.origin, old_origin);
	
		SV_FlyMove (ent, FRAMETIME, MASK_SHOT);

		gi.linkentity (ent);
		G_TouchTriggers (ent);

		if (ent->groundentity)
			if (!wasonground)
				if (hitsound)
					gi.sound (ent, 0, gi.soundindex("world/land.wav"), 1, 1, 0);

		// check for water transition
		wasinwater = (ent->watertype & MASK_WATER);
		ent->watertype = gi.pointcontents (ent->s.origin);
		isinwater = ent->watertype & MASK_WATER;

		if (isinwater)
			ent->waterlevel = 1;
		else
			ent->waterlevel = 0;

		if (!wasinwater && isinwater)
			gi.positioned_sound (old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
		else if (wasinwater && !isinwater)
			gi.positioned_sound (ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);

	}

	// relink
	gi.linkentity(ent);

	// regular thinking
	SV_RunThink (ent);
}

void adjustRiders(edict_t *ent) /* FS: Zaero specific game dll changes */
{
	int i = 0;

	// make sure the offsets are constant
	for (i = 0; i < 2; i++)
	{
		if (ent->rideWith[i] != NULL)
			VectorAdd(ent->s.origin, ent->rideWithOffset[i], ent->rideWith[i]->s.origin);
	}
}

void SV_Physics_Ride (edict_t *ent) /* FS: Zaero specific game dll changes */
{
	// base ourself on the step
	SV_Physics_Step(ent);

	adjustRiders(ent);
}


void
G_RunEntity(edict_t *ent)
{
	trace_t trace; /* FS: Coop: Rogue specific */
	vec3_t previous_origin; /* FS: Coop: Rogue specific */

	if (!ent)
	{
		return;
	}

	if ((game.gametype == rogue_coop) && (ent->movetype == MOVETYPE_STEP)) /* FS: Coop: Rogue specific */
	{
		VectorCopy(ent->s.origin, previous_origin);
	}

	if (ent->prethink)
	{
		ent->prethink(ent);
	}

	switch ((int)ent->movetype)
	{
		case MOVETYPE_PUSH:
		case MOVETYPE_STOP:
			SV_Physics_Pusher(ent);
			break;
		case MOVETYPE_NONE:
			SV_Physics_None(ent);
			break;
		case MOVETYPE_NOCLIP:
			SV_Physics_Noclip(ent);
			break;
		case MOVETYPE_STEP:
			SV_Physics_Step(ent);
			break;
		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_FLY:
		case MOVETYPE_BOUNCEFLY: /* FS: Zaero specific game dll changes */
		case MOVETYPE_FLYMISSILE:
			SV_Physics_Toss(ent);
			break;
		case MOVETYPE_NEWTOSS: /* FS: Coop: Rogue specific */
			SV_Physics_NewToss(ent);
			break;
		case MOVETYPE_WALLBOUNCE: /* FS: Coop: Xatrix specific */
			SV_Physics_Toss(ent);
			break;
		case MOVETYPE_FALLFLOAT: /* FS: Zaero specific game dll changes */
			SV_Physics_FallFloat(ent);
			break;
		case MOVETYPE_RIDE: /* FS: Zaero specific game dll changes */
			SV_Physics_Ride(ent);
			break;
		default:
			gi.error("SV_Physics: bad movetype %i", (int)ent->movetype);
	}

	if ((game.gametype == rogue_coop) && (ent->movetype == MOVETYPE_STEP)) /* FS: Coop: Rogue specific */
	{
		/* if we moved, check and fix origin if needed */
		if (!VectorCompare(ent->s.origin, previous_origin))
		{
			trace = gi.trace(ent->s.origin, ent->mins, ent->maxs,
					previous_origin, ent, MASK_MONSTERSOLID);

			if (trace.allsolid || trace.startsolid)
			{
				VectorCopy(previous_origin, ent->s.origin);
			}
		}
	}
}

/*
 * Toss, bounce, and fly movement. When on ground and
 * no velocity, do nothing. With velocity, slide.
 */
void
SV_Physics_NewToss(edict_t *ent) /* FS: Coop: Rogue specific */
{
	trace_t trace;
	vec3_t move;
	edict_t *slave;
	qboolean wasinwater;
	qboolean isinwater;
	float speed, newspeed;
	vec3_t old_origin;

	if (!ent)
	{
		return;
	}

	/* regular thinking */
	SV_RunThink(ent);

	/* if not a team captain, so movement will be handled elsewhere */
	if (ent->flags & FL_TEAMSLAVE)
	{
		return;
	}

	/* find out what we're sitting on. */
	VectorCopy(ent->s.origin, move);
	move[2] -= 0.25;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs,
			move, ent, ent->clipmask);

	if (ent->groundentity && ent->groundentity->inuse)
	{
		ent->groundentity = trace.ent;
	}
	else
	{
		ent->groundentity = NULL;
	}

	/* if we're sitting on something flat and have no velocity of our own, return. */
	if (ent->groundentity && (trace.plane.normal[2] == 1.0) &&
		!ent->velocity[0] && !ent->velocity[1] && !ent->velocity[2])
	{
		return;
	}

	/* store the old origin */
	VectorCopy(ent->s.origin, old_origin);

	SV_CheckVelocity(ent);

	/* add gravity */
	SV_AddGravity(ent);

	if (ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
	{
		SV_AddRotationalFriction(ent);
	}

	/* add friction */
	speed = VectorLength(ent->velocity);

	if (ent->waterlevel) /* friction for water movement */
	{
		newspeed = speed - (sv_waterfriction * 6 * ent->waterlevel);

		if (newspeed < 0)
		{
			newspeed = 0;
		}

		newspeed /= speed;
		VectorScale(ent->velocity, newspeed, ent->velocity);
	}
	else if (!ent->groundentity) /* friction for air movement */
	{
		newspeed = speed - ((sv_friction));

		if (newspeed < 0)
		{
			newspeed = 0;
		}

		newspeed /= speed;
		VectorScale(ent->velocity, newspeed, ent->velocity);
	}
	else /* use ground friction */
	{
		newspeed = speed - (sv_friction * 6);

		if (newspeed < 0)
		{
			newspeed = 0;
		}

		newspeed /= speed;
		VectorScale(ent->velocity, newspeed, ent->velocity);
	}

	SV_FlyMove(ent, FRAMETIME, ent->clipmask);
	gi.linkentity(ent);

	G_TouchTriggers(ent);

	/* check for water transition */
	wasinwater = (ent->watertype & MASK_WATER);
	ent->watertype = gi.pointcontents(ent->s.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
	{
		ent->waterlevel = 1;
	}
	else
	{
		ent->waterlevel = 0;
	}

	if (!wasinwater && isinwater)
	{
		gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
	}
	else if (wasinwater && !isinwater)
	{
		gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
	}

	/* move teamslaves */
	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		VectorCopy(ent->s.origin, slave->s.origin);
		gi.linkentity(slave);
	}
}
