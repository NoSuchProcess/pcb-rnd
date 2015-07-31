#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "global.h"
#include "hid.h"
#include "resource.h"
#include "hid/common/hid_resource.h"

/* #define DEBUG_HID_RESOURCE */

static int button_count;   // number of buttons we have actions for
static int *button_nums;   // list of button numbers
static int *mod_count;     // how many mods they have
static unsigned *mods;     // mods, in order, one button after another
static Resource** actions; // actions, in order, one button after another

static Resource *
res_wrap (char *value)
{
  Resource *tmp;
  tmp = resource_create (0);
  resource_add_val (tmp, 0, value, 0);
  return tmp;
}

static unsigned
parse_mods (char *value)
{
  unsigned m = 0;
  long int mod_num;
  char *s;

  for (s=value; *s; s++)
    *s = tolower(*s);

  s = strstr(value, "mod");
  if (s)
    {
      s += 3; // skip "mod" to get to number
      errno = 0;
      mod_num = strtol(s, (char**) NULL, 0);
      if (!errno)
        m |= M_Mod(mod_num);
    }
  if (strstr(value, "shift"))
    m |= M_Shift;
  if (strstr(value, "ctrl"))
    m |= M_Ctrl;
  if (strstr(value, "alt"))
    m |= M_Alt;
  if (strstr(value, "up"))
    m |= M_Release;
  return m;
}

static int
button_name_to_num (const char *name)
{
  /* All mouse-related resources must be named.  The name is the
     mouse button number.  */
  if (!name)
    return -1;
  else if (strcasecmp (name, "left") == 0)
    return 1;
  else if (strcasecmp (name, "middle") == 0)
    return 2;
  else if (strcasecmp (name, "right") == 0)
    return 3;
  else if (strcasecmp (name, "up") == 0)
    return 4;
  else if (strcasecmp (name, "down") == 0)
    return 5;
  else
    return atoi (name);
}

void
load_mouse_resource (const Resource *res)
{
  int bi, mi, a;
  int action_count;
#ifdef DEBUG_HID_RESOURCE
  fprintf(stderr, "note mouse resource:\n");
  resource_dump (res);
#endif

  button_count = res->c;
  button_nums = (int *)malloc(res->c * sizeof(int));
  mod_count = (int *)malloc(res->c * sizeof(int));
  action_count = 0;
  for (bi=0; bi<res->c; bi++)
    {
      if (res->v[bi].value)
        action_count++;

      if (res->v[bi].subres)
        action_count += res->v[bi].subres->c;

    }
  mods = (unsigned int *)malloc(action_count * sizeof(int));
  actions = (Resource **)malloc(action_count * sizeof(Resource*));

  a = 0;
  for (bi=0; bi<res->c; bi++)
    {
      int button_num = button_name_to_num(res->v[bi].name);

      if (button_num < 0)
	continue;

      button_nums[bi] = button_num;
      mod_count[bi] = 0;

      if (res->v[bi].value)
        {
          mods[a] = 0;
          actions[a++] = res_wrap (res->v[bi].value);         
          mod_count[bi] = 1;
        }

      if (res->v[bi].subres)
	{
	  Resource *m = res->v[bi].subres;
          mod_count[bi] += m->c;

	  for (mi=0; mi<m->c; mi++, a++)
	    {
	      switch (resource_type (m->v[mi]))
		{
		case 1: /* subres only */
                  mods[a] = 0;
                  actions[a] = m->v[mi].subres;
		  break;

		case 10: /* value only */
                  mods[a] = 0;
                  actions[a] = res_wrap (m->v[mi].value);
		  break;

		case 101: /* name = subres */
		  mods[a] = parse_mods (m->v[mi].name);
		  actions[a] = m->v[mi].subres;
		  break;

		case 110: /* name = value */
		  mods[a] = parse_mods (m->v[mi].name);
		  actions[a] = res_wrap (m->v[mi].value);
		  break;
		}
	    }
	}
    }
}

static Resource*
find_best_action (int button, int start, unsigned mod_mask)
{
  int i, j;
  int count = mod_count[button];
  unsigned search_mask = mod_mask & ~M_Release;
  unsigned release_mask = mod_mask & M_Release;

  // look for exact mod match
  for (i=start; i<start+count; i++)
    if (mods[i] == mod_mask) {
      return actions[i];
    }

  for (j=search_mask-1; j>=0; j--)
    if ((j & search_mask) == j) // this would work
      for (i=start; i<start+count; i++) // search for it
        if (mods[i] == (j | release_mask))
          return actions[i];

  return NULL;
}

void
do_mouse_action (int button, int mod_mask)
{
  Resource *action = NULL;
  int bi, i, a;

  // find the right set of actions;
  a = 0;
  for (bi=0; bi<button_count && !action; bi++)
    if (button_nums[bi] == button)
      {
        action = find_best_action(bi, a, mod_mask);
        break;
      }
    else
      a += mod_count[bi]; // skip this buttons actions

  if (!action)
    return;

  for (i = 0; i < action->c; i++)
    if (action->v[i].value)
      if (hid_parse_actions (action->v[i].value))
        return;
}

Resource *resource_create_menu(const char *name, const char *action, const char *mnemonic, const char *accel, const char *tip, int main_menu)
{
	Resource *resp, *res;
	ResourceVal *rvp, *rv;
	int next;

	/* child */
	res = calloc(sizeof(Resource), 1);
	res->c = 1 + (action != NULL) + (mnemonic != NULL) + (accel != NULL) + (tip != NULL);
	rv = malloc(sizeof(ResourceVal) * res->c);
	res->v = rv;
	if (main_menu)
		res->flags = /*FLAG_S | FLAG_NV | FLAG_V*/ 7;
	else
		res->flags = FLAG_NS | FLAG_NV | FLAG_V;


	rv[0].name = NULL;
	rv[0].value = strdup(name);
	rv[0].subres = NULL;
	next = 1;

	if (action != NULL) {
		rv[next].name = NULL;
		rv[next].value = strdup(action);
		rv[next].subres = NULL;
		next++;
	}

	if (mnemonic != NULL) {
		rv[next].name = "m";
		rv[next].value = strdup(mnemonic);
		rv[next].subres = NULL;
		next++;
	}

	if (accel != NULL) {
		rv[next].name = "a";
		rv[next].value = strdup(accel);
		rv[next].subres = NULL;
		next++;
	}

	if (tip != NULL) {
		rv[next].name = "tip";
		rv[next].value = strdup(tip);
		rv[next].subres = NULL;
		next++;
	}

	/* parent */
	resp = calloc(sizeof(Resource), 1);
	rvp = malloc(sizeof(ResourceVal));

	resp->v = rvp;
	resp->c = 1;
	rvp->name = NULL;
	rvp->value = NULL;
	rvp->subres = res;

	return resp;
}
