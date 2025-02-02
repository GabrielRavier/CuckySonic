#include <stdint.h>
#include "../Endianness.h"
#include "../Level.h"
#include "../LevelCollision.h"
#include "../Game.h"
#include "../Log.h"
#include "../Objects.h"
#include "../Audio.h"
#include "../MathUtil.h"

//#define BOUNCINGRING_BLINK	//When set, the rings will blink shortly before it despawns

static const uint8_t animationCollect[] =	{0x05,0x04,0x05,0x06,0x07,0xFC};

static const uint8_t *animationList[] = {
	animationCollect,
};

enum SCRATCH
{
	//U8
	SCRATCHU8_ANIM_COUNT = 0,
	SCRATCHU8_MAX = 1,
	//U16
	SCRATCHU16_ANIM_ACCUM = 0,
	SCRATCHU16_MAX = 1,
};

void ObjBouncingRing_Routine0(OBJECT *object)
{
	//Cap our rings
	unsigned int *rings = &gRings;
	if (*rings >= 32)
		*rings = 32;
	
	//Spawn the given amount of rings
	union
	{
		#ifdef CPU_BIGENDIAN
			struct
			{
				uint8_t speed;
				uint8_t angle;
			} asInd;
		#else
			struct
			{
				uint8_t angle;
				uint8_t speed;
			} asInd;
		#endif
		
		int16_t angleSpeed = 0x288;
	};
	
	int16_t xVel, yVel;
	for (unsigned int i = 0; i < *rings; i++)
	{
		//Create the ring object
		OBJECT *ring = new OBJECT(&gLevel->objectList, &ObjBouncingRing);
		ring->routine = 1;
		ring->xRadius = 8;
		ring->yRadius = 8;
		ring->widthPixels = 8;
		ring->x.pos = object->x.pos;
		ring->y.pos = object->y.pos;
		ring->texture = gLevel->GetObjectTexture("data/Object/Ring.bmp");
		ring->mappings = gLevel->GetObjectMappings("data/Object/Ring.map");
		ring->renderFlags.xFlip = false;
		ring->renderFlags.yFlip = false;
		ring->renderFlags.alignPlane = true;
		ring->priority = 3;
		ring->collisionType = COLLISIONTYPE_OTHER;
		ring->touchWidth = 6;
		ring->touchHeight = 6;
		
		ring->ScratchAllocU8(SCRATCHU8_MAX);
		ring->ScratchAllocU16(SCRATCHU16_MAX);
		ring->scratchU8[SCRATCHU8_ANIM_COUNT] = 0xFF;
		ring->scratchU16[SCRATCHU16_ANIM_ACCUM] = 0x00;
		
		ring->parent = object->parent;
		
		//Get the ring's velocity
		if (angleSpeed >= 0)
		{
			//Get this velocity
			int16_t sin, cos;
			GetSine(asInd.angle, &sin, &cos);
			
			xVel = sin * (1 << asInd.speed);
			yVel = cos * (1 << asInd.speed);
			
			//Get the next angle and speed
			asInd.angle += 0x10;
			
			if (asInd.angle < 0x10)
			{
				angleSpeed -= 0x80;
				if (angleSpeed < 0)
					angleSpeed = 0x288;
			}
		}
		
		//Set the ring's velocity
		ring->xVel = xVel;
		ring->yVel = yVel;
		
		xVel = -xVel;
		angleSpeed = -angleSpeed;
	}
	
	//Clear our rings and delete the spawner
	PlaySound(SOUNDID_RING_LOSS);
	*rings = 0;
	object->deleteFlag = true;
}

void ObjBouncingRing(OBJECT *object)
{
	//Allocate scratch memory
	object->ScratchAllocU8(SCRATCHU8_MAX);
	object->ScratchAllocU16(SCRATCHU16_MAX);
	
	switch (object->routine)
	{
		case 0:
		{
			ObjBouncingRing_Routine0(object);
			break;
		}
		case 1:
		{
			//Move and fall
			PLAYER *parentPlayer = (PLAYER*)object->parent;
			object->xPosLong += object->xVel * 0x100;
			
			if (parentPlayer->status.reverseGravity)
				object->yPosLong -= object->yVel * 0x100;
			else
				object->yPosLong += object->yVel * 0x100;
			
			object->yVel += 0x18;
			
			//Check for collision with the floor or ceiling
			int16_t checkVel = object->yVel;
			if (parentPlayer->status.reverseGravity)
				checkVel = -checkVel;
			
			if (checkVel >= 0)
			{
				int16_t distance = FindFloor(object->x.pos, object->y.pos + object->yRadius, COLLISIONLAYER_NORMAL_TOP, false, nullptr);
				
				//If touching the floor, bounce off
				if (distance < 0)
				{
					object->y.pos += distance;
					object->yVel = object->yVel * 3 / -4;
				}
			}
			else
			{
				int16_t distance = FindFloor(object->x.pos, object->y.pos - object->yRadius, COLLISIONLAYER_NORMAL_LRB, true, nullptr);
				
				//If touching a ceiling, bounce off
				if (distance < 0)
				{
					object->y.pos -= distance;
					object->yVel = -object->yVel;
				}
			}
			
			//Check for collision with walls
			if (object->xVel > 0)
			{
				int16_t distance = FindWall(object->x.pos + object->xRadius, object->y.pos, COLLISIONLAYER_NORMAL_LRB, false, nullptr);
				
				//If touching a wall, bounce off
				if (distance < 0)
				{
					object->x.pos += distance;
					object->xVel = object->xVel / -2;
				}
			}
			else if (object->xVel < 0)
			{
				int16_t distance = FindWall(object->x.pos - object->xRadius, object->y.pos, COLLISIONLAYER_NORMAL_LRB, true, nullptr);
				
				//If touching a wall, bounce off
				if (distance < 0)
				{
					object->x.pos -= distance;
					object->xVel = object->xVel / -2;
				}
			}
			
			//Animate
			if (object->scratchU8[SCRATCHU8_ANIM_COUNT] != 0)
			{
				object->scratchU16[SCRATCHU16_ANIM_ACCUM] += object->scratchU8[SCRATCHU8_ANIM_COUNT]--;
				object->mappingFrame = (object->scratchU16[SCRATCHU16_ANIM_ACCUM] >> 9) & 0x3;
			}
			
			//Check for deletion
			if (object->scratchU8[SCRATCHU8_ANIM_COUNT] == 0)
				object->deleteFlag = true;
			else
			#ifdef BOUNCINGRING_BLINK
				if (object->scratchU8[SCRATCHU8_ANIM_COUNT] > 60 || gLevel->frameCounter & (object->scratchU8[SCRATCHU8_ANIM_COUNT] > 30 ? 0x4 : 0x2))
			#endif
					object->DrawInstance(object->renderFlags, object->texture, object->mappings, object->highPriority, object->priority, object->mappingFrame, object->x.pos, object->y.pos);
			break;
		}
		case 2: //Touched player, collect a ring
		{
			//Increment routine
			object->routine++;
			
			//Clear collision and change priority
			object->collisionType = COLLISIONTYPE_NULL;
			object->priority = 1;
			
			//Collect the ring
			AddToRings(1);
		}
	//Fallthrough
		case 3: //Sparkling
		{
			object->Animate(animationList);
			object->DrawInstance(object->renderFlags, object->texture, object->mappings, object->highPriority, object->priority, object->mappingFrame, object->x.pos, object->y.pos);
			break;
		}
		case 4: //Deleting after sparkle
		{
			object->deleteFlag = true;
			break;
		}
	}
}
