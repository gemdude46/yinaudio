#ifndef _PARSER_H
#define _PARSER_H

#include <string>
#include <vector>
#include "audionode.h"

int parse_code(std::string code, std::vector<AudioNode*> &nodes);

#endif
