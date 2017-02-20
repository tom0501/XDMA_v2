/*
 * This file contains WZab's extensions to the xdma driver provided by Xilinx
 * All my extensions are published under GPL v2
 * Wojciech M. Zabolotny <wzab@ise.pw.edu.pl>
 * 
 * This file will be included in the original xdma-core.c to make maintenace easier
 */
 
 
/* Mapping of the allocated buffers */  
void swz_mmap_open(struct vm_area_struct *vma)
{
}
 
void swz_mmap_close(struct vm_area_struct *vma)
{
}
 
static int swz_mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
    long offset;
    struct xdma_char *xchar = NULL;
    char * buffer = NULL;
    int buf_num = 0;
    xchar = (struct xdma_char *) vma->vm_private_data;
    //Calculate the offset (according to info in 
    // https://lxr.missinglinkelectronics.com/linux+v2.6.32/drivers/gpu/drm/i915/i915_gem.c#L1195
    // it is better not ot use the vmf->pgoff )
    offset = (unsigned long)(vmf->virtual_address - vma->vm_start);
    //Calculate the buffer number
    buf_num = offset/WZ_DMA_BUFLEN;
    //Check if the resulting number is not higher than the number of allocated buffers
    if(buf_num > WZ_DMA_NOFBUFS) {
        printk(KERN_ERR "Access outside the buffer\n");
        return -EFAULT;
    }
    //Calculate the offset inside the buffer
    offset = offset - buf_num * WZ_DMA_BUFLEN;
    buffer = xchar->engine->wz_ext.buf_addr[buf_num];
    //Get the pfn of the buffer
    vm_insert_pfn(vma,(unsigned long)(vmf->virtual_address),virt_to_phys(&buffer[offset]) >> PAGE_SHIFT);         
    return VM_FAULT_NOPAGE;
}
 
struct vm_operations_struct swz_mmap_vm_ops =
{
    .open =     swz_mmap_open,
    .close =    swz_mmap_close,
    .fault =    swz_mmap_fault,    
};


static int char_sgdma_wz_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct xdma_char * xchar;
    xchar = (struct xdma_char *) file->private_data;
    //Check if the buffers are allocated
    if(xchar->engine->wz_ext.buf_ready) {
        printk(KERN_ERR "Can't mmap when buffers are not allocated\n");
        return -EINVAL;    
    }
    vma->vm_ops = &swz_mmap_vm_ops;
    vma->vm_flags |= VM_RESERVED;    
    //file->private data contains the pointer to the xdma_char
    vma->vm_private_data = file->private_data;
    swz_mmap_open(vma);
    return 0;
}


/* Functions supporting the ioctls */
static int ioctl_do_wz_alloc_buffers(struct xdma_engine *engine, unsigned long arg) 
{
    int i;
    struct wz_xdma_engine_ext * ext;
    ext = &engine->wz_ext;
    //We allocate the buffers using dmam_alloc_noncoherent, so the user space
    //application may use cache.
    for(i=0;i<WZ_DMA_NOFBUFS;i++) {
        ext->buf_addr[i] = dma_alloc_noncoherent(&engine->lro->pci_dev->dev,
                WZ_DMA_BUFLEN, &ext->buf_dma_t[i],GFP_USER);
        if(ext->buf_addr[i] == NULL) {
            int j;
            //Free already allocated buffers
            for(j=0;j<i;j++) {
                dma_free_noncoherent(&engine->lro->pci_dev->dev,
                WZ_DMA_BUFLEN, ext->buf_addr[i], ext->buf_dma_t[i]);
            }
            ext->buf_ready = 0;
            return -ENOMEM;
        }
    }
    ext->buf_ready = 1;
    return 0;
}

static int ioctl_do_wz_free_buffers(struct xdma_engine *engine, unsigned long arg) 
{
    int i;
    struct wz_xdma_engine_ext * ext;
    ext = &engine->wz_ext;
    if(ext->buf_ready) {
        dma_free_noncoherent(&engine->lro->pci_dev->dev,
        WZ_DMA_BUFLEN, ext->buf_addr[i], ext->buf_dma_t[i]);        
    }
    ext->buf_ready = 0;
}

/* 
 * At the moment it is unclear, how we can handle interrupts. It seems, that the 
 * interrupt handling scheme in xdma is quite complex and relying on it may impact
 * the performance... 
 * However we need to get the quick verification of the concept first.
 */
  

static int ioctl_do_wz_start(struct xdma_engine *engine, unsigned long arg)
{
    //First build the XDMA transfer descriptors
    //Close them into a loop
    //@@@ Maybe the above should be moved to alloc_buffers???
    //Submmit the whole transfer
    //Start the transfer
};

static int ioctl_do_wz_stop(struct xdma_engine *engine, unsigned long arg)
{
    //Stop the transfer
    //Clear the transfer descriptors
};

static int ioctl_do_wz_getbuf(struct xdma_engine *engine, unsigned long arg)
{
};