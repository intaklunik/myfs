# Filesystem
- **struct inode** represents a file and stores file metadata (permissions, type, timestamps, etc.).
- **struct dentry** represents a filename (directory entry), exists separatly from inode because 1 inode may have multiple dentries (hardlinks).
- **struct file** represents an open file, stores the open mode, file position and a reference to the inode.

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
- s_root = d_make_root([root inode](#root-inode))
- s_time_gran
- s_blocksize
- s_blocksize_bits
- s_nzones
- s_dirsize
- s_namelen
- s_max_links  
 
#### statfs
statfs(struct dentry *dentry, struct kstatfs *buf)
Fill in struct kstatfs.  

### fill_super(struct super_block *sb,  void *data,  int silent)
1. Fill in struct super_block (described above).
2. Create and initialize [root inode](#root-inode).
3. Call d_make_root(root_inode) and set result to sb->s_root. Check if != NULL.

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
In start_kernel(): vfs_caches_init() -> inode_init() -> creates inode_hashtable (for ino -> struct inode) and inode_cachep - slab cache where actual struct inode objects are allocated.
If your inode structure embeds struct inode, you should create your own your_inode_cachep:
- kmem_cache_create - call before register_filesystem, args ([Read here](https://elixir.bootlin.com/linux/v6.17-rc1/source/include/linux/slab.h#L281)):
    - name - cache name
    - object_size - size of an object in the cache
    - align 
    - flags
    - **ctor** - a constructor for the objects. In your constructor function you **must** call inode_init_once(&vfs_inode) to properly initialize the embedded struct inode.
- kmem_cache_destroy - call after unregister_filesystem.
- [alloc_inode_sb](#https://elixir.bootlin.com/linux/v6.17-rc1/source/include/linux/fs.h#L3405) - call in your alloc_inode().
- kmem_cache_free - call in your free/destroy_inode().

### struct inode
- i_ino - inode number. For on-disk FS ino is predefined (you get it from your structures). For in-mem FS can be generated dynamically using get_next_ino(). ino cannot be 0. (ino 0 - no inode)
- i_sb - pointer to the super_block, parent->i_sb.
- i_mode - file type(S_IFREG, S_IFDIR...) + file permissions.  

|        | File                 | Directory                                   |
|--------|----------------------|-------------------------------------------- |
| read   | read a file          | list files in the dir (ls)                  |
| write  | write to file        | create, delete, rename files in the dir     |
| execute| run file as a program| enter the dir and use it in a file path (cd)|

- i_rdev - device number written by mknod. Exists only for character and block device inodes, stored on disk.
- i_cdev - pointer to struct cdev that owns this device number. Filled in at runtime, acts as a cache, not stored on disk. (Instead of looking up the cdev in cdev_map using i_rdev on every open(), the kernel performs lookup once and then stores the result in i_cdev; next open() call goes directly to i_cdev pointer [Source code](https://elixir.bootlin.com/linux/v6.16/source/fs/char_dev.c#L386))
- i_op - inode_operations. You define 2 sets of i_op: for directories and files.
- i_fop - file_operations. You define 2 sets of i_fop: for directories and files.
- i_nlink - number of hard links. Initialized with 1 (the link is between the filename and the actual data): alloc_inode() -> inode_init_always() -> inode_init_always_gfp().  
For directories should be **at least 2**: first - '.', second - from the parent directory, other - from each child's directory('..').   
For root - '.' and '..' both point to itself. 
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

Most FSes use ino as a unique identifier of on-disk inode structure and do not keep a direct pointer to struct inode. So, to be able to access struct inode based on ino, you should call insert_inode_hash(), when you creating a new inode. And to get actual struct inode from ino **(iget)**, call iget_locked() -> check i_state: if I_NEW -> synchronize new inode with on-disk inode and call unlock_new_inode(). If iget_locked() returns I_NEW inode, it means only(!) that inode wasn't present in inode_hashtable, maybe because it was unused (i_count dropped to 0 and VFS destroyed it) or because it has never been hashed before.

#### iget vs new_inode
Both functions can be used to create a new struct inode.
- new_inode(sb) - call it when creating a brand new inode that has never existed on disk: mkdir, mknod, create.
- your custom iget(sb, ino) function  - call it when the inode may already exist on disk: during root_inode creation, lookup.

## root inode
Create and initialize in fill_super().
In **minix** FS root_inode creation and initialization is done in [iget()](#iget-vs-new_inode).
- i_ino - predefined in your FS or get_next_ino(). Some FSes set root ino to 2 (0 - no inode, 1 - bad block).
- i_nlink, i_op, i_fop - set as for directories.
- i_sb - set to sb argument you receive in your fill_super().   

The dentry for the root inode is created automatically during d_make_root() -> d_alloc_anon() ->  __d_alloc() -> [slash_name](https://elixir.bootlin.com/linux/v6.17-rc1/source/fs/dcache.c#L1702), which is "/".

## struct inode_operations

| Operation        | Directory | File |
| -----------------|:---------:|:----:| 
| [lookup](#lookup)| +         | -    |
| [mkdir](#mkdir)  | +         | -    |
| [rmdir](#rmdir)  | +         | -    |
| create           |           |      |
| link             |           |      |
| unlink           |           |      |
| rename           |           |      |
| [mknod](#mknod)  | +         | -    |
| setattr          |           |      |
| getattr          |           |      |
| listxattr        |           |      |

### lookup
lookup(struct inode *parent, struct dentry *dentry, unsigned int flags) - checks if the name in dentry exists under the parent directory.   

1. Check whether the given name in dentry is valid. If not, return ENAMETOOLONG.
2. Call your lookup_name(dentry) that returns ino:
    - ino == 0 -> entry doesn't exist -> d_add(dentry, NULL) - add dentry to the negative cache.  
    - ino != 0 -> entry exists -> iget(ino) -> d_add(dentry, inode) - add dentry to the positive cache.

### mkdir
mkdir(struct mnt_idmap *idmap, struct inode *parent, struct dentry *dentry, umode_t mode) - creates a new directory with the given name in the parent directory.   

1. Create and initialize new inode.
2. inc_nlink() for the parent directory and for the new directory.
3. d_instantiate(dentry, inode) - attach the new inode to the dentry.

### rmdir
rmdir(struct inode *parent, struct dentry *dentry) - removes an **empty** directory with the given name from the parent directory.   

1. Get inode (directory to remove) from the dentry: d_inode(dentry).
2. Check if the directory is empty (your implementation). If not, return ENOTEMPTY.
3. drop_nlink() for the parent directory and the current directory.

### mknod
mknod(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode, dev_t rdev) - creates a special file (char, block, FIFO, socket) with the given name in the parent directory.

1. Create and initialize new inode.
2. Call init_special_inode(): for S_IFCHR and S_IFBLK it will set inode->i_rdev = rdev, and inode->i_fop = &def_chr_fops and &def_blk_fops. Device files use device-specific fops. During def_*_fops->open(), inode fops are replaced with device fops.
3. d_instantiate(dentry, inode) - attach the new inode to the dentry.

## struct file_operations

| Operation                        | Directory | File |
|----------------------------------|:---------:|:----:|
|open                              |+          |+     |
|release                           |           |      |
|iterate_shared                    |+          |-     |
|[read](#read)                     |-          |+     |
|[write](#write)                   |-          |+     |
|llseek                            |           |      |
|mmap                              |           |      |
|poll                              |           |      |
|[unlocked_ioctl](#unlocked_ioctl) |+          |+     |
|flush                             |           |      |
|fsync                             |           |      |

### read
read(struct file *filp, char __user *buff, size_t count, loff_t *offp) - reads data from file *filp* into the user buffer *buff*.    
Read may return fewer bytes than requested ( < *count*).  
1. FS-specific code
2. copy_to_user()
3. Advance the file offset by the number of bytes actually read.
4. Return number of bytes read, 0 (EOF) or negative error code (error).

### write
write(struct file *filp, const char __user *buff, size_t count, loff_t *offp) - writes data from user buffer *buff* into file *filp*.    
Write may return fewer bytes than requested.
1. FS-specific code
2. copy_from_user()
3. Advance the file offset by the number of bytes actually written.
4. Return number of bytes written or negative error code (error).

### unlocked_ioctl
unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) - handles ioctl command *cmd* with an argument *arg* for a file *filp*.  
For custom ioctl commands:
1. Choose an ioctl magic number [Read here](https://www.kernel.org/doc/html/latest/userspace-api/ioctl/ioctl-number.html)
2. Define ioctl commands using macros _IO*(type, nr, argtype), where type is the magic number defined in step 1, nr - the command number, argtype - the argument type (may be a pointer).
3. Use copy_to_user/copy_from_user or put_user/get_user to exchange data with userspace.
4. Return -ENOTTY for unsupported commands.

# Useful links
[https://elixir.bootlin.com/linux/v6.9/source/Documentation/filesystems/vfs.rst](https://elixir.bootlin.com/linux/v6.9/source/Documentation/filesystems/vfs.rst)
