#include <linux/module.h>
#include <linux/statfs.h>
#include "myfs_node.h"

#define MYFS_NAME "myfs"
#define MYFS_MAGIC_NUMBER 0x1234
#define MYFS_MAX_FILE_SIZE 512
#define MYFS_BLOCK_SIZE_BITS 9


static struct kmem_cache *myfs_inode_cachep;

static void myfs_inode_init_once(void *foo)
{
	struct myfs_node *node = (struct myfs_node *)foo;
	inode_init_once(&node->vfs_inode);
}

static int myfs_init_inodecache(void)
{
	myfs_inode_cachep = kmem_cache_create("myfs_inode_cache", sizeof(struct myfs_node), 0, 0, myfs_inode_init_once); 
	
	if (!myfs_inode_cachep) {
		return -ENOMEM;
	}

	return 0;
}

static void myfs_destroy_inodecache(void)
{
	kmem_cache_destroy(myfs_inode_cachep);
}

static int myfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	u64 id = huge_encode_dev(dentry->d_sb->s_dev);
	buf->f_fsid = u64_to_fsid(id);
	buf->f_type = dentry->d_sb->s_magic;
	buf->f_bsize = dentry->d_sb->s_blocksize;
	buf->f_namelen = MYFS_NAME_MAX;
	
	return 0;
}

static struct inode * myfs_alloc_inode(struct super_block *sb)
{
	struct myfs_node *node = alloc_inode_sb(sb, myfs_inode_cachep, GFP_KERNEL);
	
	if (!node) {
		return NULL;
	}	

	return &node->vfs_inode;
}

static void myfs_destroy_inode(struct inode *inode)
{
	kmem_cache_free(myfs_inode_cachep, itomyfs(inode));
}

static const struct super_operations myfs_ops = {
	.statfs = myfs_statfs,
	.drop_inode = generic_delete_inode,
	.alloc_inode = myfs_alloc_inode,
	.destroy_inode = myfs_destroy_inode,
};

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	sb->s_maxbytes = MYFS_MAX_FILE_SIZE;
	sb->s_blocksize_bits = MYFS_BLOCK_SIZE_BITS;
	sb->s_blocksize = 1 << sb->s_blocksize_bits;
	sb->s_magic = MYFS_MAGIC_NUMBER;
	sb->s_op = &myfs_ops;
	sb->s_time_gran = 1;
	
	struct inode *root = myfs_create_root(sb);
	
	if (IS_ERR(root)) {
		return PTR_ERR(root);
	}

	sb->s_root = d_make_root(root);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static struct dentry * myfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

static struct file_system_type myfs_type = {
	.name = MYFS_NAME,
	.owner = THIS_MODULE,
	.mount = myfs_mount,
	.kill_sb = kill_anon_super,
};

static struct dentry * myfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
	return mount_nodev(&myfs_type, flags, data, myfs_fill_super);
}

static int __init init_myfs(void)
{
	int ret = 0;

	ret = myfs_init_inodecache();
	if (ret) {
		return ret;
	}
	ret = register_filesystem(&myfs_type);
	if (ret) {
		myfs_destroy_inodecache();
	}

	return ret;
}

static void __exit exit_myfs(void)
{
	unregister_filesystem(&myfs_type);
	myfs_destroy_inodecache();
}

module_init(init_myfs);
module_exit(exit_myfs);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MyFS in-memory file system");
MODULE_AUTHOR("intaklunik");
