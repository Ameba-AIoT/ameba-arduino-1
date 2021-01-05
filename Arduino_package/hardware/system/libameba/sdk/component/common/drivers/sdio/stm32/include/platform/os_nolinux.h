
typedef unsigned int size_t;
typedef u32 dma_addr_t;
typedef unsigned int gfp_t;
typedef unsigned short __le16;
typedef unsigned int __le32;

#define __iomem
#define IRQF_SHARED 0
#define PAGE_CACHE_SIZE 8192 //PLATFORM_TODO
#define __FUNCTION__ "func"
#define __func__ "func"
#define __devinit 

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#ifndef NULL
#define NULL	0
#endif

#ifndef true
#define true 1
#define false 0
#endif

struct device {
	const char		*init_name; /* initial name of the device */
};

struct platform_device {
	const char	* name;
	int			id;
	struct device	dev;
};

struct scatterlist {
	unsigned long	page_link; //linux page idx
	unsigned int	offset; //offset in page
	unsigned int	length; //total length
	const void* dma_address;
};

struct dma_attrs {
	//PLATFORM_TODO
	int			reserved;
};

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};

typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);

static inline const char *dev_name(const struct device *dev)
{
	/* Use the init name until the kobject becomes available */
	if (dev->init_name)
		return dev->init_name;
	else
		return NULL;
}

static inline unsigned int rtw_min(const unsigned int a, const unsigned int b)
{
	return ((a) > (b) ? (b) : (a));
}

static inline int rtw_memcpy(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1 = cs, *su2 = ct, *end = su1 + count;
	int res = 0;

	while (su1 < end) {
		res = *su1++ - *su2++;
		if (res)
			break;
	}
	return res;
}

static inline void *rtw_memset(void *s, int c, unsigned int  count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;

	return s;
}

static inline char *rtw_strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

static inline unsigned int rtw_strlen(const char *s)
{
	const char *sc = s;

	while (*sc != '\0')
		sc++;

	return sc - s;
}

/* DMA Ralated */
/**
 * sg_init_one - Initialize a single entry sg list
 * @sg:		 SG entry
 * @buf:	 Virtual address for IO
 * @buflen:	 IO length
 *
 **/
static inline void sg_init_one(struct scatterlist *sg, const void *buf, unsigned int buflen)
{
	rtw_memset(sg, 0, sizeof(*sg));
	//sg->page_link |= 0x02;
	//sg->page_link &= ~0x01;

	sg->length = buflen;
	sg->dma_address = buf;
}

#define sg_dma_address(sg)	((sg)->dma_address)

/* Define sg_next is an inline routine now in case we want to change
   scatterlist to a linked list later. */
static inline struct scatterlist *sg_next(struct scatterlist *sg)
{
	return sg + 1;
}

/*
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the x86 architecture, we just read/write the
 * memory location directly.
 */
static inline u8 readb(const volatile void __iomem *addr)
{
#if 1
	return *(const volatile u8 *) addr;
#else
	return sil_reb_mem((const u8*)addr);
#endif
}

static inline u16 readw(const volatile void __iomem *addr)
{
#if 1
	return *(const volatile u16 *) addr;
#else
	return sil_reh_mem((const u16*)addr);
#endif
}

static inline u32 readl(const volatile void __iomem *addr)
{
#if 1
	return *(const volatile u32 *) addr;
#else
	return sil_rew_mem((const u32*)addr);
#endif
}

static inline void writeb(u8 b, volatile void __iomem *addr)
{
#if 1
	*(volatile u8 *) addr = b;
#else
	return sil_wrb_mem((u8*)addr, b);
#endif
}

static inline void writew(u16 b, volatile void __iomem *addr)
{
#if 1
	*(volatile u16 *) addr = b;
#else
	return sil_wrh_mem((u16*)addr, b);
#endif
}

static inline void writel(u32 b, volatile void __iomem *addr)
{
#if 1
	*(volatile u32 *) addr = b;
#else
	return sil_wrw_mem((u32*)addr, b);
#endif
}

static inline u32 __raw_readl(unsigned long addr)
{
#if 1
	return *(const volatile u32 *) addr;
#else
	return sil_rew_mem((const u32*)addr);
#endif
}

static inline void __raw_writel(u32 b, unsigned long addr)
{
#if 1
	*(volatile u32 *) addr = b;
#else
	return sil_wrw_mem((u32*)addr, b);
#endif
}


