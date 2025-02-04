#include "gxcore.h"
#include "app_common_porting_stb_api.h"
#include "app_ds_ads_porting_stb_api.h"
#include "app_common_porting_ca_demux.h"
#include "app_common_porting_ca_os.h"

#define DSADS_P(...)                          do {                                            \
                                                    printf(" ");                        \
                                                    printf(__VA_ARGS__);                        \
                                                } while(0)
static uint8_t dsadPrintBuff[1024];
static uint32_t dsadPrintEnable = 1;
static FilterNotifyFunction gpfDataNotifyFunction[DSAD_TOTAL_TYPE] = {NULL,};
BOOL full_screen_ad_need_show = FALSE;
ST_DSAD_PROGRAM_SHOW_INFO full_screen_pic_info;

static void app_desai_ad_filter_notify(handle_t Filter, const uint8_t* Section, size_t Size)
{
	uint16_t            filterID = 0;
	uint16_t            i = 0;
	int16_t            pid = 0x1fff;
	uint16_t            section_length = 0;
	bool bFlag = FALSE;
	uint8_t byReqID = 0;
	uint8_t*            data = (uint8_t*)Section;
	int                 len = Size;
	ca_filter_t* sfilter = NULL;
	ca_filter_t filter = {0};

	int32_t ret;
	int j=0;

	if(!Filter)
	{
		return;
	}
	if(!Section)
	{
		return;
	}
	if(!Size)
	{
		return;
	}

   	ret = GxDemux_FilterGetPID(Filter,&pid);
	CHECK_RET(DEMUX,ret);
	while(len > 0)
	{
		if(app_porting_ca_section_filter_crc32_check(data))
		{
			break;
		}
		bFlag = FALSE;
		section_length = ((data[1] & 0x0F) << 8) + data[2] + 3;

		if(section_length > len)
		{
			ADS_ERROR("section len is wrong!!%d>%d\n",section_length,len);
			return;
		}
		for (filterID = 0; filterID< MAX_FILTER_COUNT; filterID++)
		{
		    sfilter = app_porting_ca_demux_get_by_filter_id(filterID,&filter);
			if(!filter.usedStatus)
			{
				continue;
			}
			if(pid != filter.pid)
			{
				continue;
			}
			if(!filter.channelhandle || !filter.handle)
			{
				continue;
			}
			if(Filter != filter.handle)
			{
				continue;
			}

			/*NIT表或相应FILTER数据*/
			for (i = 0; i<filter.filterLen; i++)
			{
				if ((data[i]&filter.mask[i]) != (filter.match[i]&filter.mask[i]))
				{
					printf("data[%d]=0x%x cafilter[filterID].match[%d]=0x%x \n",
						i,data[i],i,filter.match[i]);
					return;
				}
			}

			//sfilter->nms = 0;
			byReqID = filter.byReqID;
			bFlag = TRUE;
			break;
		}

		if (FALSE == bFlag)
		{
			ADS_FAIL(" FALSE == bFlag\n");
			return;
		}
		if((filter.byReqID != DSAD_DELECMD_DATA) && (filter.byReqID != DSAD_CONFIG_DATA))
		{
			//printf("DSAD rcv data,pid=0x%x,section_length=%d,reqId=%d\n",pid,section_length,filter.byReqID);
		}
	//	GxDemux_FilterDisable(Filter);
		if(gpfDataNotifyFunction[filter.byReqID])
		{
			//if((sfilter->byReqID == DSAD_DELECMD_DATA) ||(sfilter->byReqID == DSAD_CONFIG_DATA))
			gpfDataNotifyFunction[filter.byReqID](pid,data,section_length);
		}

		j ++;
		data += section_length;
		len -= section_length;
	}
	return;
}


static void app_desai_ad_filter_timeout_notify(handle_t Filter, const uint8_t* Section, size_t Size)
{
	uint8_t	filterId;
	int16_t	pid;
	ca_filter_t filter = {0};
	int32_t ret;

   	ret = GxDemux_FilterGetPID(Filter,&pid);
	CHECK_RET(DEMUX,ret);
	for (filterId= 0; filterId< MAX_FILTER_COUNT; filterId++)
	{
		app_porting_ca_demux_get_by_filter_id(filterId,&filter);
		if (( 0 != filter.usedStatus)&&(0 != filter.handle)
			&&(0 != filter.channelhandle)&&(Filter == filter.handle)&&(pid == filter.pid))
		{
			if(gpfDataNotifyFunction[filter.byReqID])
			{
				gpfDataNotifyFunction[filter.byReqID](pid,NULL,0);
			}
		}
	}
	return;

}

