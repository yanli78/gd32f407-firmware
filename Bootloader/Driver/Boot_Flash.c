#include "Boot_Flash.h"

uint8_t Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);
    while(addr < end) {
        if(fmc_page_erase(addr) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        addr += 4096U;
    }
    fmc_lock();
    return 1U;
}

uint8_t Boot_Flash_Write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR);
    for(i = 0U; i < size; i++) {
        if(fmc_byte_program(addr + i, data[i]) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        if(*(volatile uint8_t *)(addr + i) != data[i]) {
            fmc_lock();
            return 0U;
        }
    }
    fmc_lock();
    return 1U;
}
