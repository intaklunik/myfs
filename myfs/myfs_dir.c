#include "myfs_node.h"


static int myfs_mkdir(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode)
{
	struct myfs_node *myfs_parent = itomyfs(parent);
	struct inode *dir;
	struct myfs_node *myfs_dir = myfs_new_node(myfs_parent, dentry, S_IFDIR | mode);

	if (IS_ERR(myfs_dir)) {
		return PTR_ERR(dir);
	}	

	dir = &myfs_dir->vfs_inode;

       	myfs_node_link(myfs_parent, myfs_dir);
	
	inc_nlink(parent);
	d_instantiate(dentry, dir);
	inc_nlink(dir);

	return 0;
}

static int myfs_rmdir(struct inode *parent, struct dentry *dentry)
{
	struct inode *dir = d_inode(dentry);
	struct myfs_node *myfs_dir = itomyfs(dir); 
	
	if (!myfs_is_dir_empty(myfs_dir)) {
		return -ENOTEMPTY;
	}

	myfs_node_unlink(myfs_dir);

	drop_nlink(parent);
	drop_nlink(dir);

	return 0;
}

static struct dentry * myfs_lookup(struct inode *parent, struct dentry *dentry, unsigned int flags)
{
	if (!myfs_is_name_valid(dentry)) {
		return ERR_PTR(-ENAMETOOLONG);		
	}

	struct myfs_node *myfs_parent = itomyfs(parent);
	struct myfs_node *myfs_node = myfs_node_lookup(myfs_parent, dentry);
	
	if (!myfs_node) {
		d_add(dentry, NULL);
	}
	else {
		struct inode *inode = &myfs_node->vfs_inode;
		d_add(dentry, inode);
	}

	return NULL;
}

const struct inode_operations myfs_dir_iops = { 
	.lookup = myfs_lookup,
	.mkdir = myfs_mkdir,
	.rmdir = myfs_rmdir,
};

const struct file_operations myfs_dir_fops = { 
	.open = generic_file_open,
};

