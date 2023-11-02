/**
\brief bmx160 driver.

\author Tengfei Chang <tengfei.chang@gmail.com>, Nov 2021.
*/

#include "i2c.h"
#include "bmx388.h"
#include "math.h"

//=========================== define ==========================================

typedef struct{

    //int16_t mag_x;
    //int16_t mag_y;
    //int16_t mag_z;

    //int16_t rhall;

    //int16_t gyr_x;
    //int16_t gyr_y;
    //int16_t gyr_z;

    //int16_t acc_x;
    //int16_t acc_y;
    //int16_t acc_z;

    uint8_t pres[3];
    uint8_t temp[3];
    
}bmx388x_data_t;

 //typedef struct bmp388_handle_s
 //{                                                                  
 //    uint16_t t1;                                                                        
 //    uint16_t t2;                                                                        
 //    int8_t t3;                                                                          
 //    int16_t p1;                                                                         
 //    int16_t p2;                                                                         
 //    int8_t p3;                                                                          
 //    int8_t p4;                                                                          
 //    uint16_t p5;                                                                        
 //    uint16_t p6;                                                                        
 //    int8_t p7;                                                                          
 //    int8_t p8;                                                                          
 //    int16_t p9;                                                                         
 //    int8_t p10;                                                                         
 //    int8_t p11;                                                                         
 //    int64_t t_fine;                                                                     
 //} bmp388x_compensation_t;

typedef struct {
    
    bmx388x_data_t bmx388x_data;
        uint32_t fullpres;
        uint32_t fulltemp;
        uint32_t sen_time;
        uint8_t res[21];
        double  PAR_T[3];//3个温度修正系数
	double  PAR_P[11];//11个气压修正系数
        float   t_fine;

}bmx388x_var_t;

//=========================== variables =======================================

bmx388x_var_t bmx388x_var;

//=========================== prototypes ======================================

//=========================== public ==========================================

// admin
uint8_t bmx388_who_am_i(void) {

    uint8_t chipid;
    i2c_read_bytes(BMX160_REG_ADDR_CHIPID, &chipid, 1);
    return chipid;
}

//void bmx160_config_wakeup(void) {
    
//}

uint8_t bmx160_power_on(void) {
    uint8_t pwr_cmd;
    pwr_cmd = BMX160_CMD_PWR_CTRL;
    i2c_write_bytes(BMX160_REG_ADDR_PWR_CTRL, &pwr_cmd,1);
}

uint8_t bmx160_get_pmu_status(void) {

    uint8_t pmu_status;
    i2c_read_bytes(BMX160_REG_ADDR_PMU_STATUS, &pmu_status, 1);
    return pmu_status;
}

uint8_t bmx160_get_pwr_status(void) {

    uint8_t pwr_status;
    i2c_read_bytes(BMX160_REG_ADDR_PWR_CTRL, &pwr_status, 1);
    return pwr_status;
}


void bmx160_set_cmd(uint8_t cmd) {
    i2c_write_bytes(BMX160_REG_ADDR_PWR_CTRL, &cmd, 1);
}

//// configuration

//void    bmx160_acc_config(uint8_t config) {
//    i2c_write_bytes(BMX160_REG_ADDR_ACC_CONF, &config, 1);
//}

//void    bmx160_gyr_config(uint8_t config) {
//    i2c_write_bytes(BMX160_REG_ADDR_GYR_CONF, &config, 1);
//}

//void    bmx160_mag_config(uint8_t config) {
//    i2c_write_bytes(BMX160_REG_ADDR_MAG_CONF, &config, 1);
//}

//// range & interface

//void    bmx160_acc_range(uint8_t range) {
//    i2c_write_bytes(BMX160_REG_ADDR_ACC_RANGE, &range, 1);
//}

//void    bmx160_gyr_range(uint8_t range) {
//    i2c_write_bytes(BMX160_REG_ADDR_GYR_RANGE, &range, 1);
//}

//void    bmx160_mag_if(uint8_t interface) {
//    i2c_write_bytes(BMX160_REG_ADDR_MAG_IF, &interface, 1);
//}


// read
void bmx160_read_9dof_data(void) {
    i2c_read_bytes(BMX160_REG_ADDR_DATA, (uint8_t*)(&bmx388x_var.bmx388x_data), sizeof(bmx388x_data_t));
    //i2c_read_bytes(BMX160_REG_ADDR_DATA, (uint8_t*)(&bmx388x_var.bmx388x_data), 1);
    bmx388x_var.fulltemp = (bmx388x_var.bmx388x_data.temp[2] << 16) | (bmx388x_var.bmx388x_data.temp[1] << 8) | bmx388x_var.bmx388x_data.temp[0];
    bmx388x_var.fullpres = (bmx388x_var.bmx388x_data.pres[2] << 16) | (bmx388x_var.bmx388x_data.pres[1] << 8) | bmx388x_var.bmx388x_data.pres[0];
}


