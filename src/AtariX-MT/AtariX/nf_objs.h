#ifndef _NFOBJS_H
#define _NFOBJS_H

#include "nf_base.h"

typedef const NF_Base *pNatFeat;

extern NF_Base const nf_fVDIDrv;
extern NF_Base const nf_xhdi;
extern NF_Base const nf_audio;
extern NF_Base const nf_hostfs;
extern NF_Base const nf_ethernet;
extern NF_Base const nf_cdrom;
extern NF_Base const nf_pci;
extern NF_Base const nf_osmesa;
extern NF_Base const nf_jpeg;
extern NF_Base const nf_scsidrv;
extern NF_Base const nf_xhdi;

extern const NF_Base *const nf_objects[];
extern unsigned int const nf_objs_cnt;

extern void NFReset(void);
extern void NFCreate(void);
extern void NFDestroy(void);

#endif
