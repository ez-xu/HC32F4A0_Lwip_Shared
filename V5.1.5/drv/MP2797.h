#ifndef _MP2797_H
#define _MP2797_H
#include "main.h"

typedef struct
{
	uint16_t VCELL[16];
	uint16_t VGPIO[4];
	int16_t  ICELL1;
	
	uint8_t  ChipSystemError;	 
	uint16_t ADC_STS;	
	uint16_t ADC_CTRL;
	uint16_t RD_INT0;	
	union
	{
		uint16_t CC_CFG; 
		struct
		{
			unsigned cc_en :1;			   
			unsigned cc_en_safe :1;
			unsigned rev1 :1;
			unsigned cc_pwr_save :1;
			unsigned cc_done :1;		   
			unsigned cc_error_sts :1;
			unsigned rev2 :2;
			unsigned cc_int_cnt :6;		   
			unsigned cc_b2b_acc_ctrl :1;   
			unsigned rev3 :1;
		}bits;
	}unCC_cfg;
	uint32_t CC_Bits;
	double CC_Temp;
	
	uint32_t VscanCnt;
	uint32_t VScanTime;
	uint32_t CCTime;
	
	double U2;
	double Uh;
	double VBat;
	double VJY2;
	double VJY2_JY1;	
	
}BMS_REG;
extern BMS_REG BMSData;

#define REG_CELL_CONFIG 					0x00    	 
#define REG_PWR_STATUS						0x01		 
#define REG_STB_STATUS						0x02		 
#define REG_LOAD_CHGR_STATUS				0x03		 

#define REG_ACT_CONFIG					 	0x05		 
#define REG_STB_CONFIG						0x06		 
#define REG_SAFE_CONFIG						0x07		 
#define REG_REG_CONFIG						0x08		 
#define REG_LOAD_CHGR_CONFIG				0x09		 
#define REG_GPIO_STATUS						0x0A		 
#define REG_GPIO_OUT						0x0B		 
#define REG_GPIO_CONFIG						0x0C		 
#define REG_PINS_CONFIG						0x0D		 
#define REG_WDT_STATUS						0x0E		 
#define REG_WDT_RST							0x0F		 
#define REG_WDT_CONFIG						0x10		 
#define REG_FET_STATUS						0x11		 
#define REG_FET_CTRL						0x12		 
#define REG_FET_MODE						0x13		 
#define REG_FET_CONFIG						0x14		 
#define REG_RD_INT0							0x15		 
#define REG_RD_INT1							0x16		 
#define REG_INT0_CLR				  		0x17		 
#define REG_INT1_CLR 						0x18		 
#define REG_INT0_EN							0x19		 
#define REG_INT1_EN							0x1A		 
#define REG_INT_TYPE0						0x1B		 
#define REG_INT_TYPE1						0x1C		 
#define REG_INT_TYPE2						0x1D		 
//#define REG_INT_TYPE3				  		0x1E	 
//#define REG_INT_TYPE4						0x1F	 
#define REG_MASK_INT0				  		0x1E		 
#define REG_MASK_INT1						0x1F		 
#define REG_OC_STATUS						0x20		 
 
//#define REG_OC_MASK						0x22	 
#define REG_OCFT_CTRL						0x23		 
#define REG_DSGOC_LIM						0x24	 
#define REG_DSGOC_DEG						0x25		 
#define REG_CHGOC_DEG						0x26		 
#define REG_SC_STATUS						0x27	 
//#define REG_SC_MASK						0x28	 
 
#define REG_SCFT_CTRL						0x2A		 
#define REG_DSGSC_CONFIG					0x2B		 
#define REG_CHGSC_CONFIG					0x2C		 
#define REG_RD_CELL_UV						0x2D		 
#define REG_RD_CELL_OV						0x2E		 
#define REG_RD_CELL_MSMT					0x2F		 
#define REG_RD_CELL_DEAD					0x30	 
//#define REG_CELLUV_MASK					0x31 
//#define REG_CELLOV_MASK					0x32	 
//#define REG_CELLMSMT_MASK					0x33	 
#define REG_CELL_MSMT_STS					0x33		 
#define REG_PACKFT_CTRL						0x34		 
#define REG_CELLFT_CTRL						0x35		 
#define REG_CELL_HYST						0x36		 
//#define REG_PACK_HYST						0x37	 
#define REG_PACK_UV_OV						0x37		 
#define REG_CELL_UV							0x38		 
#define REG_CELL_OV							0x39		 
#define REG_PACK_UV							0x3A		 
#define REG_PACK_OV							0x3B		 
#define REG_CELL_DEAD_THR					0x3C		 
#define REG_CELL_MSMT						0x3D		 
#define REG_RD_NTC_DIE						0x3E		 
#define REG_RD_VANTC4						0x3F		 
#define REG_RD_VANTC3						0x40		 
#define REG_RD_VANTC2						0x41		 
#define REG_RD_VANTC1						0x42		 
#define REG_RD_T_DIE						0x43		 
#define REG_NTC_CLR							0x44		 
//#define REG_NTC_MASK						0x45	 
#define REG_DIE_CONFIG						0x46		 
#define REG_NTC_CONFIG						0x47		 
#define REG_OTHR_DSG						0x48	 
#define REG_UTHR_DSG						0x49		 
#define REG_OTHR_CHG						0x4A		 
#define REG_UTHR_CHG						0x4B		 
#define REG_NTCM_OTHR						0x4C		 
#define REG_DIE_OT							0x4D		 
#define REG_SELF_STS						0x4E	 
#define REG_RD_VA1P8						0x4F 
#define REG_RD_VA3P3						0x50		 
#define REG_RD_VA5							0x51	 
#define REG_RD_VASELF						0x52		 
#define REG_RD_OPENH						0x53	 
#define REG_RD_OPENL						0x54	 
#define REG_SFT_GO							0x55		 
#define REG_SELF_CFG						0x56	 
#define REG_OPEN_CFG						0x57		 
#define REG_REGIN_UV						0x58	 
#define REG_V3P3_UV							0x59		 
#define REG_VDD_UV							0x5A		 
#define REG_SELF_THR						0x5B	 
 														 
