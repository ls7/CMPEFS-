#include <linux/dcache.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>

  #include <linux/stat.h>
  #include <linux/fcntl.h>
 #include <linux/file.h>
 #include <linux/uio.h>
  #include <linux/fsnotify.h>
 #include <linux/security.h>
  #include <linux/export.h>
 #include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
// #include "read_write.h"

 #include <asm/uaccess.h>
 #include <asm/unistd.h>

#include <linux/backing-dev.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/pagevec.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

//for sockets
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include<linux/mount.h>

#define DEBUG(msg) printk(KERN_ALERT "CMPEFS: %s\n", msg)

#define DEFAULT_MODE      0775

#define NETLINK_USER 31

static int pid;

struct sock *nl_sk = NULL;
int received = 0; //set to 1 when message is received
char *data;
MODULE_LICENSE("GPL");



/* This method get all the incoming messages from the socket*/
static void hello_nl_recv_msg(struct sk_buff *skb) {
	DEBUG("hello_nl_recv_msg");
	struct nlmsghdr *nlh;
	//int pid;
	int msg_size;
	char *msg="Hello from kernel";

	printk(KERN_INFO "(hello_nl_recv_msg)Entering: %s\n", __FUNCTION__);

	msg_size=strlen(msg);

	nlh=(struct nlmsghdr*)skb->data;
	printk(KERN_INFO "(hello_nl_recv_msg)Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
	pid = nlh->nlmsg_pid; //pid of sending process 
	data = (char*)nlmsg_data(nlh);
	if(*data =='R'){
		received = 1;
		data = data+2;
	}
	
}



/*
 * This part is operations about inode
 */
static struct inode
*cmpefs_get_inode(struct super_block *, const struct inode *, umode_t, dev_t);

static int cmpefs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = cmpefs_get_inode(dir->i_sb, dir, mode, dev);
	DEBUG("In mknod");
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);
		error = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}

	return error;
}

static int cmpefs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{	DEBUG("In create");
	return cmpefs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int cmpefs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{	DEBUG("Inside mkdir");
	int retval = cmpefs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

static struct inode_operations cmpefs_dir_inode_operations = {
	.create = cmpefs_create,
	.lookup = simple_lookup,
	.mkdir  = cmpefs_mkdir,
	.mknod  = cmpefs_mknod,
};

/*Reading a file*/
static void my_netlink_read(struct file *filp,  char __user *buf, size_t len, loff_t *ppos)
{
	//do_sync_read(filp, buf,  len, ppos);
	DEBUG("in read");
	
	struct nlmsghdr *nlh;

	struct sk_buff *skb_out;
	int info_size;
	//char *msg="R test.txt ";
	char *msg=filp->f_path.dentry->d_name.name; 
	int res;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
	
	info_size = strlen(msg);
	skb_out = nlmsg_new(info_size,0);

	if(!skb_out)
	{

    		printk(KERN_ERR "Failed to allocate new skb\n");
    		return;
	} 
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,info_size,0); 
 
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh),"R ",2);
	strncpy(nlmsg_data(nlh)+2,msg,info_size);
	info_size = info_size + 2;
	strncpy(nlmsg_data(nlh)+info_size,"\0",1);
	res=nlmsg_unicast(nl_sk,skb_out,pid);
	DEBUG("Read done....");

	if(res<0){
    		printk(KERN_INFO "Error while sending bak to user\n");
	}

	
	if(received == 1){
		printk(KERN_INFO "Retriving ... data: %s\n",data);
		strncpy(buf,data, strlen(data));
		received = 0;
	} 
		

}

