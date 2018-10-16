#ifndef _NODESERIALIZER_H
#define _NODESERIALIZER_H

#include <vector>
#include "audionode.h"
#include "nodelink.h"

void serialize(std::ostream &dest, std::vector<AudioNode*> &nodes, std::vector<NodeLink> &links);

#endif
