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
    - [drop_inode](#drop_inode)
    - [alloc_inode](#alloc_inode)
    - [destroy_inode](#destroy_inode)
    - [free_inode](#free_inode)
- s_root - [root inode](#root-inode)
 
#### statfs
statfs(struct dentry *dentry, struct kstatfs *buf)
Fill in struct kstatfs.  

### fill_super(struct super_block *sb,  void *data,  int silent)
Fill in struct super_block (described above) and initialize [root inode](#root-inode). 

## Inode
### struct super_operations [Read here](https://www.kernel.org/doc/html/next/filesystems/vfs.html)
<a id="drop_inode"></a>
- drop_inode - should be NULL or generic_delete_inode (if you don't use [inode_hashtable](#inode_hashtable)). Called in [iput()](#iput): if drop_inode is NULL -> **generic_drop_inode** -> checks i_nlink; if drop_inode is **generic_delete_inode** -> delete inode regardless of the value of i_nlink.
<a id="alloc_inode"></a>
- alloc_inode - implement if your own inode structure embeds struct inode. It is called during new_inode().
<a id="destroy_inode"></a>
- destroy_inode - implement if you allocated resources for struct inode (e.g. for inode->i_private) and undo everything from alloc_inode.
<a id="free_inode"></a>
- free_inode - implement if you use call_rcu() in destroy_inode() to free struct inode, then it's better to release memory here.

### inode_cache

### struct inode
- i_ino - inode number. For on-disk FS ino is predefined (you get it from your structures). For in-mem FS can be generated dynamically using get_next_ino(). ino cannot be 0. (ino 0 - no inode)
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
    - inode_inc_link_count(inode) - inc_nlink + [mark_inode_dirty](#mark_inode_dirty) - should not be called before d_instantiate, because it expects inode to be I_NEW (?)
    - inode_dec_link_count(inode) - drop_nlink + [mark_inode_dirty](#mark_inode_dirty)     
#### mark_inode_dirty
Marks the inode as dirty, telling the VFS that the in-memory inode data has changed compared to the on-disk copy. It schedules the inode to be written back to disk later.  
[From source code](https://elixir.bootlin.com/linux/v6.17-rc1/source/fs/fs-writeback.c#L2468)
> CAREFUL!  We only add the inode to the dirty list if it is hashed or if it refers to a blockdev. Unhashed inodes will never be added to the dirty list even if they are later hashed, as they will have been marked dirty already.

#### About hard link and soft link [Read here](https://www.redhat.com/en/blog/linking-linux-explained)  
> A hard link always points a filename to data on a storage device.    
A soft link always points a filename to another filename, which then points to information on a storage device.  
- i_count - reference count of the inode in the kernel.
    - ihold() - increment i_count
    <a id="iput"></a>
    - iput() - decrement i_count. If i_count == 0 -> drop_inode: if i_nlink == 0, evict inode -> destroy_inode(). If not - retain inode in cache if FS is alive, sync and evict if FS is shutting down. (So basically, if no one is using struct inode we should just let it stay in [inode_hashtable](#inode_hashtable), unless i_nlink == 0, it means on-disk inode is prepared to be destroyed, so we can destroy it too)
    - igrab() - checks whether inode is dying: if so - returns NULL, if not increment i_count.

### inode_hashtable
inode_hashtable is a global inode hash table, where the key is (sb, ino). Used for quick search of an inode based on inode number.
- insert_inode_hash(inode) - add an inode to inode_hashtable.
- ilookup(sb, ino) - get an inode from inode_hashtable: if found, returns the inode with increased i_count; if not, returns NULL.
- iget_locked(sb, ino) - similar to ilookup(), but if not in inode_hashtable -> alloc_inode() and returns it locked, hashed, and with the I_NEW flag set.
- unlock_new_inode(inode) - called once the inode is fully initialized, to clear the new state of the inode and wake up anyone waiting for the inode to finish initialisation.

Most FSes use ino as a unique identifier of on-disk inode structure and do not keep a direct pointer to struct inode. So, to be able to access struct inode based on ino, you should call insert_inode_hash(), when you creating a new inode. And to get actual struct inode from ino (iget), call iget_locked() -> check i_state: if I_NEW -> synchronize new inode with on-disk inode and call unlock_new_inode(). If iget_locked() returns I_NEW inode, it means only(!) that inode wasn't present in inode_hashtable, maybe because it was unused (i_count dropped to 0 and VFS destroyed it) or because it has never been hashed before.

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


