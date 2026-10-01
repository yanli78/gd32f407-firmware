#include "Boot_Flash.h"

/* FMC 页大小（GD32F4xx 主存储区为 4KB/页） */
#define BOOT_FMC_PAGE_SIZE  4096U

/* 擦除 [addr, addr + size)：地址与长度都必须页对齐 */
uint8_t Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    uint32_t end;

    if((size == 0U) || ((size % BOOT_FMC_PAGE_SIZE) != 0U)) return 0U;
    if((addr % BOOT_FMC_PAGE_SIZE) != 0U) return 0U;
    end = addr + size;

    fmc_unlock();
    /* 清掉上一次操作遗留的错误标志，否则后续编程会被直接拒绝 */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR |
                   FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);
    while(addr < end) {
        if(fmc_page_erase(addr) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        fmc_flag_clear(FMC_FLAG_END);
        addr += BOOT_FMC_PAGE_SIZE;
    }
    fmc_lock();
    return 1U;
}

/* 写片内 Flash：主体 32 位字编程 + 收尾字节编程，写完回读校验。
   升级镜像最大 128KB，逐字节编程需要约 13 万次编程操作，
   改成字编程后降到约 3.3 万次，IAP 写入时间相应缩短到约 1/4。 */
uint8_t Boot_Flash_Write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i = 0U;
    uint32_t word;

    if(size == 0U) return 1U;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR |
                   FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);

    /* 1) 起始地址未 4 字节对齐：先按字节写 */
    while(((addr + i) % 4U) != 0U && (i < size)) {
        if(fmc_byte_program(addr + i, data[i]) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        if(*(volatile uint8_t *)(addr + i) != data[i]) {
            fmc_lock();
            return 0U;
        }
        i++;
    }

    /* 2) 主体：32 位字编程 */
    while((size - i) >= 4U) {
        word = (uint32_t)data[i] |
               ((uint32_t)data[i + 1U] << 8U) |
               ((uint32_t)data[i + 2U] << 16U) |
               ((uint32_t)data[i + 3U] << 24U);
        if(fmc_word_program(addr + i, word) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        if(*(volatile uint32_t *)(addr + i) != word) {
            fmc_lock();
            return 0U;
        }
        i += 4U;
    }

    /* 3) 收尾不足 4 字节：按字节写 */
    while(i < size) {
        if(fmc_byte_program(addr + i, data[i]) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        if(*(volatile uint8_t *)(addr + i) != data[i]) {
            fmc_lock();
            return 0U;
        }
        i++;
    }

    fmc_lock();
    return 1U;
}
