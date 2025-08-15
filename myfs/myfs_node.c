#include <linux/slab.h>
#include <linux/stat.h>
#include "myfs_node.h"


int myfs_dir_init(struct myfs_dir **dir)
{
	struct myfs_dir *_dir;
	_dir = kzalloc(sizeof(struct myfs_dir), GFP_KERNEL);
	if (!_dir) {
		return -ENOMEM;
	}

	_dir->node.ino = 1;

	*dir = _dir;

	return 0;
}

void myfs_node_free(struct myfs_node *node, unsigned int i_mode)
{
	if (S_ISREG(i_mode)) {
		
	}
	else if (S_ISDIR(i_mode)) {
		struct myfs_dir *dir = container_of(node, struct myfs_dir, node);
		kfree(dir);
	}
}

