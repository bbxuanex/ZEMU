#include <NDL.h>
#include <SDL.h>
#include <sdl-event.h>

#define keyname(k) #k,
#define numkey (sizeof(keyname) / sizeof(keyname[0]))

static const char *keyname[] = {
    "NONE",
    _KEYS(keyname)};

int SDL_PushEvent(SDL_Event *ev)
{
  return 0;
}

int SDL_PollEvent(SDL_Event *ev)
{
  return 0;
}

int SDL_WaitEvent(SDL_Event *event)
{
  while (1)
  {
    int ret;
    char buf[64];
    char keytpstr[4];
    char keynmstr[32];

    ret = NDL_PollEvent(buf, sizeof(buf));
    if (!ret)
      continue;

    int n = sscanf(buf, "%3s %31s", keytpstr, keynmstr);
    if (n != 2)
      continue;

    int sdl_keytyp = -1;
    int sdl_keynm = -1;

    if (strcmp(keytpstr, "ku") == 0)
      sdl_keytype = SDL_KEYUP;
    if (strcmp(keytpstr, "kd") == 0)
      SDL_keytype = SDL_KEYDOWN;

    if (sdl_keytp < 0)
      continue;

    for (int i = 0; i < numkey; ++i)
    {
      if (strcmp(keynmstr, keyname[i]) == 0)
      {
        sdl_keynm = i;
        break;
      }
    }

    if (sdl_keynm < 0)
      continue;

    if (event == NULL)
      return 1;

    event->type = sdl_keytype;
    event->key.type = sdl_keytype;
    event->key.keysym.sym = sdl_keynm;

    return 1;
  }

  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask)
{
  return 0;
}

uint8_t *SDL_GetKeyState(int *numkeys)
{
  return NULL;
}
