#ifndef MYFS_NODE_H
#define MYFS_NODE_H

extern const struct inode_operations myfs_dir_iops;
extern const struct file_operations myfs_dir_fops;

struct myfs_node {
	unsigned int ino;
};

struct myfs_dir {
	struct myfs_node node;
};

int myfs_dir_init(struct myfs_dir **dir);
void myfs_node_free(struct myfs_node *node, unsigned int i_mode);

#endif //MYFS_NODE_H