void DSAD_Printf(OUT const INT8 *string,...)
{
#if 0
	va_list args;
	int i;
	if (dsadPrintEnable == 1)
	{
		va_start(args,string);
		i = Vsnprintf((char *)dsadPrintBuff,sizeof(dsadPrintBuff), string, args);
		va_end(args);
		DSADS_P("%s", dsadPrintBuff);
	}
#endif
}

BOOL DS_AD_Flash_Read(UINT32 puiStartAddr,  UINT8 *pucData,  UINT32 uiLen)
{
	ADS_Dbg("%s>>puiStartAddr=0x%x,pucData=%p,uiLen=%d\n",
		__FUNCTION__,puiStartAddr,pucData,uiLen);
	return app_ds_ad_flash_read(puiStartAddr,pucData,uiLen);
}
BOOL DS_AD_Flash_Write(UINT32 puiStartAddr,  UINT8 *pucData,  UINT32 uiLen)
{
	ADS_Dbg("%s>>puiStartAddr=0x%x,pucData=%p,uiLen=%d\n",
		__FUNCTION__,puiStartAddr,pucData,uiLen);
	return app_ds_ad_flash_write(puiStartAddr,pucData,uiLen);

}
BOOL DS_AD_Flash_Erase(UINT32 puiStartAddr,  UINT32 uiLen)
{
	ADS_Dbg("%s>>puiStartAddr=0x%x,uiLen=%d\n",__FUNCTION__,puiStartAddr,uiLen);
	return app_ds_ad_flash_erase(puiStartAddr,uiLen);
}


void* DSAD_Malloc(UINT32 uiBufferSize)
{
	return app_porting_ca_os_malloc(uiBufferSize);
}
void DSAD_Free(IN void* pucBuffer)
{
	app_porting_ca_os_free(pucBuffer);
}
void DSAD_Memset(IN void* pucBuffer, UINT8 ucValue, UINT32 uiSize)
{
	memset(pucBuffer,ucValue,uiSize);
}
void DSAD_Memcpy(IN void* pucDestBuffer, IN void* pucSourceBuffer, UINT32 uiSize)
{
	memcpy(pucDestBuffer,pucSourceBuffer,uiSize);
}
INT32 DSAD_Memcmp(IN void* pucDestBuffer, IN void* pucSourceBuffer, UINT32 uiSize)
{
	return memcmp(pucDestBuffer,pucSourceBuffer,uiSize);
}
INT32 DSAD_StrLen(IN const UINT8* pucFormatBuffer)
{
	return strlen((char *)pucFormatBuffer);
}
INT32 DSAD_Sprintf(IN UINT8* pucDestBuffer, IN const UINT8* pucFormatBuffer, ...)
{
	va_list args;
	int i;

	va_start(args, pucFormatBuffer);
	i = vsprintf((char*)pucDestBuffer, (char*)pucFormatBuffer, args);
	va_end(args);
	return i;
}

void DSAD_Sleep(UINT16 usMilliSeconds)
{
	app_porting_ca_os_sleep(usMilliSeconds);
}
BOOL DSAD_RegisterTask (IN INT8* pucName, UINT8 ucPriority, IN void* pTaskFun )
{
	int 		ret;
	handle_t	handle;
	uint32_t priority = GXOS_DEFAULT_PRIORITY;

	if ((NULL == pucName)||(NULL == pTaskFun))
	{
		ADS_Dbg("DSADS_RegisterTask szName=%s  pTaskFun=0x%x\n",pucName,(unsigned int)pTaskFun);
		return 0;
	}

	ADS_Dbg("DSADS_RegisterTask szName=%s	ucPriority=%d\n",pucName,priority);

	ret = app_porting_ca_os_create_task(pucName, &handle, (void*)pTaskFun,
			  NULL, 1024*20, priority);

	return 1;
}

void DSAD_SemaphoreInit(IN DSAD_Semaphore* puiSemaphore , UINT8 ucInitVal)
{
	handle_t semaphore = 0;

	if (NULL == puiSemaphore)
	{
		return ;
	}
	app_porting_ca_os_sem_create(&semaphore,ucInitVal);
	*puiSemaphore = (handle_t)semaphore;
	ADS_Dbg("%s:	%d	%d\n",__FUNCTION__,*puiSemaphore,ucInitVal);
	return ;
}

void DSAD_ReleaseSemaphore(IN DSAD_Semaphore* puiSemaphore)
{
	if (NULL == puiSemaphore)
	{
		return ;
	}
	app_porting_ca_os_sem_signal((handle_t)*puiSemaphore);

	return ;
}

void DSAD_WaitSemaphore(IN DSAD_Semaphore* puiSemaphore)
{
	if (NULL == puiSemaphore)
	{
		return ;
	}
	app_porting_ca_os_sem_wait((handle_t)*puiSemaphore);

	return ;
}

