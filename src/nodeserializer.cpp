#include "nodeserializer.h"

#include <vector>
#include "audionode.h"
#include "nodelink.h"

void serialize(std::ostream &dest, std::vector<AudioNode*> &nodes, std::vector<NodeLink> &links) {
	for (auto const &node: nodes) {
		node->serialize(dest);
		dest << "name=" << node->name << '\n';
	}

	for (auto const &link: links) {
		dest << link.src->name << '/' << link.src_id << "=>" << link.dest->name << '/' << link.dest_id << '\n';
	}
}
