#include <linux/module.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include "myfs_node.h"

#define MYFS_NAME "myfs"
#define MYFS_MAGIC_NUMBER 0x1234
#define MYFS_MAX_FILE_SIZE 512
#define MYFS_BLOCK_SIZE_BITS 9
#define MYFS_NAME_MAX 16

static int myfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	u64 id = huge_encode_dev(dentry->d_sb->s_dev);
	buf->f_fsid = u64_to_fsid(id);
	buf->f_type = dentry->d_sb->s_magic;
	buf->f_bsize = dentry->d_sb->s_blocksize;
	buf->f_namelen = MYFS_NAME_MAX;
	return 0;
}

static void myfs_destroy_inode(struct inode *inode)
{
	struct myfs_node *node = inode->i_private;
	myfs_node_free(node, inode->i_mode);
	inode->i_private = NULL;
}

static const struct super_operations myfs_ops = {
	.statfs = myfs_statfs,
	.drop_inode = generic_delete_inode,
	.destroy_inode = myfs_destroy_inode,
};

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	int ret = 0;
	sb->s_maxbytes = MYFS_MAX_FILE_SIZE;
	sb->s_blocksize_bits = MYFS_BLOCK_SIZE_BITS;
	sb->s_blocksize = 1 << sb->s_blocksize_bits;
	sb->s_magic = MYFS_MAGIC_NUMBER;
	sb->s_op = &myfs_ops;
	sb->s_time_gran = 1;
	
	struct myfs_dir *myfs_dir;
       	ret = myfs_dir_init(&myfs_dir);
	if (ret) {
		return ret;
	}
	
	struct inode *inode;
	inode = new_inode(sb);
	inode->i_ino = myfs_dir->node.ino;
	inode->i_sb = sb;
	inode->i_op = &myfs_dir_iops;
	inode->i_fop = &myfs_dir_fops;
	inode->i_mode = S_IFDIR | 0777;
	sb->s_root = d_make_root(inode);
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
	return register_filesystem(&myfs_type);
}

static void __exit exit_myfs(void)
{
	unregister_filesystem(&myfs_type);
}

module_init(init_myfs);
module_exit(exit_myfs);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MyFS in-memory file system");
MODULE_AUTHOR("intaklunik");
