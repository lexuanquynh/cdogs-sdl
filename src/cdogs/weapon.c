/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "weapon.h"

#include <assert.h>
#include <math.h>

#include <json/json.h>

#include "config.h"
#include "game_events.h"
#include "json_utils.h"
#include "objs.h"
#include "sounds.h"

CArray gGunDescriptions;	// of GunDescription

#define RELOAD_DISTANCE_PLUS 300

const TOffsetPic cGunPics[GUNPIC_COUNT][DIRECTION_COUNT][GUNSTATE_COUNT] = {
	{
	 {{-2, -10, 86}, {-3, -8, 78}, {-3, -7, 78}},
	 {{-2, -10, 87}, {-2, -9, 79}, {-3, -8, 79}},
	 {{0, -12, 88}, {0, -5, 80}, {-1, -5, 80}},
	 {{-2, -9, 90}, {0, -2, 81}, {-1, -3, 81}},
	 {{-2, -9, 90}, {-1, -2, 82}, {-1, -3, 82}},
	 {{-6, -10, 91}, {-7, -4, 83}, {-6, -5, 83}},
	 {{-8, -11, 92}, {-12, -6, 84}, {-11, -6, 84}},
	 {{-6, -14, 93}, {-8, -12, 85}, {-7, -11, 85}}
	 },
	{
	 {{-1, -7, 142}, {-1, -7, 142}, {-1, -7, 142}},
	 {{-1, -7, 142}, {-1, -7, 142}, {-1, -7, 142}},
	 {{-2, -8, 143}, {-2, -8, 143}, {-2, -8, 143}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-8, -10, 145}, {-8, -10, 145}, {-8, -10, 145}},
	 {{-8, -10, 145}, {-8, -10, 145}, {-8, -10, 145}}
	 }
};

const OffsetTable cMuzzleOffset[GUNPIC_COUNT] = {
	{
	 {2, 0},
	 {7, 2},
	 {13, 2},
	 {7, 6},
	 {2, 6},
	 {2, 6},
	 {0, 2},
	 {2, 2}
	 }
};

// Initialise all the static weapon data
#define VERSION 1
static void LoadGunDescription(
	GunDescription *g, json_t *node, const GunDescription *defaultGun);
void WeaponInitialize(CArray *descs, const char *filename)
{
	CArrayInit(descs, sizeof(const GunDescription));
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		printf("Error: cannot load guns file %s\n", filename);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing guns file %s\n", filename);
		goto bail;
	}
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read guns file version");
		goto bail;
	}

	GunDescription defaultDesc;
	LoadGunDescription(
		&defaultDesc, json_find_first_label(root, "DefaultGun")->child, NULL);
	for (int i = 0; i < GUN_COUNT; i++)
	{
		CArrayPushBack(descs, &defaultDesc);
	}
	json_t *gunsNode = json_find_first_label(root, "Guns")->child;
	for (json_t *child = gunsNode->child; child; child = child->next)
	{
		GunDescription g;
		LoadGunDescription(&g, child, &defaultDesc);
		int idx = -1;
		LoadInt(&idx, child, "Index");
		if (idx >= 0 && idx < GUN_COUNT)
		{
			memcpy(CArrayGet(descs, idx), &g, sizeof g);
		}
	}
	json_t *pseudoGunsNode = json_find_first_label(root, "PseudoGuns")->child;
	for (json_t *child = pseudoGunsNode->child; child; child = child->next)
	{
		GunDescription g;
		LoadGunDescription(&g, child, &defaultDesc);
		g.IsRealGun = false;
		CArrayPushBack(descs, &g);
	}

