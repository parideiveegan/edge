#include "sensor.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   4
#define I2C_MASTER_SCL_IO   5
#define I2C_MASTER_FREQ_HZ  400000

#define QMA7981_ADDR        0x12
#define REG_X_LSB           0x01

void i2c_master_init(void)
{
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

esp_err_t qma_write(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    return i2c_master_write_to_device(I2C_MASTER_NUM, QMA7981_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

esp_err_t qma_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, QMA7981_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

void imu_read_g(float *xg, float *yg, float *zg)
{
    uint8_t d[6];
    qma_read(REG_X_LSB, d, 6);

    int16_t xi = ((int16_t)(d[1] << 8 | d[0])) >> 4;
    int16_t yi = ((int16_t)(d[3] << 8 | d[2])) >> 4;
    int16_t zi = ((int16_t)(d[5] << 8 | d[4])) >> 4;

    *xg = xi / 1024.0f;
    *yg = yi / 1024.0f;
    *zg = zi / 1024.0f;
}