static void my_netlink_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{

	//do_sync_write(filp, buf,  len, ppos);

	DEBUG("in write");
	DEBUG(filp->f_path.dentry->d_name.name);
	//update the size

	printk(KERN_INFO "Size of file: %ld\n", filp->f_path.dentry->d_inode->i_size);
	loff_t size = filp->f_path.dentry->d_inode->i_size;

	if(size==0)
   		filp->f_path.dentry->d_inode->i_size = len;
	else
		  filp->f_path.dentry->d_inode->i_size = size + len;

	struct nlmsghdr *nlh;
	//int pid;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg=filp->f_path.dentry->d_name.name; //"W test.txt ";
	int info_size;

	int total_size;
	char *intVal;
	int res;



	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
	printk(KERN_INFO "Size of buffer: %u\n", len);
	//printk(KERN_INFO "File path is %s\n",name);
	//msg_size=strlen(msg);

	info_size = strlen(msg);
	msg_size=strlen(buf);
	total_size = info_size + msg_size;

	intVal = (char *)&total_size;

	skb_out = nlmsg_new(info_size+msg_size,0);

	if(!skb_out)
	{

    		printk(KERN_ERR "Failed to allocate new skb\n");
	       return;

	} 
	
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size+info_size,0); 
 	nlh->nlmsg_flags |= NLM_F_ACK;

	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	//strncpy(nlmsg_data(nlh),msg,msg_size);

	//memset(nlmsg_data(nlh),0,sizeof(buf));
	//total_size = 0;

	strncpy(nlmsg_data(nlh),"W ",2);
	//total_size =2;
	strncpy(nlmsg_data(nlh)+2,msg,info_size);
	strncpy(nlmsg_data(nlh)+info_size+2," ",1);
	//sprintf(nlmsg_data(nlh)+info_size, "%u", len);

	strncpy(nlmsg_data(nlh)+info_size+3,buf,len);
	info_size = 3+info_size+len;
	strncpy(nlmsg_data(nlh)+info_size,"\0",1);
	//strncpy(nlmsg_data(nlh)+info_size,"\0",1);

	DEBUG("sending file name and data ....");
	 printk(KERN_INFO "sent data : %s\n",nlmsg_data(nlh));

	res=nlmsg_unicast(nl_sk,skb_out,pid);
	DEBUG("Write done....");

	if(res<0)
	    printk(KERN_INFO "Error while sending bak to user\n");

	DEBUG("Retriving ");


}

const struct file_operations cmpefs_file_operations = {

//	.read		= do_sync_read,
	.read           = my_netlink_read,
	.aio_read	= generic_file_aio_read,
	.write		= my_netlink_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.llseek		= generic_file_llseek,
};


#ifdef ankit


/*****************************************************************************/
/*
 * add a contiguous set of pages into a ramfs inode when it's truncated from
 * size 0 on the assumption that it's going to be used for an mmap of shared
 * memory
 */
int cmpefs_nommu_expand_for_mapping(struct inode *inode, size_t newsize)
{
	unsigned long npages, xpages, loop;
	struct page *pages;
	unsigned order;
	void *data;
	int ret;

	/* make various checks */
	order = get_order(newsize);
	if (unlikely(order >= MAX_ORDER))
		return -EFBIG;

	ret = inode_newsize_ok(inode, newsize);
	if (ret)
		return ret;

	i_size_write(inode, newsize);

	/* allocate enough contiguous pages to be able to satisfy the
	 * request */
	pages = alloc_pages(mapping_gfp_mask(inode->i_mapping), order);
	if (!pages)
		return -ENOMEM;

	/* split the high-order page into an array of single pages */
	xpages = 1UL << order;
	npages = (newsize + PAGE_SIZE - 1) >> PAGE_SHIFT;

	split_page(pages, order);

	/* trim off any pages we don't actually require */
	for (loop = npages; loop < xpages; loop++)
		__free_page(pages + loop);

	/* clear the memory we allocated */
	newsize = PAGE_SIZE * npages;
	data = page_address(pages);
	memset(data, 0, newsize);

	/* attach all the pages to the inode's address space */
	for (loop = 0; loop < npages; loop++) {
		struct page *page = pages + loop;

		ret = add_to_page_cache_lru(page, inode->i_mapping, loop,
					GFP_KERNEL);
		if (ret < 0)
			goto add_error;

		/* prevent the page from being discarded on memory pressure */
		SetPageDirty(page);
		SetPageUptodate(page);

		unlock_page(page);
		put_page(page);
	}

	return 0;

add_error:
	while (loop < npages)
		__free_page(pages + loop++);
	return ret;
}

/*****************************************************************************/
/*
 *
 */
static int cmpefs_nommu_resize(struct inode *inode, loff_t newsize, loff_t size)
{
	int ret;

	/* assume a truncate from zero size is going to be for the purposes of
	 * shared mmap */
	if (size == 0) {
		if (unlikely(newsize >> 32))
			return -EFBIG;

		return cmpefs_nommu_expand_for_mapping(inode, newsize);
	}

	/* check that a decrease in size doesn't cut off any shared mappings */
	if (newsize < size) {
		ret = nommu_shrink_inode_mappings(inode, size, newsize);
		if (ret < 0)
			return ret;
	}

	truncate_setsize(inode, newsize);
	return 0;
}

