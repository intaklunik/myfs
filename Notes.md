# Notes

## Register and mount file system
Define struct file_system_type, where:
- name - your FS name
- owner - THIS_MODULE or NULL (for in-kernel)
- [mount](#mount) - called during mount operation
- [kill_sb](#kill_sb) - called during unmount operation

#### module_init
0. (Optional) If you implement [inode_cache](#inode_cache), initialize it here.
1. Call register_filesystem(&fs_type)

#### module_exit
0. (Optional) If you implement [inode_cache](#inode_cache), destroy it here.
1. Call unregister_filesystem(&fs_type)

#### mount
mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)  

0. Check mount flags (?)
1. Call one of the functions: ([Read here](https://linux-kernel-labs.github.io/refs/heads/master/labs/filesystems_part1.html))
> **mount_bdev()**, which mounts a file system stored on a block device  
**mount_single()**, which mounts a file system that shares an instance between all mount operations  
**mount_nodev()**, which mounts a file system that is not on a physical device  
**mount_pseudo()**, a helper function for pseudo-file systems (sockfs, pipefs, generally file systems that can not be mounted)

#### kill_sb
kill_sb(struct super_block *sb)
- kill_block_super() - for on-disk FS
- kill_anon_super() - for in-mem FS that keeps its own directory structure
- kill_litter_super() - for in-mem FS that relies on dentry tree as a filesystem tree (calls dget(dentry) ramfs)

## struct super_block
- s_magic - magic number of FS.
- s_maxbytes - maximum file size in FS in bytes.
- s_op - struct super_operations:  
    - [statfs](#statfs)
    - [drop_inode]()
    - [alloc_inode](#alloc_inode)
    - [destroy_inode](#destroy_inode)
- s_root - [root inode](#root-inode)
 
#### statfs
statfs(struct dentry *dentry, struct kstatfs *buf)
Fill in struct kstatfs.  

### fill_super(struct super_block *sb,  void *data,  int silent)
Fill in struct super_block (described above) and initialize [root inode](#root-inode). 

## Inode

### struct super_operations [Read here](https://www.kernel.org/doc/html/next/filesystems/vfs.html)

### inode_cache

### struct inode
- i_ino - inode number. For on-disk FS ino is predefined (you get it from your structures). For in-mem FS can be generated dynamically using get_next_ino(). ino cannot be 0. (?)
- i_sb - pointer to the super_block, parent->i_sb.
- i_mode - file type(S_IFREG, S_IFDIR...) + file permissions.  

|        | File                 | Directory                                   |
|--------|----------------------|-------------------------------------------- |
| read   | read a file          | list files in the dir (ls)                  |
| write  | write to file        | create, delete, rename files in the dir     |
| execute| run file as a program| enter the dir and use it in a file path (cd)|

- i_op - inode_operations. You define 2 sets of i_op: for directories and files.
- i_fop - file_operations. You define 2 sets of i_fop: for directories and files.
- i_nlink - number of hard links. 
    - inc_nlink(inode) - increment i_nlink
    - drop_nlink(inode) - decrement i_nlink
    - set_nlink(inode, N) - set i_nlink = N
    - inode_inc_link_count(inode) - inc_nlink + mark_inode_dirty - should not be called before d_instantiate, because it expects inode to be I_NEW (?)
    - inode_dec_link_count(inode) - drop_nlink + mark_inode_dirty  

#### About hard link and soft link [Read here](https://www.redhat.com/en/blog/linking-linux-explained)  
> A hard link always points a filename to data on a storage device.    
A soft link always points a filename to another filename, which then points to information on a storage device.  
- i_count - reference count of the inode in the kernel

## root inode

## struct inode_operations

| Operation | Directory | File |
| ----------|:---------:|:----:| 
| lookup    | +         | -    |
| mkdir     | +         | -    |
| rmdir     | +         | -    |

### lookup(struct inode *parent, struct dentry *dentry, unsigned int flags)
Checks if the name in dentry exists under the parent directory.

### mkdir(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode)
Creates a new directory with the given name in the parent directory.

### rmdir(struct inode *parent, struct dentry *dentry)
Removes an **empty** directory with the given name from the parent directory.

## struct file_operations

| Operation | Directory | File |
|-----------|:---------:|:----:|
|open       |+          |+     |


