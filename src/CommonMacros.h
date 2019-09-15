#pragma once
#include "SDL_endian.h"

#if (SDL_BYTEORDER == SDL_BIGENDIAN)
	#define POSDEF(axis)	\
		union	\
		{	\
			struct	\
			{	\
				int16_t pos;	\
				uint16_t sub;	\
			} axis;	\
			int32_t	axis##PosLong;	\
		};
#else
	#define POSDEF(axis)	\
		union	\
		{	\
			struct	\
			{	\
				uint16_t sub;	\
				int16_t pos;	\
			} axis;	\
			int32_t	axis##PosLong;	\
		};
#endif