/*****************************************************************************/
/*
 * handle a change of attributes
 * - we're specifically interested in a change of size
 */
static int cmpefs_nommu_setattr(struct dentry *dentry, struct iattr *ia)
{
	struct inode *inode = dentry->d_inode;
	unsigned int old_ia_valid = ia->ia_valid;
	int ret = 0;

	/* POSIX UID/GID verification for setting inode attributes */
	ret = inode_change_ok(inode, ia);
	if (ret)
		return ret;

	/* pick out size-changing events */
	if (ia->ia_valid & ATTR_SIZE) {
		loff_t size = inode->i_size;

		if (ia->ia_size != size) {
			ret = cmpefs_nommu_resize(inode, ia->ia_size, size);
			if (ret < 0 || ia->ia_valid == ATTR_SIZE)
				goto out;
		} else {
			/* we skipped the truncate but must still update
			 * timestamps
			 */
			ia->ia_valid |= ATTR_MTIME|ATTR_CTIME;
		}
	}

	setattr_copy(inode, ia);
 out:
	ia->ia_valid = old_ia_valid;
	return ret;
}

#endif

const struct inode_operations cmpefs_file_inode_operations = {
	.setattr		= simple_setattr,
	.getattr		= simple_getattr, 
};

static struct backing_dev_info cmpefs_backing_dev_info = {
	.name		= "cmpefs",
	.ra_pages	= 0,	// No readahead 
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK |
			  BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
			  BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
};

const struct address_space_operations cmpefs_aops = {
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
//	.set_page_dirty = __set_page_dirty_no_writeback,
};

static struct inode *cmpefs_get_inode(struct super_block *sb,
		const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		DEBUG("Inside cmpefs get inode");
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		DEBUG("Inside cmpefs get inode : before i_mapping");
		inode->i_mapping->a_ops = &cmpefs_aops;
		inode->i_mapping->backing_dev_info = &cmpefs_backing_dev_info;
		DEBUG("Inside cmpefs get inode : after i_mapping");
	switch (mode & S_IFMT) {
		
		case S_IFREG:
			DEBUG("Inside IFREG");
			inode->i_op = &cmpefs_file_inode_operations;
			inode->i_fop = &cmpefs_file_operations;
			break;
		case S_IFDIR:
			DEBUG("Inside IFDIR");
			inode->i_op = &cmpefs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;
			break;
		}
	/*	inode->i_op = &cmpefs_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;  */
	}

	return inode;
}



/*
 * This part is operations about super block
 */

static const struct super_operations cmpefs_ops ={
};

/*
 * This is an intersting function, it makes an inode as a super block's
 * root. This is who combines inode and super block
 */
static int cmpefs_file_super(struct super_block *sb, void *data, int slient)
{
	struct inode *inode;

	sb->s_op = &cmpefs_ops;
	inode = cmpefs_get_inode(sb, NULL, S_IFDIR | DEFAULT_MODE, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		DEBUG("cannot get dentry root");
	DEBUG("work done, I have mounted");
	return 0;
}

static struct dentry *cmpefs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	DEBUG("mount cmpefs");
	return mount_nodev(fs_type, flags, data, cmpefs_file_super);
}

static void cmpefs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
	DEBUG("umount, cmpefs killed");
}

static struct file_system_type cmpefs_type = {
	.name         = "cmpefs",
	.mount        = cmpefs_mount,
	.owner        = THIS_MODULE,
	.kill_sb      = cmpefs_kill_sb,
};

/*
 * This part is about linux modules
 */
static int __init hello_init(void) {

printk("Entering: %s\n",__FUNCTION__);
nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg,NULL, THIS_MODULE);
if(!nl_sk)
{

    printk(KERN_ALERT "Error creating socket.\n");
    return -10;

}

return 0;
}

static int __init init_cmpefs(void)
{
	int err;

	DEBUG("In init");

	err = register_filesystem(&cmpefs_type);
	if (err)
		DEBUG("registration failed");

	hello_init();

	return 0;
}

static void __exit hello_exit(void) {

printk(KERN_INFO "exiting hello module\n");
netlink_kernel_release(nl_sk);
}

static void __exit exit_cmpefs(void)
{
	DEBUG("bye, world");
	hello_exit();
	unregister_filesystem(&cmpefs_type);
}

module_init(init_cmpefs);
module_exit(exit_cmpefs);
