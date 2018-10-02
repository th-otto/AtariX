/*
 * NatFeats (Native Features main dispatcher)
 */

#include "config.h"
#include "Globals.h"
#include "m68kcpu.h"
#include "natfeat.h"
#include "nf_objs.h"

#define ID_SHIFT	20
#define IDX2MASTERID(idx)	(((idx)+1) << ID_SHIFT)
#define MASTERID2IDX(id)	(((id) >> ID_SHIFT)-1)
#define MASKOUTMASTERID(id)	((id) & ((1L << ID_SHIFT)-1))


uint32 nf_get_id(uint32 args)
{
	uint32 name_ptr = m68ki_read_32(args);
	char name[1024];
	unsigned int i;
	
	atari2HostSafeStrncpy(name, name_ptr, sizeof(name));

	for (i = 0; i < nf_objs_cnt; i++)
	{
		if (strcasecmp(name, nf_objects[i]->name) == 0)
		{
			return IDX2MASTERID(i);
		}
	}

	return 0;							/* ID with given name not found */
}


sint32 nf_call(uint32 args)
{
	uint32 fncode = m68ki_read_32(args);
	unsigned int idx = MASTERID2IDX(fncode);
	pNatFeat obj;
	
	if (idx >= nf_objs_cnt)
	{
		return 0;						/* returning an undefined value */
	}

	fncode = MASKOUTMASTERID(fncode);
	args += 4;							/* parameters follow on the stack */

	obj = nf_objects[idx];

	if (obj->isSuperOnly && !FLAG_S)
	{
		m68ki_exception_privilege_violation();				/* privilege exception */
		return 0;
	}

	return obj->dispatch(fncode, args);
}
