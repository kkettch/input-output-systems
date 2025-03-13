#include "kernel.h"

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

long sbi_get_impl_version(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0x2, 0x10); // FID=0, EID=0x10 (SBI_BASE)
    return ret.value; // SBI version is returned in value
}
long sbi_hart_get_status(long hart_id) {
    struct sbiret ret = sbi_call(hart_id, 0, 0, 0, 0, 0, 2, 0x48534D);
    return ret.value; // Returns hart status
}
void kernel_main(void) {
    long sbi_version = sbi_get_impl_version();
    
    putchar('\n');
    putchar('S'); putchar('B'); putchar('I'); putchar(' ');
    putchar('v');
    putchar('0' + ((sbi_version >> 24) & 0xF)); // Major version
    putchar('.');
    putchar('0' + ((sbi_version >> 0) & 0xF));  // Minor version
    putchar('\n');

    // const char *s = "Hello World!\n";
    // for (int i = 0; s[i] != '\0'; i++) {
    //     putchar(s[i]);
    // }

    for (;;) {
        long ch = getchar();
        if (ch > 0) {
            putchar((char)ch);
        }
    }
}
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Устанавливаем указатель стека
        "j kernel_main\n"       // Переходим к функции main ядра
        :
        : [stack_top] "r" (__stack_top) // Передаём верхний адрес стека в виде %[stack_top]
    );
}