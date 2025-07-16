
#ifdef MAC_BIT64
#define VM_ADDR()			(pc += 8, (Address)(((uintptr_t)pc[-8] << 56) | \
							((uintptr_t)pc[-7] << 48) | ((uintptr_t)pc[-6] << 40) | \
							((uintptr_t)pc[-5] << 32) | ((uintptr_t)pc[-4] << 24) | \
							((uintptr_t)pc[-3] << 16) | ((uintptr_t)pc[-2] << 8) | \
							(uintptr_t)pc[-1]))
#else
#define VM_ADDR()			(pc += 4, (Address)((pc[-4] << 24) | \
							(pc[-3] << 16) | (pc[-2] << 8) | pc[-1])
#endif

