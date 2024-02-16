// SPDX-License-Identifier: GPL-2.0

/*
 * spfs.h - On-disk as well as in-core SPFS structures.
 *
 * Copyright (c) 2023-2024 Steve D. Pate
 */

#define SP_BSIZE                2048
#define SP_MAXFILES             128
#define SP_MAXBLOCKS            760
#define SP_NAMELEN              28        
#define SP_DIRENT_SIZE 			32        
#define SP_DIRS_PER_BLOCK       64
#define SP_DIRECT_BLOCKS        247
#define SP_FIRST_DATA_BLOCK     129
#define SP_MAGIC                0x53504653
#define SP_INODE_BLOCK          1
#define SP_ROOT_INO             2

/*
 * The on-disk superblock. The number of inodes and 
 * data blocks is fixed.
 */

struct sp_superblock {
	__u32	s_magic;
	__u32	s_mod;
	__u32	s_nifree;
	__u32	s_inode[SP_MAXFILES];
	__u32	s_nbfree;
	__u16	s_block[SP_MAXBLOCKS];
};

/*
 * The on-disk inode.
 */

struct sp_inode {
	__u32	i_mode;
	__u32	i_nlink;
	__u32	i_atime;
	__u32	i_mtime;
	__u32	i_ctime;
	__u32	i_uid;
	__u32	i_gid;
	__u32	i_size;
	__u32	i_blocks;
	__u32	i_addr[SP_DIRECT_BLOCKS];
};

/*
 * Allocation flags
 */

#define SP_INODE_FREE     0
#define SP_INODE_INUSE    1
#define SP_BLOCK_FREE     0
#define SP_BLOCK_INUSE    1

/*
 * Filesystem flags
 */

#define SP_FSCLEAN        0
#define SP_FSDIRTY        1

/*
 * Fixed size directory entry.
 */

struct sp_dirent {
        __u32       d_ino;
        char        d_name[SP_NAMELEN];
};

#ifdef __KERNEL__

/*
 * In-core SPFS superblock
 */

struct spfs_sb_info {
	unsigned long  	s_nifree;
	unsigned long  	s_inode[SP_MAXFILES];
	unsigned long  	s_nbfree;
	unsigned long  	s_block[SP_MAXBLOCKS];
	struct mutex 	s_lock;
};

/*
 * In-core SPFS inode
 */

struct sp_inode_info {
    char            i_fs[4];
	int				i_blocks;
	int				i_addr[SP_DIRECT_BLOCKS];
	char			i_symlink[SP_NAMELEN];
    struct inode	vfs_inode;  
};

#define	SPFS_SB		0x0001
#define	SPFS_INODE	0x0002

#define SBTOSPFSSB(sb)	(struct spfs_sb_info *)sb->s_fs_info
#define ITOSPI(inode)   (struct sp_inode_info *)inode->i_private

static inline struct sp_inode_info *spi_container(struct inode *inode)
{
    return container_of(inode, struct sp_inode_info, vfs_inode);
}

/*
 * Functions and structures defined throughout the source code.
 */

extern struct address_space_operations sp_aops;
extern struct inode_operations sp_file_inops;
extern struct inode_operations sp_dir_inops;
extern struct file_operations sp_dir_operations;
extern struct file_operations sp_file_operations;
static const struct inode_operations sp_symlink_operations;

/*
 * Functions from sp_alloc.c
 */

extern ino_t sp_ialloc(struct super_block *);
extern int sp_block_alloc(struct super_block *sb);

/*
 * Functions from sp_dir.c
 */

extern int sp_dirdel(struct inode *dip, char *name);
extern int sp_diradd(struct inode *dip, const char *name, int inum);
extern int sp_rename(struct user_namespace *mnt_userns, struct inode *old_dir,
                     struct dentry *old_dentry, struct inode *new_dir,
                     struct dentry *new_dentry, unsigned int flags);
extern int sp_readdir(struct file *f, struct dir_context *ctx);
extern struct inode *sp_new_inode(struct inode *dip, struct dentry *dentry, 
                                   umode_t mode, const char *symlink_target);
extern int sp_create(struct user_namespace *mnt_userns, struct inode *dip,
                     struct dentry *dentry, umode_t mode, bool excl);
extern int sp_mkdir(struct user_namespace *mnt_userns, struct inode *dip,
                    struct dentry *dentry, umode_t mode);
extern int sp_rmdir(struct inode *dip, struct dentry *dentry);
extern struct dentry *sp_lookup(struct inode *dip, struct dentry *dentry, 
                                unsigned int flags);
extern int sp_getattr(struct user_namespace *mnt_userns, 
                      const struct path *path, struct kstat *stat, 
                      u32 request_mask, unsigned int flags);
extern int sp_readlink(struct dentry *dentry, char __user *buffer, 
                              int buflen);
extern const char *sp_page_get_link(struct dentry *dentry, struct inode *inode,
                                    struct delayed_call *callback);
extern int sp_symlink(struct user_namespace *mnt_userns, struct inode *dip,
                      struct dentry *dentry, const char *target);
extern int sp_link(struct dentry *old, struct inode *dip, struct dentry *new);
extern int sp_unlink(struct inode *dip, struct dentry *dentry);

/*
 * Functions from sp_inode.c
 */

extern int sp_find_entry(struct inode *, char *);
extern int sp_unlink(struct inode *, struct dentry *);
extern int sp_link(struct dentry *, struct inode *, struct dentry *);
extern struct inode *sp_read_inode(struct super_block *sb, unsigned long ino);
extern int sp_write_inode(struct inode *inode, struct writeback_control *wbc);
extern void sp_free_inode(struct inode *inode);
extern void sp_evict_inode(struct inode *inode);
extern void sp_put_super(struct super_block *sb);
extern int sp_statfs(struct dentry *dentry, struct kstatfs *buf);
extern struct inode * sp_alloc_inode(struct super_block *sb);
extern int spfs_fill_super(struct super_block *sb, void *data, 
                                  int silent);
extern struct dentry *spfs_mount(struct file_system_type *fs_type, int flags,
                                 const char *dev_name, void *data);


/*
 * Functions from sp_file.c	
 */

extern int sp_get_block(struct inode *inode, sector_t block,
                        struct buffer_head *bh_result, int create);
extern void sp_write_failed(struct address_space *mapping, loff_t to);
extern int sp_write_begin(struct file *file, 
                                 struct address_space *mapping, loff_t pos, 
                                 unsigned len, struct page **pagep, 
                                 void **fsdata);
extern int sp_write_end(struct file *file, struct address_space *mapping,
                               loff_t pos, unsigned len, unsigned copied,
                               struct page *page, void *fsdata);
extern int sp_writepage(struct page *page, struct writeback_control *wbc);
extern int sp_read_folio(struct file *file, struct folio *folio);
extern sector_t sp_bmap(struct address_space *mapping, sector_t block);

/*
 * Functions from sp_ioctl.c
 */

extern long sp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
