#include "nv.h"
#include "stm32f3xx_hal.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

// ===== НАСТРОЙКИ =====
#define EEPROM_ADDR_7B   0x50u
#define EEPROM_ADDR      (EEPROM_ADDR_7B << 1)
#define EEPROM_PAGE_SZ   32u
#define NV_SLOT0_BASE    0x0000
#define NV_SLOT1_BASE    0x0100
#define NV_MAGIC         0x4F444F33u
#define NV_VERSION       02u

#define I2C_TIMEOUT_MS   50
#define WRITE_RETRY_MAX  3
#define RECOVERY_DELAY   10

// ===== ВНУТРЕННЯЯ FLASH =====
// Адрес в конце Flash (последняя страница, 2 КБ)
#define FLASH_BACKUP_ADDR  0x0801F800  // для STM32F303 с 64 КБ Flash
// Проверь в даташите точный адрес последней страницы!

#define FLASH_MAGIC        0xE30B

// Глобальная переменная для флагов ошибок
volatile uint8_t g_system_error = 0;

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t seq;
    uint16_t pulse_rem;
    uint16_t data_len;
    ui_data_t data;
    uint32_t crc32;
} nv_blob_t;
#pragma pack(pop)

static uint32_t g_last_seq = 0;
static uint8_t flash_initialized = 0;

// ===== CRC32 =====
static uint32_t crc32_calc(const void *data, uint32_t len)
{
    const uint8_t *p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++) {
            uint32_t m = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & m);
        }
    }
    return ~crc;
}

// ===== I²C функции (как были) =====
static void i2c_recover(void)
{
    HAL_I2C_DeInit(&hi2c1);
    HAL_Delay(RECOVERY_DELAY);
    extern void MX_I2C1_Init(void);
    MX_I2C1_Init();
    HAL_Delay(RECOVERY_DELAY);
    HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDR, 3, 100);
}

static int eep_wait_ready(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDR, 1, 5) == HAL_OK)
            return 0;
        HAL_Delay(1);
    }
    return -1;
}

static int eep_read(uint16_t mem, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, mem,
                         I2C_MEMADD_SIZE_16BIT, buf, len, I2C_TIMEOUT_MS) != HAL_OK)
        return -1;
    return 0;
}

static int eep_write_reliable(uint16_t mem_addr, const uint8_t *buf, uint16_t len)
{
    for (int attempt = 0; attempt < WRITE_RETRY_MAX; attempt++) {
        if (attempt > 0) {
            HAL_Delay(RECOVERY_DELAY * attempt);
        }

        uint16_t remaining = len;
        uint16_t addr = mem_addr;
        const uint8_t *ptr = buf;
        int ok = 1;

        while (remaining) {
            uint16_t page_off = addr % EEPROM_PAGE_SZ;
            uint16_t chunk = EEPROM_PAGE_SZ - page_off;
            if (chunk > remaining) chunk = remaining;

            if (HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr,
                                   I2C_MEMADD_SIZE_16BIT, (uint8_t*)ptr, chunk, I2C_TIMEOUT_MS) != HAL_OK) {
                ok = 0;
                break;
            }

            if (eep_wait_ready(20) < 0) {
                ok = 0;
                break;
            }

            addr += chunk;
            ptr += chunk;
            remaining -= chunk;
        }

        if (ok) return 0;

        i2c_recover();
    }
    return -1;
}

static uint8_t blob_is_valid(const nv_blob_t *b)
{
    if (b->magic != NV_MAGIC) return 0;
    if (b->version != NV_VERSION) return 0;
    if (b->data_len != sizeof(ui_data_t)) return 0;
    if (b->pulse_rem >= 4838u) return 0;

    uint32_t need = crc32_calc(b, sizeof(nv_blob_t) - 4);
    return (need == b->crc32);
}

// ===== РАБОТА С FLASH =====

static void flash_unlock(void)
{
    HAL_FLASH_Unlock();
}

static void flash_lock(void)
{
    HAL_FLASH_Lock();
}

static int flash_erase_page(void)
{
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = FLASH_BACKUP_ADDR,
        .NbPages = 1
    };
    uint32_t page_error = 0;

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        return -1;
    }
    return 0;
}

