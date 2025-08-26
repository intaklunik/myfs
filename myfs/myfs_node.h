#ifndef MYFS_NODE_H
#define MYFS_NODE_H

#include <linux/fs.h>

#define MYFS_NAME_MAX 16

extern const struct inode_operations myfs_dir_iops;
extern const struct file_operations myfs_dir_fops;

struct myfs_file {
	char *buf;
};

struct myfs_dir {
	struct list_head children;
};

struct myfs_node {
	struct inode vfs_inode;
	char name[MYFS_NAME_MAX];
	unsigned int namelen;
	int size;
	union {
		struct myfs_dir dir;
		struct myfs_file file;
	};
	struct list_head list;
};


inline struct myfs_node * itomyfs(const struct inode *inode);
inline int myfs_is_name_valid(const struct dentry *dentry);
inline int myfs_is_dir_empty(const struct myfs_node *dir);

struct inode * myfs_create_root(struct super_block *sb);
struct myfs_node * myfs_new_node(struct myfs_node *myfs_parent, struct dentry *dentry, umode_t mode);
inline void myfs_node_link(struct myfs_node *parent, struct myfs_node *node);
inline void myfs_node_unlink(struct myfs_node *node);
struct myfs_node * myfs_node_lookup(struct myfs_node *parent, struct dentry *dentry);

#endif //MYFS_NODE_H

