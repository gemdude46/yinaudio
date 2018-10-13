#ifndef _NODELINK_H
#define _NODELINK_H

#include "audionode.h"

struct NodeLink {
	AudioNode* src;
	int src_id;

	AudioNode* dest;
	int dest_id;
};

#endif
