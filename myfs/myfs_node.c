#include "myfs_node.h"

inline struct myfs_node * itomyfs(const struct inode *inode)
{
	return container_of(inode, struct myfs_node, vfs_inode);
}

inline int myfs_is_name_valid(const struct dentry *dentry)
{
	return dentry->d_name.len <= MYFS_NAME_MAX;
}

inline int myfs_is_dir_empty(const struct myfs_node *dir)
{
	return list_empty(&dir->dir.children);
}

struct inode * myfs_create_root(struct super_block *sb)
{
	struct inode *root = new_inode(sb);
	
	if (!root) {
		return ERR_PTR(-ENOMEM);
	}

	struct myfs_node *myfs_root = itomyfs(root);

	root->i_ino = get_next_ino();
	root->i_sb = sb;
	root->i_op = &myfs_dir_iops;
	root->i_fop = &myfs_dir_fops;
	root->i_mode = S_IFDIR | 0777;

	INIT_LIST_HEAD(&myfs_root->dir.children);
	myfs_root->size = 0;

	set_nlink(root, 2);

	return root;
}

struct myfs_node * myfs_new_node(struct myfs_node *myfs_parent, struct dentry *dentry, umode_t mode)
{
	struct inode *parent = &myfs_parent->vfs_inode;
	struct inode *inode = new_inode(parent->i_sb);
	struct myfs_node *myfs_node;

	if (!inode) {
		return ERR_PTR(-ENOMEM);
	}

	myfs_node = itomyfs(inode);
	strscpy(myfs_node->name, dentry->d_name.name, MYFS_NAME_MAX);
	myfs_node->namelen = dentry->d_name.len;
	myfs_node->size = 0;

	inode->i_ino = get_next_ino();
	inode->i_sb = parent->i_sb;
	inode->i_mode = mode;
	
	if (S_ISREG(mode)) {
	
	}
	else if (S_ISDIR(mode)) {
		inode->i_op = &myfs_dir_iops;
		inode->i_fop = &myfs_dir_fops;
		INIT_LIST_HEAD(&myfs_node->dir.children);
	}

	return myfs_node;
}

inline void myfs_node_link(struct myfs_node *parent, struct myfs_node *node)
{
	list_add_tail(&node->list, &parent->dir.children);
	++parent->size;
}

inline void myfs_node_unlink(struct myfs_node *parent, struct myfs_node *node)
{
	list_del_init(&node->list);
	--parent->size;
}

struct myfs_node * myfs_node_lookup(struct myfs_node *parent, struct dentry *dentry)
{
	const char *lookup_name = dentry->d_name.name;
	struct myfs_node *node;

	list_for_each_entry(node, &parent->dir.children, list) {
		if (!strncmp(lookup_name, node->name, MYFS_NAME_MAX)) {
			return node;		
		}
	}

	return NULL;
}