//int16_t bmx160_read_acc_x(void) {

//    return bmx160x_var.bmx160x_data.acc_x;
//}
//int16_t bmx160_read_acc_y(void) {
    
//    return bmx160x_var.bmx160x_data.acc_y;
//}
//int16_t bmx160_read_acc_z(void) {
    
//    return bmx160x_var.bmx160x_data.acc_z;
//}

//int16_t bmx160_read_mag_x(void) {
    
//    return bmx160x_var.bmx160x_data.mag_x;
//}
//int16_t bmx160_read_mag_y(void) {
    
//    return bmx160x_var.bmx160x_data.mag_y;
//}
//int16_t bmx160_read_mag_z(void) {
    
//    return bmx160x_var.bmx160x_data.mag_z;
//}

//int16_t bmx160_read_gyr_x(void) {
    
//    return bmx160x_var.bmx160x_data.gyr_x;
//}
//int16_t bmx160_read_gyr_y(void) {
    
//    return bmx160x_var.bmx160x_data.gyr_y;
//}
//int16_t bmx160_read_gyr_z(void) {
    
//    return bmx160x_var.bmx160x_data.gyr_z;
//}

 uint8_t bmp388_get_sensortime(void)
 {
     
     i2c_read_bytes(BMX160_REG_ADDR_SENSORTIME, (uint8_t*)(&bmx388x_var.sen_time), sizeof(bmx388x_var.sen_time));

     return 0;                                                                              /* success return 0 */
 }
 void bmp388_get_compensation(void)
 {
     i2c_read_bytes(BMX160_REG_ADDR_COMP,(uint8_t*)(&bmx388x_var.res[0]),sizeof(bmx388x_var.res));
     bmx388x_var.PAR_T[0]=((bmx388x_var.res[1]<<8)|bmx388x_var.res[0])/pow(2,-8);
     bmx388x_var.PAR_T[1]=((bmx388x_var.res[3]<<8)|bmx388x_var.res[2])/pow(2,30);
     bmx388x_var.PAR_T[2]=(int8_t)bmx388x_var.res[4]/pow(2,48);//计算3个温度修正系数

     bmx388x_var.PAR_P[0]=(((int16_t)((bmx388x_var.res[6]<<8)|bmx388x_var.res[5]))-pow(2,14))/pow(2,20);
     bmx388x_var.PAR_P[1]=(((int16_t)((bmx388x_var.res[8]<<8)|bmx388x_var.res[7]))-pow(2,14))/pow(2,29);
     bmx388x_var.PAR_P[2]=(int8_t)bmx388x_var.res[9]/pow(2,32);
     bmx388x_var.PAR_P[3]=(int8_t)bmx388x_var.res[10]/pow(2,37);
     bmx388x_var.PAR_P[4]=((bmx388x_var.res[12]<<8)|bmx388x_var.res[11])/pow(2,-3);
     bmx388x_var.PAR_P[5]=((bmx388x_var.res[14]<<8)|bmx388x_var.res[13])/pow(2,6);
     bmx388x_var.PAR_P[6]=(int8_t)bmx388x_var.res[15]/pow(2,8);
     bmx388x_var.PAR_P[7]=(int8_t)bmx388x_var.res[16]/pow(2,15);
     bmx388x_var.PAR_P[8]=(int16_t)((bmx388x_var.res[18]<<8)|bmx388x_var.res[17])/pow(2,48);
     bmx388x_var.PAR_P[9]=(int8_t)bmx388x_var.res[19]/pow(2,48);
     bmx388x_var.PAR_P[10]=(int8_t)bmx388x_var.res[20]/pow(2,65);//计算11个温度修正系数
}

void bmp388_compensation_temp(void)
{
    float partial_data1;
    float partial_data2;

    partial_data1 = (float)(bmx388x_var.fulltemp-bmx388x_var.PAR_T[0]);
    partial_data2 = (float)(partial_data1*bmx388x_var.PAR_T[1]);

    bmx388x_var.t_fine = partial_data2+(partial_data1*partial_data1)*bmx388x_var.PAR_T[2];

}

////=========================== helper ==========================================

//float bmx160_from_lsb_to_celsius(int16_t lsb) {

//}

//float bmx160_from_fs8_lp1_to_mg(int16_t lsb) {

//}

////=========================== private =========================================