/**
\brief registers address mapping of bmx160 sensor.

\author Tengfei Chang <tengfei.chang@gmail.com>, Nov 2021.
*/

#include "stdint.h"

//=========================== define ==========================================

#define BMX388_ADDR 0x76

//---- register addresses

#define BMX160_REG_ADDR_CHIPID      0x00
#define BMX160_REG_ADDR_ERR_REG     0x02
#define BMX160_REG_ADDR_PMU_STATUS  0x03

// sensor data

#define BMX160_REG_ADDR_DATA        0x04
#define BMX160_REG_ADDR_PRESURE     0x04
#define BMX160_REG_ADDR_TEMPATURE   0x07
#define BMX160_REG_ADDR_SENSORTIME  0x0C
#define BMX160_REG_ADDR_EVENT       0x10
#define BMX160_REG_ADDR_INT_STATUS  0x11
#define BMX160_REG_ADDR_FIFO_LENGTH 0x12
#define BMX160_REG_ADDR_FIFO_DATA   0x14
#define BMX160_REG_ADDR_FIFO_WTM    0x15
#define BMX160_REG_ADDR_FIFO_CONFIG1 0x17
#define BMX160_REG_ADDR_FIFO_CONFIG2 0x18
#define BMX160_REG_ADDR_INT_CTRL    0x19
#define BMX160_REG_ADDR_INT_CONF    0x1A
#define BMX160_REG_ADDR_PWR_CTRL    0x1B
#define BMX160_REG_ADDR_OSR         0x1C
#define BMX160_REG_ADDR_ODR         0x1D
#define BMX160_REG_ADDR_COMP        0x31

//---- register values

//#define BMX160_CMD_PMU_ACC_SUSPEND        0x10
//#define BMX160_CMD_PMU_ACC_NORMAL         0x11
//#define BMX160_CMD_PMU_ACC_LOW_POWER      0x12

//#define BMX160_CMD_PMU_GYR_SUSPEND        0x14
//#define BMX160_CMD_PMU_GYR_NORMAL         0x15
//#define BMX160_CMD_PMU_GYR_FSU            0x17

//#define BMX160_CMD_PMU_MAG_IF_SUSPEND     0x18
//#define BMX160_CMD_PMU_MAG_IF_NORMAL      0x19
//#define BMX160_CMD_PMU_MAG_IF_LOW_POWER   0x1A
#define BMX160_CMD_PWR_CTRL 0x33

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

// admin
uint8_t bmx388_who_am_i(void);
uint8_t bmx160_power_on(void);
uint8_t bmx160_get_pmu_status(void);
uint8_t bmx160_get_pwr_status(void);
uint8_t bmp388_get_sensortime(void);

void    bmx160_set_cmd(uint8_t cmd);
void    bmx160_acc_config(uint8_t config);
void    bmx160_gyr_config(uint8_t config);
void    bmx160_mag_config(uint8_t config);

void    bmx160_acc_range(uint8_t range);
void    bmx160_gyr_range(uint8_t range);
void    bmx160_mag_if(uint8_t interface);

// read
int16_t bmx160_read_temperature(void);

int16_t bmx160_read_acc_x(void);
int16_t bmx160_read_acc_y(void);
int16_t bmx160_read_acc_z(void);

int16_t bmx160_read_mag_x(void);
int16_t bmx160_read_mag_y(void);
int16_t bmx160_read_mag_z(void);

int16_t bmx160_read_gyr_x(void);
int16_t bmx160_read_gyr_y(void);
int16_t bmx160_read_gyr_z(void);

void    bmx160_read_9dof_data(void);
void    bmp388_get_compensation(void);

void bmp388_compensation_temp(void);

//=========================== private =========================================