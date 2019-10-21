#pragma once
#define WINDOW_TITLE	"CuckySonic"
#define FRAMERATE		60

#define GAME_ORGANIZATION	"CuckyDev"
#define GAME_TITLE			"CuckySonic"

/// This defines a macro for unused parameters to mask corresponding warnings, changes the symbol name so it can't be accidentally used
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ UNUSED_ ## x
#else
# define UNUSED(x) UNUSED_ ## x
#endif
