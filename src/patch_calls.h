static unsigned char *allocate_near(uintptr_t at) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    const uintptr_t step = info.dwAllocationGranularity;
    const uintptr_t center = at & ~(step - 1);
    for (uintptr_t delta = 0; delta < 0x7fff0000ULL; delta += step) {
        for (int direction = 0; direction < 2; direction++) {
            if (direction && delta == 0) continue;
            if (direction && center < delta) continue;
            uintptr_t candidate = direction ? center-delta : center+delta;
            if (!direction && candidate < center) continue;
            void *p = VirtualAlloc((void *)candidate, 4096, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (p) return (unsigned char *)p;
        }
    }
    return NULL;
}
static void write_jump(unsigned char *out, void (*target)(void)) {
    uintptr_t address = (uintptr_t)target;
    out[0] = 0x48; out[1] = 0xb8; // mov rax, imm64; jmp rax
    memcpy(out+2, &address, 8); out[10] = 0xff; out[11] = 0xe0;
}
static int write_relative_call(unsigned char *callsite, unsigned char *target) {
    intptr_t rel = (intptr_t)target - ((intptr_t)callsite + 5);
    if (rel < INT32_MIN || rel > INT32_MAX) return 0;
    int32_t displacement = (int32_t)rel;
    callsite[0] = 0xe8;
    memcpy(callsite+1, &displacement, 4);
    return 1;
}