BOOL DSAD_MsgQueueInit
	(IN INT8* pucName,
	UINT32* uiMsgQueueHandle,
	UINT32 uiMaxMsgLen,
	EN_DSAD_MSG_QUEUE_MODE enMode)
{
	static handle_t  queuedata;
	ADS_Dbg("%s  pucName=%s\t",__FUNCTION__,pucName);
	if(GXCORE_SUCCESS==GxCore_QueueCreate(&queuedata,uiMaxMsgLen,sizeof(ST_DSAD_MSG_QUEUE *)))
	{
		*uiMsgQueueHandle=(UINT32)queuedata;
		ADS_Dbg("success!size=%d depth=%d queue=%u	enMode=%d\n",
			sizeof(ST_DSAD_MSG_QUEUE *),uiMaxMsgLen,*uiMsgQueueHandle,enMode);
		return TRUE;
	}
	else
	{
		ADS_Dbg("failed !\n");
		return FALSE;
	}
}

BOOL DSAD_MsgQueueGetMsg
(UINT32 uiMsgHandle,
ST_DSAD_MSG_QUEUE* pstMsg,
EN_DSAD_MSG_QUEUE_MODE enMode,
UINT32 uiWaitMilliSecond)
{
	static uint32_t size_cpy;
	int ret;
	if(enMode==DSAD_MSG_QUEUE_NOWAIT)
		uiWaitMilliSecond=0;
	ret=GxCore_QueueGet(uiMsgHandle, pstMsg,sizeof(ST_DSAD_MSG_QUEUE *),&size_cpy,uiWaitMilliSecond);

	if(GXCORE_SUCCESS==ret && size_cpy !=0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL DSAD_MsgQueueSendMsg(UINT32 uiMsgHandle, ST_DSAD_MSG_QUEUE* pstMsg)
{
	if(GXCORE_SUCCESS == GxCore_QueuePut(uiMsgHandle,pstMsg,sizeof(ST_DSAD_MSG_QUEUE *),0))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

EN_DSAD_FUNCTION_TYPE DS_AD_SetFilter(ST_DSAD_FILTER_INFO *pstFilterInfo)
{
	ca_filter_t filter = {0};
	uint16_t			filterID;
	ca_filter_t* sfilter = NULL;
	uint8_t 	match[18] = {0,};
	uint8_t 	mask[18] = {0,};
	uint8_t    bRet = 0;
	//int i;

	if((pstFilterInfo == NULL)||(pstFilterInfo->enDataId >= DSAD_TOTAL_TYPE))
	{
		return DSAD_FUN_ERR_PARA;
	}
	//ADS_Dbg("%s>>pid=0x%x,dataId=%d\n",__FUNCTION__,pstFilterInfo->usChannelPid,pstFilterInfo->enDataId);

	for (filterID = 0; filterID< MAX_FILTER_COUNT; filterID++)
	{
		  sfilter = app_porting_ca_demux_get_by_filter_id(filterID,&filter);
		if (( 0 !=filter.usedStatus)&&(pstFilterInfo->usChannelPid == filter.pid)
			&&(0 !=filter.channelhandle)&&(pstFilterInfo->enDataId == filter.byReqID)
			&&(0!=filter.handle))
		{
			//return DSAD_FUN_OK;
			DS_AD_StopFilter(pstFilterInfo->enDataId,pstFilterInfo->usChannelPid);
			break;
		}
	}
#if 0
	for (i = 0; i< pstFilterInfo->ucFilterLen; i++)
	{
		if (0 == i)
		{
			match[i]	 = pstFilterInfo->aucFilter[i];
			mask[i] = pstFilterInfo->aucMask[i];
		}
		else
		{
			match[i+2]	 = pstFilterInfo->aucFilter[i];
			mask[i+2] = pstFilterInfo->aucMask[i];
		}
	}
	filter.filterLen = pstFilterInfo->ucFilterLen+2;
	if(filter.filterLen <= 3)
	{
		filter.filterLen = 4;
	}
#else
	memcpy(match, pstFilterInfo->aucFilter, pstFilterInfo->ucFilterLen);
	memcpy(mask, pstFilterInfo->aucMask, pstFilterInfo->ucFilterLen);
	memset(&filter,0,sizeof(ca_filter_t));
	filter.filterLen = pstFilterInfo->ucFilterLen;
#endif
	memcpy(filter.match,match,filter.filterLen);
	memcpy(filter.mask,mask,filter.filterLen);

	filter.byReqID = pstFilterInfo->enDataId;
	filter.crcFlag = TRUE;
	filter.nWaitSeconds = pstFilterInfo->ucWaitSeconds;
	filter.pid = pstFilterInfo->usChannelPid;
	filter.equalFlag = TRUE;
	filter.repeatFlag = TRUE;
	//filter.sw_filter_en = FALSE;
	filter.Callback = (FilterNotify)app_desai_ad_filter_notify;
	filter.timeOutCallback = app_desai_ad_filter_timeout_notify;
	gpfDataNotifyFunction[pstFilterInfo->enDataId] = pstFilterInfo->pfDataNotifyFunction;
	bRet = app_porting_ca_demux_start_filter(&filter);
	if(bRet)
	{
//		ADS_Dbg("OK\n");
		return DSAD_FUN_OK;
	}
	else
	{
		ADS_Dbg("%s.%d>>exit\n",__FUNCTION__,__LINE__);
		ADS_Dbg("FAILED\n");
		return DSAD_FUN_ERR_PARA;
	}
}


EN_DSAD_FUNCTION_TYPE DS_AD_StopFilter(EN_DSAD_DATA_ID enDataID, UINT16 usPID)
{
	uint16_t            filterID = 0;
	ca_filter_t sfilter = {0};
	if((enDataID != DSAD_DELECMD_DATA) && (enDataID != DSAD_CONFIG_DATA))
	{
		//ADS_Dbg("%s>>pid=0x%x,dataId=%d\n",__FUNCTION__,usPID,enDataID);
	}
	for (filterID = 0; filterID< MAX_FILTER_COUNT; filterID++)
	{
		app_porting_ca_demux_get_by_filter_id(filterID,&sfilter);
		if(!sfilter.usedStatus)
		{
			continue;
		}
		if(!sfilter.channelhandle || !sfilter.handle)
		{
			continue;
		}

		if(usPID == sfilter.pid && sfilter.byReqID == enDataID)
		{
			app_porting_ca_demux_release_filter( filterID, 1);
		}
	}
	return DSAD_FUN_OK;
}
void DSAD_ShowMessage(UINT8 ucMessageType, IN UINT8 * pucMessage)
{
	ST_DSAD_LOG_INFO * pLogoInfo = NULL;
	ST_DSAD_PICTURE_SHOW_INFO *pstPictureType=NULL;
	ST_DSAD_AV_SHOW_INFO *pstAvType = NULL;
	ST_DSAD_PROGRAM_SHOW_INFO * pstFullScreenInfo = NULL;

	ADS_Dbg("%s>>ucMessageType=%d,pucMessage=%p !\n",__FUNCTION__,ucMessageType,pucMessage);
	switch(ucMessageType)
	{
		case DSAD_MESSAGE_UPDATE_LOG_TYPE:
			pLogoInfo = (ST_DSAD_LOG_INFO *)pucMessage;
			if(!pLogoInfo)
			{
				break;
			}
			if(pLogoInfo->ucLogType == 1)
			{
				pstPictureType = (ST_DSAD_PICTURE_SHOW_INFO *)&pLogoInfo->Element;
				/*Picture data,JPEG,write to flash*/
				app_flash_save_logo_data(
						(char *)pstPictureType->pucPicData,
						pstPictureType->uiDataLen);
			}
			else if(pLogoInfo->ucLogType == 2)
			{
				pstAvType = (ST_DSAD_AV_SHOW_INFO *)&pLogoInfo->Element;
				/*I frame data,write to file in FS*/
				app_flash_save_ad_data_to_flash_file(
					(const char*)pLogoInfo,
					sizeof(ST_DSAD_LOG_INFO),
					DS_AD_LOGO_CONFIG_FILE);
				app_flash_save_ad_data_to_flash_file(
					(const char*)pstAvType->pucAvData,
					pstAvType->uiDataLen,
					DS_AD_LOGO_I_FRAME_FILE);
				break;
			}
			else
			{
				break;
			}
		break;
		case DSAD_MSEEAGE_SHOWFULLSRCEEN_TYPE:
		pstFullScreenInfo = (ST_DSAD_PROGRAM_SHOW_INFO *)pucMessage;
		if(pstFullScreenInfo == NULL)
		{
			break;
		}
		memset(&full_screen_pic_info,0,sizeof(ST_DSAD_PROGRAM_SHOW_INFO));
		memcpy(&full_screen_pic_info,pstFullScreenInfo,sizeof(ST_DSAD_PROGRAM_SHOW_INFO));
		full_screen_ad_need_show = TRUE;

		break;
		case DSAD_MSEEAGE_HIDEFULLSRCEEN_TYPE:
		full_screen_ad_need_show = FALSE;
		app_ds_ad_hide_full_screen_pic();
		break;
		default:
		break;
	}


}
/**/