#define REG_FT_STS1							0x5D		 
#define REG_FT_STS2							0x5E		 
#define REG_FT_CLR							0x5F		 
#define REG_FT_REC							0x60		 
#define REG_FT0_CONFIG						0x61		 
#define REG_FT1_CONFIG						0x62	 
#define REG_FT2_CONFIG						0x63		 
 
#define REG_RD_CCIRQL						0x65		 
#define REG_RD_CCIRQH						0x66	 
#define REG_RD_CCACCQL						0x67		 
#define REG_RD_CCACCQH						0x68		 
#define REG_RD_VPACKP						0x69		 
#define REG_RD_VTOP							0x6A		 
#define REG_RD_ITOP							0x6B		 
#define REG_RD_VCELL1						0x6C	 
#define REG_RD_ICELL1						0x6D	 
#define REG_RD_VCELL2						0x6E	 
#define REG_RD_ICELL2						0x6F	 
#define REG_RD_VCELL3						0x70	 
#define REG_RD_ICELL3						0x71	 
#define REG_RD_VCELL4						0x72	 
#define REG_RD_ICELL4						0x73		 
#define REG_RD_VCELL5						0x74		 
#define REG_RD_ICELL5						0x75		 
#define REG_RD_VCELL6						0x76		 
#define REG_RD_ICELL6						0x77	 
#define REG_RD_VCELL7						0x78		 
#define REG_RD_ICELL7						0x79		 
#define REG_RD_VCELL8						0x7A		 
#define REG_RD_ICELL8						0x7B		 
#define REG_RD_VCELL9						0x7C	 
#define REG_RD_ICELL9						0x7D	 
#define REG_RD_VCELL10						0x7E		 
#define REG_RD_ICELL10						0x7F		 
#define REG_RD_VCELL11						0x80		 
#define REG_RD_ICELL11						0x81		 
#define REG_RD_VCELL12						0x82	 
#define REG_RD_ICELL12						0x83		 
#define REG_RD_VCELL13						0x84		 
#define REG_RD_ICELL13						0x85		 
#define REG_RD_VCELL14						0x86		 
#define REG_RD_ICELL14						0x87		 
#define REG_RD_VCELL15						0x88		 
#define REG_RD_ICELL15					 	0x89		 
#define REG_RD_VCELL16						0x8A		 
#define REG_RD_ICELL16						0x8B		 
#define REG_RD_VNTC4						0x8C		 
#define REG_RD_VNTC3						0x8D		 
#define REG_RD_VNTC2						0x8E		 
#define REG_RD_VNTC1						0x8F	 
#define REG_RD_VGPIO3						0x90	 
#define REG_RD_VGPIO2						0x91		 
#define REG_RD_VGPIO1						0x92		 
#define REG_RD_TDIE							0x93		 
#define REG_RD_V1P8							0x94		 
#define REG_RD_V3P3							0x95		 
#define REG_RD_V5							0x96		 
#define REG_CC_STS							0x97	 
#define REG_ADC_STS							0x98		 
#define REG_ADC_CTRL						0x99		 
#define REG_CC_CONFIG						0x9A		 
#define REG_TRIMG_IPCB						0x9B		 
#define REG_HRSCAN0							0x9C		 
#define REG_HRSCAN1							0x9D		 
#define REG_HRSCAN2							0x9E		 
#define REG_COMM_STS						0x9F		 
#define REG_SILC_INFO1						0xA0	 
#define REG_SILC_INFO2						0xA1		 
#define REG_TEST_REV						0xA2		 
#define REG_COMM_CONFIG						0xA3		 
#define REG_BAL_STS							0xA4		 
#define REG_BAL_LIST						0xA5		 
#define REG_BAL_CTRL						0xA6		 
#define REG_BAL_CONFIG						0xA7		 
#define REG_BAL_THR							0xA8		 

#define USE_CRC								1
#define NO_CRC								0

#define I_SENSE_R							1

#define Current_Offset						0	

#define R_TEMP_MEAS_BAT						10000	
#define BETA_NTC_BAT						3380

#define R_TEMP_MEAS_PCB						10000
#define BETA_NTC_PCB						3380

void MP2797_Init(uint8_t mode);
void MP2797_Comm(uint8_t mode);

#endif
