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

    Copyright (c) 2013-2014, 2016 Cong Xu
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
#pragma once

#include "actors.h"
#include "draw_buffer.h"
#include "gamedata.h"
#include "grafx_bg.h"

typedef enum
{
	BODY_PART_HEAD,
	BODY_PART_BODY,
	BODY_PART_GUN
} BodyPart;

typedef struct
{
	const Pic *Head;
	const Pic *Body;
	const Pic *Gun;
	BodyPart DrawOrder[3];
	const CharColors *Colors;
	bool IsDead;
	bool IsDying;
	bool IsTransparent;
	HSV *Tint;
	color_t *Mask;
} ActorPics;

void DrawCharacterSimple(
	Character *c, const Vec2i pos, const direction_e d,
	const bool hilite, const bool showGun);
void DrawHead(
	const Character *c, const direction_e dir,
	const int state, const Vec2i pos);

void DrawChatters(DrawBuffer *b, const Vec2i offset);

void GetCharacterPicsFromActor(ActorPics *pics, TActor *a);
void DrawActorPics(
	const ActorPics *pics, const Vec2i picPos, const direction_e d);
void DrawLaserSight(
	const ActorPics *pics, const TActor *a, const Vec2i picPos);
void DrawActorHighlight(
	const ActorPics *pics, const Vec2i pos, const color_t color,
	const direction_e d);