void NV_BackupToFlash(const ui_data_t *d, uint16_t pulse_rem)
{
    nv_blob_t blob;
    memset(&blob, 0, sizeof(blob));

    blob.magic = NV_MAGIC;
    blob.version = NV_VERSION;
    blob.seq = g_last_seq + 1;  // не очень актуально для flash, но пусть
    blob.pulse_rem = (pulse_rem < 4838u) ? pulse_rem : 0;
    blob.data_len = sizeof(ui_data_t);
    memcpy(&blob.data, d, sizeof(ui_data_t));
    blob.crc32 = crc32_calc(&blob, sizeof(nv_blob_t) - 4);

    flash_unlock();

    // Стираем страницу (только если нужно)
    if (!flash_initialized) {
        if (flash_erase_page() != 0) {
            flash_lock();
            g_system_error |= ERR_FLASH;
            return;
        }
        flash_initialized = 1;
    }

    // Записываем по 64 бита (для ускорения)
    uint64_t *src = (uint64_t*)&blob;
    uint32_t words = sizeof(nv_blob_t) / 8;

    for (uint32_t i = 0; i < words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                               FLASH_BACKUP_ADDR + i*8, src[i]) != HAL_OK) {
            flash_lock();
            g_system_error |= ERR_FLASH;
            return;
        }
    }

    flash_lock();
    g_system_error &= ~ERR_FLASH;
}

int NV_RestoreFromFlash(ui_data_t *d, uint16_t *pulse_rem)
{
    nv_blob_t *blob = (nv_blob_t*)FLASH_BACKUP_ADDR;

    // Проверяем валидность
    if (blob->magic != NV_MAGIC) return -1;
    if (blob->version != NV_VERSION) return -1;
    if (blob->data_len != sizeof(ui_data_t)) return -1;
    if (blob->pulse_rem >= 4838u) return -1;

    uint32_t need = crc32_calc(blob, sizeof(nv_blob_t) - 4);
    if (need != blob->crc32) return -1;

    memcpy(d, &blob->data, sizeof(ui_data_t));
    *pulse_rem = blob->pulse_rem;
    g_last_seq = blob->seq;

    return 0;
}

void NV_ClearFlashBackup(void)
{
    flash_unlock();
    flash_erase_page();
    flash_lock();
    flash_initialized = 0;
}

// ===== ЗАГРУЗКА =====

int NV_Load(ui_data_t *d, uint16_t *pulse_rem)
{
    nv_blob_t a, b;
    int ra = eep_read(NV_SLOT0_BASE, (uint8_t*)&a, sizeof(nv_blob_t));
    int rb = eep_read(NV_SLOT1_BASE, (uint8_t*)&b, sizeof(nv_blob_t));

    uint8_t va = (ra == 0) ? blob_is_valid(&a) : 0;
    uint8_t vb = (rb == 0) ? blob_is_valid(&b) : 0;

    if (!va && !vb) {
        // Внешняя память не работает - пробуем Flash
        g_system_error |= ERR_EEPROM;
        if (NV_RestoreFromFlash(d, pulse_rem) == 0) {
            return 0;  // загрузились из Flash
        }
        return -1;  // всё плохо
    }

    const nv_blob_t *best = NULL;
    if (va && vb) best = (a.seq >= b.seq) ? &a : &b;
    else if (va) best = &a;
    else best = &b;

    memcpy(d, &best->data, sizeof(ui_data_t));
    *pulse_rem = best->pulse_rem;
    g_last_seq = best->seq;

    g_system_error &= ~ERR_EEPROM;
    return 0;
}

// ===== СОХРАНЕНИЕ =====

int NV_Save(const ui_data_t *d, uint16_t pulse_rem)
{
    nv_blob_t blob;
    memset(&blob, 0, sizeof(blob));

    blob.magic = NV_MAGIC;
    blob.version = NV_VERSION;
    blob.seq = g_last_seq + 1;
    blob.pulse_rem = (pulse_rem < 4838u) ? pulse_rem : 0;
    blob.data_len = sizeof(ui_data_t);
    memcpy(&blob.data, d, sizeof(ui_data_t));
    blob.crc32 = crc32_calc(&blob, sizeof(nv_blob_t) - 4);

    uint16_t base = (blob.seq & 1) ? NV_SLOT1_BASE : NV_SLOT0_BASE;

    int result = eep_write_reliable(base, (const uint8_t*)&blob, sizeof(nv_blob_t));

    if (result != 0) {
        // Ошибка записи - сохраняем во Flash
        NV_BackupToFlash(d, pulse_rem);
        g_system_error |= ERR_EEPROM;
    } else {
        // Успешно - очищаем флаг
        g_system_error &= ~ERR_EEPROM;
        g_last_seq = blob.seq;
        // Flash не чистим - оставляем как резерв
    }

    return result;
}

void NV_Init(void)
{
    // Ничего не делаем при инициализации
}
