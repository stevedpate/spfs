// SPDX-License-Identifier: GPL-2.0

/*
 * spfs.h - On-disk as well as in-core SPFS structures.
 *
 * Copyright (c) 2023 Steve D. Pate
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

#define SBTOSPFSSB(sb)	(struct spfs_sb_info *)sb->s_fs_info
#define ITOSPI(inode)	(struct sp_inode_info *)&inode->i_private

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

static inline struct
sp_inode_info *SPFS_I(struct inode *inode)
{
    return container_of(inode, struct sp_inode_info, vfs_inode);
}

#define	SPFS_SB		0x0001
#define	SPFS_INODE	0x0002

/*
 * XXX split by file
 */

extern struct address_space_operations sp_aops;
extern struct inode_operations sp_file_inops;
extern struct inode_operations sp_dir_inops;
extern struct file_operations sp_dir_operations;
extern struct file_operations sp_file_operations;
static const struct inode_operations sp_symlink_operations;

extern ino_t sp_ialloc(struct super_block *);
extern int sp_find_entry(struct inode *, char *);
extern int sp_block_alloc(struct super_block *);
extern int sp_block_alloc(struct super_block *);
extern int sp_unlink(struct inode *, struct dentry *);
extern int sp_link(struct dentry *, struct inode *, struct dentry *);
extern struct inode *sp_read_inode(struct super_block *sb, unsigned long ino);

extern long sp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