bail:
	json_free_value(&root);
	if (f)
	{
		fclose(f);
	}
}
static void LoadGunDescription(
	GunDescription *g, json_t *node, const GunDescription *defaultGun)
{
	memset(g, 0, sizeof *g);
	if (defaultGun)
	{
		memcpy(g, defaultGun, sizeof *g);
		if (defaultGun->name)
		{
			CSTRDUP(g->name, defaultGun->name);
		}
		g->MuzzleHeight /= Z_FACTOR;
	}
	char *tmp;

	if (json_find_first_label(node, "Pic"))
	{
		tmp = GetString(node, "Pic");
		if (strcmp(tmp, "blaster") == 0)
		{
			g->pic = GUNPIC_BLASTER;
		}
		else if (strcmp(tmp, "knife") == 0)
		{
			g->pic = GUNPIC_KNIFE;
		}
		else
		{
			g->pic = -1;
		}
		CFREE(tmp);
	}

	g->name = GetString(node, "Name");

	if (json_find_first_label(node, "Bullet"))
	{
		tmp = GetString(node, "Bullet");
		g->Bullet = StrBulletType(tmp);
		CFREE(tmp);
	}

	LoadInt(&g->Cost, node, "Cost");

	LoadInt(&g->Lock, node, "Lock");

	LoadInt(&g->ReloadLead, node, "ReloadLead");

	if (json_find_first_label(node, "Sound"))
	{
		tmp = GetString(node, "Sound");
		g->Sound = StrSound(tmp);
		CFREE(tmp);
	}

	if (json_find_first_label(node, "ReloadSound"))
	{
		tmp = GetString(node, "ReloadSound");
		g->ReloadSound = StrSound(tmp);
		CFREE(tmp);
	}

	LoadInt(&g->SoundLockLength, node, "SoundLockLength");

	LoadDouble(&g->Recoil, node, "Recoil");

	LoadInt(&g->Spread.Count, node, "SpreadCount");
	LoadDouble(&g->Spread.Width, node, "SpreadWidth");
	LoadDouble(&g->AngleOffset, node, "AngleOffset");

	LoadInt(&g->MuzzleHeight, node, "MuzzleHeight");
	g->MuzzleHeight *= Z_FACTOR;
	LoadInt(&g->ElevationLow, node, "ElevationLow");
	LoadInt(&g->ElevationHigh, node, "ElevationHigh");
	if (json_find_first_label(node, "MuzzleFlashParticle"))
	{
		tmp = GetString(node, "MuzzleFlashParticle");
		g->MuzzleFlash = ParticleClassGet(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	if (json_find_first_label(node, "Brass"))
	{
		tmp = GetString(node, "Brass");
		g->Brass = ParticleClassGet(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	LoadBool(&g->CanShoot, node, "CanShoot");

	LoadInt(&g->ShakeAmount, node, "ShakeAmount");

	g->IsRealGun = true;
}
void WeaponTerminate(CArray *descs)
{
	for (int i = 0; i < (int)descs->size; i++)
	{
		GunDescription *g = CArrayGet(descs, i);
		CFREE(g->name);
	}
	CArrayTerminate(descs);
}

Weapon WeaponCreate(const GunDescription *gun)
{
	Weapon w;
	w.Gun = gun;
	w.state = GUNSTATE_READY;
	w.lock = 0;
	w.soundLock = 0;
	w.stateCounter = -1;
	return w;
}

// TODO: use map structure?
const GunDescription *StrGunDescription(const char *s)
{
	for (int i = 0; i < (int)gGunDescriptions.size; i++)
	{
		const GunDescription *g = CArrayGet(&gGunDescriptions, i);
		if (strcmp(s, g->name) == 0)
		{
			return g;
		}
	}
	CASSERT(false, "cannot parse gun name");
	return CArrayGet(&gGunDescriptions, 0);
}

void WeaponSetState(Weapon *w, gunstate_e state);

static void AddBrass(
	const GunDescription *g, const direction_e d, const Vec2i pos);
void WeaponUpdate(
	Weapon *w, const int ticks, const Vec2i fullPos, const direction_e d)
{
	// Reload sound
	if (gConfig.Sound.Reloads &&
		w->lock > w->Gun->ReloadLead &&
		w->lock - ticks <= w->Gun->ReloadLead &&
		w->lock > 0 &&
		w->Gun->ReloadSound != NULL)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice,
			w->Gun->ReloadSound,
			Vec2iFull2Real(fullPos),
			RELOAD_DISTANCE_PLUS);
		// Brass shells
		if (w->Gun->Brass)
		{
			AddBrass(w->Gun, d, fullPos);
		}
	}
	w->lock -= ticks;
	if (w->lock < 0)
	{
		w->lock = 0;
	}
	w->soundLock -= ticks;
	if (w->soundLock < 0)
	{
		w->soundLock = 0;
	}
	if (w->stateCounter >= 0)
	{
		w->stateCounter = MAX(0, w->stateCounter - ticks);
		if (w->stateCounter == 0)
		{
			switch (w->state)
			{
			case GUNSTATE_FIRING:
				WeaponSetState(w, GUNSTATE_RECOIL);
				break;
			case GUNSTATE_RECOIL:
				WeaponSetState(w, GUNSTATE_READY);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
}

int WeaponCanFire(Weapon *w)
{
	return w->lock <= 0;
}

static bool GunHasMuzzle(const GunDescription *desc);
void WeaponFire(Weapon *w, direction_e d, Vec2i pos, int flags, int player)
{
	if (w->state != GUNSTATE_FIRING && w->state != GUNSTATE_RECOIL)
	{
		WeaponSetState(w, GUNSTATE_FIRING);
	}
	if (!w->Gun->CanShoot)
	{
		return;
	}

	CASSERT(WeaponCanFire(w), "Can't fire weapon");

	const double radians = dir2radians[d];
	const Vec2i muzzleOffset = GunGetMuzzleOffset(w->Gun, d);
	const Vec2i muzzlePosition = Vec2iAdd(pos, muzzleOffset);
	const bool playSound = w->soundLock <= 0;
	GunAddBullets(
		w->Gun, muzzlePosition, w->Gun->MuzzleHeight, radians,
		flags, player, playSound);
	if (playSound)
	{
		w->soundLock = w->Gun->SoundLockLength;
	}

	// Brass shells
	// If we have a reload lead, defer the creation of shells until then
	if (w->Gun->Brass && w->Gun->ReloadLead == 0)
	{
		AddBrass(w->Gun, d, pos);
	}

	w->lock = w->Gun->Lock;
}

void GunAddBullets(
	const GunDescription *g, const Vec2i fullPos, const int z,
	const double radians,
	const int flags, const int player,
	const bool playSound)
{
	const int spreadCount = g->Spread.Count;
	double spreadStartAngle = g->AngleOffset;
	const double spreadWidth = g->Spread.Width;
	if (spreadCount > 1)
	{
		// Find the starting angle of the spread (clockwise)
		// Keep in mind the fencepost problem, i.e. spread of 3 means a
		// total spread angle of 2x width
		spreadStartAngle += -(spreadCount - 1) * spreadWidth / 2;
	}

	for (int i = 0; i < spreadCount; i++)
	{
		double spreadAngle = spreadStartAngle + i * spreadWidth;
		double recoil = 0;
		if (g->Recoil > 0)
		{
			recoil =
				((double)rand() / RAND_MAX * g->Recoil) - g->Recoil / 2;
		}
		double finalAngle = radians + spreadAngle + recoil;
		GameEvent e;
		memset(&e, 0, sizeof e);
		e.Type = GAME_EVENT_ADD_BULLET;
		e.u.AddBullet.Bullet = g->Bullet;
		e.u.AddBullet.MuzzlePos = fullPos;
		e.u.AddBullet.MuzzleHeight = z;
		e.u.AddBullet.Angle = finalAngle;
		e.u.AddBullet.Elevation = RAND_INT(g->ElevationLow, g->ElevationHigh);
		e.u.AddBullet.Flags = flags;
		e.u.AddBullet.PlayerIndex = player;
		GameEventsEnqueue(&gGameEvents, e);
		if (GunHasMuzzle(g))
		{
			memset(&e, 0, sizeof e);
			e.Type = GAME_EVENT_ADD_PARTICLE;
			e.u.AddParticle.Class = g->MuzzleFlash;
			e.u.AddParticle.FullPos = fullPos;
			e.u.AddParticle.Z = z;
			e.u.AddParticle.Angle = radians;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
	if (playSound && g->Sound)
	{
		SoundPlayAt(&gSoundDevice, g->Sound, Vec2iFull2Real(fullPos));
	}
	if (g->ShakeAmount > 0)
	{
		GameEvent shake;
		shake.Type = GAME_EVENT_SCREEN_SHAKE;
		shake.u.ShakeAmount = g->ShakeAmount;
		GameEventsEnqueue(&gGameEvents, shake);
	}
}

static void AddBrass(
	const GunDescription *g, const direction_e d, const Vec2i pos)
{
	CASSERT(g->Brass, "Cannot create brass for no-brass weapon");
	GameEvent e;
	memset(&e, 0, sizeof e);
	e.Type = GAME_EVENT_ADD_PARTICLE;
	e.u.AddParticle.Class = g->Brass;
	double x, y;
	const double radians = dir2radians[d];
	GetVectorsForRadians(radians, &x, &y);
	const Vec2i ejectionPortOffset = Vec2iReal2Full(Vec2iScale(Vec2iNew(
		(int)round(x), (int)round(y)), 7));
	const Vec2i muzzleOffset = GunGetMuzzleOffset(g, d);
	const Vec2i muzzlePosition = Vec2iAdd(pos, muzzleOffset);
	e.u.AddParticle.FullPos = Vec2iMinus(muzzlePosition, ejectionPortOffset);
	e.u.AddParticle.Z = g->MuzzleHeight;
	e.u.AddParticle.Vel = Vec2iScaleDiv(
		GetFullVectorsForRadians(radians + PI / 2), 3);
	e.u.AddParticle.Vel.x += (rand() % 128) - 64;
	e.u.AddParticle.Vel.y += (rand() % 128) - 64;
	e.u.AddParticle.Angle = RAND_DOUBLE(0, PI * 2);
	e.u.AddParticle.DZ = (rand() % 6) + 6;
	e.u.AddParticle.Spin = RAND_DOUBLE(-0.1, 0.1);
	GameEventsEnqueue(&gGameEvents, e);
}

Vec2i GunGetMuzzleOffset(const GunDescription *desc, const direction_e dir)
{
	if (!GunHasMuzzle(desc))
	{
		return Vec2iZero();
	}
	gunpic_e g = desc->pic;
	CASSERT(g >= 0, "Gun has no pic");
	int body = (int)g < 0 ? BODY_UNARMED : BODY_ARMED;
	Vec2i position = Vec2iNew(
		cGunHandOffset[body][dir].dx +
		cGunPics[g][dir][GUNSTATE_FIRING].dx +
		cMuzzleOffset[g][dir].dx,
		cGunHandOffset[body][dir].dy +
		cGunPics[g][dir][GUNSTATE_FIRING].dy +
		cMuzzleOffset[g][dir].dy + BULLET_Z);
	return Vec2iReal2Full(position);
}

void WeaponHoldFire(Weapon *w)
{
	WeaponSetState(w, GUNSTATE_READY);
}

void WeaponSetState(Weapon *w, gunstate_e state)
{
	w->state = state;
	switch (state)
	{
	case GUNSTATE_FIRING:
		w->stateCounter = 8;
		break;
	case GUNSTATE_RECOIL:
		// This is to make sure the gun stays recoiled as long as the gun is
		// "locked", i.e. cannot fire
		w->stateCounter = w->lock;
		break;
	default:
		w->stateCounter = -1;
		break;
	}
}

static bool GunHasMuzzle(const GunDescription *desc)
{
	return desc->pic == GUNPIC_BLASTER;
}
bool IsHighDPS(const GunDescription *g)
{
	const BulletClass *b = &gBulletClasses[g->Bullet];
	// TODO: generalised way of determining explosive bullets
	return
		b->Falling.DropGuns.size > 0 ||
		b->OutOfRangeGuns.size > 0 ||
		b->HitGuns.size > 0;
}
static int GetEffectiveRange(const GunDescription *g)
{
	const BulletClass *b = &gBulletClasses[g->Bullet];
	const int speed = (b->SpeedLow + b->SpeedHigh) / 2;
	const int range = (b->RangeLow + b->RangeHigh) / 2;
	int effectiveRange = speed * range;
	if (b->Falling.GravityFactor != 0 && b->Falling.DestroyOnDrop)
	{
		// Halve effective range
		// TODO: this assumes a certain bouncing range
		effectiveRange /= 2;
	}
	return effectiveRange / 256;
}
bool IsLongRange(const GunDescription *g)
{
	return GetEffectiveRange(g) > 130;
}
bool IsShortRange(const GunDescription *g)
{
	return GetEffectiveRange(g) < 100;
}
