/* ********** ********** ********** ********** ********** ********** ********** ********** ********** **********
shbaek: Include File
********** ********** ********** ********** ********** ********** ********** ********** ********** ********** */

#include "grib_onem2m.h"

/* ********** ********** ********** ********** ********** ********** ********** ********** ********** **********
shbaek: Global Variable
********** ********** ********** ********** ********** ********** ********** ********** ********** ********** */
char gSiServerIp[ONEM2M_MAX_SIZE_IP_STR+1];
int  gSiServerPort;

int  gDebugOneM2M;

//3 shbaek: Do Not Used ...
OneM2M_ReqParam gReqParam;
OneM2M_ResParam gResParam;

/* ********** ********** ********** ********** ********** ********** ********** ********** ********** ********** */
#define __ONEM2M_UTIL_FUNC__
/* ********** ********** ********** ********** ********** ********** ********** ********** ********** ********** */
int Grib_SiSetServerConfig(void)
{
	int iRes = GRIB_ERROR;
	Grib_ConfigInfo* pConfigInfo = NULL;

	pConfigInfo = Grib_GetConfigInfo();
	if(pConfigInfo == NULL)
	{
		GRIB_LOGD("LOAD CONFIG ERROR !!!\n");
		return GRIB_ERROR;
	}

	STRINIT(gSiServerIp, sizeof(gSiServerIp));
	STRNCPY(gSiServerIp, pConfigInfo->platformServerIP, STRLEN(pConfigInfo->platformServerIP));

	gSiServerPort = pConfigInfo->platformServerPort;

	gDebugOneM2M = pConfigInfo->debugOneM2M;

	Grib_HttpSetDebug(gDebugOneM2M, pConfigInfo->tombStoneHTTP);
	Grib_SdaSetDebug(gDebugOneM2M);

	GRIB_LOGD("# SERVER CONFIG: %s:%d\n", gSiServerIp, gSiServerPort);

	return GRIB_SUCCESS;
}

int Grib_OneM2MResParser(OneM2M_ResParam *pResParam)
{
	int i = 0;
	int iRes = GRIB_ERROR;
	int iLoopMax = 128;
	int iDBG = FALSE;

	char* strToken		= NULL;
	char* str1Line		= NULL;
	char* strTemp		= NULL;

	char* strKey		= NULL;
	char* strValue		= NULL;
	
	char* strResponse	= NULL;

	if( (pResParam==NULL) || (pResParam->http_RecvData==NULL) )
	{
		GRIB_LOGD("# PARAM IS NULL\n");
		return GRIB_ERROR;
	}

	STRINIT(pResParam->xM2M_RsrcID, sizeof(pResParam->xM2M_RsrcID));
	STRINIT(pResParam->xM2M_PrntID, sizeof(pResParam->xM2M_PrntID));

	strToken = GRIB_CRLN;
	strResponse = STRDUP(pResParam->http_RecvData);
	if(strResponse == NULL)
	{
		GRIB_LOGD("# RESPONSE COPY ERROR\n");
		goto FINAL;
	}

	if(iDBG)GRIB_LOGD("===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n");
	do{
		//shbaek: Cut 1 Line
		if(i==0)
		{
			str1Line = STRTOK(strResponse, strToken);
		}
		else
		{
			str1Line = STRTOK(NULL, strToken);
		}

		i++;
		if(iDBG)GRIB_LOGD("[%03d]%s\n", i, str1Line);
		if(str1Line == NULL)
		{
			if(iDBG)GRIB_LOGD("END LINE: %d\n", i);
			break;
		}

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: [TBD] Find Only Resource ID Key
		strKey = "\"ri\" : \"";
		strTemp = STRSTR(str1Line, strKey);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strKey);
			if(iDBG)GRIB_LOGD("[%03d] KEY:[%s] TEMP VALUE:[%s]\n", i, strKey, strValue);

			strValueEnd = STRCHR(strValue, '"');
			strValueEnd[0] = NULL;

			STRNCPY(pResParam->xM2M_RsrcID, strValue, STRLEN(strValue));
			
			if(iDBG)GRIB_LOGD("[%03d] LAST VALUE:[%s]\n", i, pResParam->xM2M_RsrcID);
			continue;
		}
		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: [TBD] Find Only Parent ID Key
		strKey = "\"pi\" : \"";
		strTemp = STRSTR(str1Line, strKey);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strKey);
			if(iDBG)GRIB_LOGD("[%03d] KEY:[%s] TEMP VALUE:[%s]\n", i, strKey, strValue);

			strValueEnd = STRCHR(strValue, '"');
			strValueEnd[0] = NULL;

			STRNCPY(pResParam->xM2M_PrntID, strValue, STRLEN(strValue));
			
			if(iDBG)GRIB_LOGD("[%03d] LAST VALUE:[%s]\n", i, pResParam->xM2M_PrntID);
			break;//3 shbaek: Search More?
		}
		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####

	}while(i < iLoopMax);
	if(iDBG)GRIB_LOGD("===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n");

FINAL:
	if(strResponse!=NULL)FREE(strResponse);

	return GRIB_DONE;
}

int Grib_OneM2MSendMsg(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	Grib_HttpMsgInfo httpMsg;

	//shbaek: Check Server Info
	if( STRLEN(gSiServerIp)==0 || (gSiServerPort==0) )
	{
		Grib_SiSetServerConfig();
	}

	MEMSET(&httpMsg, 0x00, sizeof(Grib_HttpMsgInfo));
	MEMSET(pResParam, 0x00, sizeof(OneM2M_ResParam));


	httpMsg.serverIP = gSiServerIp;
	httpMsg.serverPort = gSiServerPort;

	httpMsg.LABEL	 = pReqParam->xM2M_Origin;
	httpMsg.recvBuff = pResParam->http_RecvData;

	if(pReqParam->xM2M_ResourceType == ONEM2M_RESOURCE_TYPE_SEMANTIC_DESCRIPTOR)
	{//shbaek: Too Large Data ...
		httpMsg.sendBuff = pReqParam->http_SendDataEx;
		if(gDebugOneM2M)GRIB_LOGD("# %s-xM2M-MSG: LARGE DATA: %d\n", pReqParam->xM2M_Origin, STRLEN(httpMsg.sendBuff));
	}
	else
	{
		httpMsg.sendBuff = pReqParam->http_SendData;
	}

	iRes = Grib_HttpSendMsg(&httpMsg);
	if(iRes <= 0)
	{
		GRIB_LOGD("# %s-xM2M-MSG: MSG SEND ERROR !!!\n", pReqParam->xM2M_Origin);
		return GRIB_ERROR;
	}

	iRes = Grib_HttpResParser(&httpMsg);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# %s-xM2M-MSG: MSG PARSE ERROR !!!\n", pReqParam->xM2M_Origin);
		return iRes;
	}

	pResParam->http_ResNum = httpMsg.statusCode;

	STRINIT(pResParam->http_ResMsg, STRLEN(pResParam->http_ResMsg));
	STRNCPY(pResParam->http_ResMsg, httpMsg.statusMsg, STRLEN(httpMsg.statusMsg));

	if(	(httpMsg.statusCode==HTTP_STATUS_CODE_OK) ||
		(httpMsg.statusCode==HTTP_STATUS_CODE_CREATED))
	{//shbaek: SUCCESS CASE
		return GRIB_DONE;
	}

	if(gDebugOneM2M)GRIB_LOGD("# %s-xM2M-MSG: %s [%d]\n", pReqParam->xM2M_Origin, httpMsg.statusMsg, httpMsg.statusCode);
	return GRIB_ERROR;
}

int Grib_GetAttrExpireTime(char* attrBuff, TimeInfo* pTime)
{
	const char* EXPIRE_TIME_STR_FORMAT = "%04d%02d%02dT%02d%02d%02d";

	time_t sysTimer;
	TimeInfo *sysTime;

	if(attrBuff == NULL)
	{
		GRIB_LOGD("# PARAM IS NULL ERROR !!!\n");
		return GRIB_ERROR;
	}

	sysTimer = time(NULL);
	sysTime  = localtime(&sysTimer);

	STRINIT(attrBuff, ONEM2M_EXPIRE_TIME_STR_SIZE);

	if(pTime == NULL)
	{
		STRNCPY(attrBuff, ONEM2M_FIX_ATTR_ET, STRLEN(ONEM2M_FIX_ATTR_ET));
	}
	else
	{
		SNPRINTF(attrBuff, ONEM2M_EXPIRE_TIME_STR_SIZE, EXPIRE_TIME_STR_FORMAT, 
					sysTime->tm_year+1900+pTime->tm_year, sysTime->tm_mon+1+pTime->tm_mon, sysTime->tm_mday+pTime->tm_mday,
					sysTime->tm_hour+pTime->tm_hour, sysTime->tm_min+pTime->tm_min, sysTime->tm_sec+pTime->tm_sec);
	}

	return GRIB_DONE;
}

int Grib_isAvailableExpireTime(char* xM2M_ExpireTime)
{
	int i = 0;
	time_t sysTimer;

	TimeInfo *sysTime;
	TimeInfo *expireTime;
	
	int isAvailable = FALSE;
	int iSeek = 0;
	char timeBuff[5] = {'\0', };

	//shbaek: Ex)20991130T163430
	if( (xM2M_ExpireTime==NULL) || (STRLEN(xM2M_ExpireTime)<15) )
	{
		GRIB_LOGD("# EXPIRE TIME INVALID ERROR !!!\n");
		return FALSE;
	}

	sysTimer = time(NULL);
	sysTime  = localtime(&sysTimer);

	if(gDebugOneM2M)GRIB_LOGD("# EXPIRE TIME: %s\n", xM2M_ExpireTime);
	
	//shbaek: YYYY
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<4; i++)timeBuff[i] = xM2M_ExpireTime[i];
	if((sysTime->tm_year+1900) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_year+1900))return FALSE;
	else iSeek = i;
	
	//shbaek: MM
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<2; i++)timeBuff[i] = xM2M_ExpireTime[iSeek+i];
	if((sysTime->tm_mon+1) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_mon+1))return FALSE;
	else iSeek += i;

	//shbaek: DD
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<2; i++)timeBuff[i] = xM2M_ExpireTime[iSeek+i];
	if((sysTime->tm_mday) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_mday))return FALSE;
	else iSeek += i;

	//shbaek: Skip 'T'
	iSeek++;

	//shbaek: HH
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<2; i++)timeBuff[i] = xM2M_ExpireTime[iSeek+i];
	if((sysTime->tm_hour) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_hour))return FALSE;
	else iSeek += i;

	//shbaek: MM
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<2; i++)timeBuff[i] = xM2M_ExpireTime[iSeek+i];
	if((sysTime->tm_min) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_min))return FALSE;
	else iSeek += i;
	iSeek += i;

	//shbaek: SS
	STRINIT(timeBuff, sizeof(timeBuff));
	for(i=0; i<2; i++)timeBuff[i] = xM2M_ExpireTime[iSeek+i];
	if((sysTime->tm_sec) < ATOI(timeBuff))return TRUE;
	else if(ATOI(timeBuff) < (sysTime->tm_sec))return FALSE;
	else iSeek += i;
	iSeek += i;

	return TRUE;
}



#define __ONEM2M_DEVICE_ID_FUNC__
//2 shbaek: NEED: xM2M_NM
int Grib_AppEntityCreate(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};
	char  httpBody[HTTP_MAX_SIZE_BODY] = {'\0',};

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  xM2M_AttrAPN[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  xM2M_AttrAPI[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRR = ONEM2M_FIX_ATTR_RR;

	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRN = NULL;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_Origin, sizeof(pReqParam->xM2M_Origin));
	STRNCPY(pReqParam->xM2M_Origin, pReqParam->xM2M_NM, STRLEN(pReqParam->xM2M_NM));
	pReqParam->xM2M_Origin[STRLEN(pReqParam->xM2M_NM)+1] = NULL;

	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_Label", pReqParam->xM2M_Origin);
	SNPRINTF(xM2M_AttrAPN, sizeof(xM2M_AttrAPN), "%s_AppName",	  pReqParam->xM2M_Origin);
	SNPRINTF(xM2M_AttrAPI, sizeof(xM2M_AttrAPI), "%s_AppID",     pReqParam->xM2M_Origin);

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqAppEntityCreate", pReqParam->xM2M_Origin);

	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);
	xM2M_AttrRN = (char *)pReqParam->xM2M_NM; //shbaek: "X-M2M-NM" Change to "rn" on v2.0

	SNPRINTF(httpBody, sizeof(httpBody), ONEM2M_BODY_FORMAT_APP_ENTITY_CREATE,
				xM2M_AttrLBL, xM2M_AttrAPN, xM2M_AttrAPI, gSiServerIp, gSiServerPort, xM2M_AttrRR,
				xM2M_AttrRN, xM2M_AttrET);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_APP_ENTITY_CREATE,
				gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_APP_ENTITY, STRLEN(httpBody), 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s%s", httpHead, httpBody);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY CREATE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum == HTTP_STATUS_CODE_CONFLICT)GRIB_LOGD("# %s: APP ENTITY ALREADY EXIST ...\n", pReqParam->xM2M_Origin);
		else GRIB_LOGD("# %s: APP ENTITY CREATE ERROR\n", pReqParam->xM2M_Origin);
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY CREATE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}
	
	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}
	
	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_Origin
int Grib_AppEntityRetrieve(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;

	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqAppEntityRetrieve", pReqParam->xM2M_Origin);

	STRINIT(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI));
	SNPRINTF(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI), "%s", pReqParam->xM2M_Origin);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_APP_ENTITY_RETRIEVE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY RETRIEVE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# APP ENTITY RETRIEVE ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY RETRIEVE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}
	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}

	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_Origin
int Grib_AppEntityDelete(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;

	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqAppEntityDelete", pReqParam->xM2M_Origin);

	STRINIT(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI));
	SNPRINTF(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI), "%s", pReqParam->xM2M_Origin);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_APP_ENTITY_DELETE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY DELETE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# APP ENTITY DELETE ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# APP ENTITY DELETE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	return GRIB_SUCCESS;
}

#define __ONEM2M_CONTAINER_FUNC__

//2 shbaek: NEED: xM2M_URI, xM2M_Origin, xM2M_NM
int Grib_ContainerCreate(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};
	char  httpBody[HTTP_MAX_SIZE_BODY] = {'\0',};

	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	unsigned long long int xM2M_AttrMNI = 0; //shbaek: maxNrOfInstances
	unsigned long long int xM2M_AttrMBS = 0; //shbaek: maxByteSize
	unsigned long long int xM2M_AttrMIA = 0; //shbaek: maxInstanceAge

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_URI] = {'\0',};
	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRN = NULL;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	xM2M_AttrMNI = ONEM2M_FIX_ATTR_MNI;
	xM2M_AttrMBS = ONEM2M_FIX_ATTR_MBS;
	xM2M_AttrMIA = ONEM2M_FIX_ATTR_MIA;

	STRNCPY(xM2M_AttrLBL, pReqParam->xM2M_URI, STRLEN(pReqParam->xM2M_URI));
	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_Label", pReqParam->xM2M_NM);

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqContainerCreate", pReqParam->xM2M_Origin);

	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);
	xM2M_AttrRN = (char *)pReqParam->xM2M_NM; //shbaek: "X-M2M-NM" Change to "rn" on v2.0

	SNPRINTF(httpBody, sizeof(httpBody), ONEM2M_BODY_FORMAT_CONTAINER_CREATE,
				xM2M_AttrLBL, xM2M_AttrMNI, xM2M_AttrMBS, xM2M_AttrMIA,
				xM2M_AttrRN, xM2M_AttrET);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTAINER_CREATE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_CONTAINER, STRLEN(httpBody), 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s%s", httpHead, httpBody);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER CREATE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum == HTTP_STATUS_CODE_CONFLICT)
		{
			GRIB_LOGD("# %s: %s CONTAINER ALREADY EXIST ...\n", pReqParam->xM2M_Origin, pReqParam->xM2M_NM);
		}
		else GRIB_LOGD("# %s: %s CONTAINER CREATE ERROR\n", pReqParam->xM2M_Origin, pReqParam->xM2M_NM);

		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER CREATE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
	}

	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_URI, xM2M_Origin
int Grib_ContainerRetrieve(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;

	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqContainerRetrieve", pReqParam->xM2M_Origin);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTAINER_RETRIEVE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER RETRIEVE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# CONTAINER RETRIEVE ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER RETRIEVE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
	}

	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_URI, xM2M_Origin
int Grib_ContainerDelete(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;

	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqContainerDelete", pReqParam->xM2M_Origin);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTAINER_DELETE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER DELETE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# CONTAINER DELETE ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTAINER DELETE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	return GRIB_SUCCESS;
}

#define __ONEM2M_POLLING_FUNC__
//2 shbaek: NEED: xM2M_Origin
int Grib_PollingChannelCreate(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};
	char  httpBody[HTTP_MAX_SIZE_BODY] = {'\0',};

	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_BRIEF] = {NULL,};
	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRN = NULL;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqPollingChannelCreate", pReqParam->xM2M_Origin);

	STRINIT(pReqParam->xM2M_NM, sizeof(pReqParam->xM2M_NM));
	STRNCPY(pReqParam->xM2M_NM, ONEM2M_URI_CONTENT_POLLING_CHANNEL, STRLEN(ONEM2M_URI_CONTENT_POLLING_CHANNEL));

	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_Label", pReqParam->xM2M_NM);

	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);
	xM2M_AttrRN = (char *)pReqParam->xM2M_NM; //shbaek: "X-M2M-NM" Change to "rn" on v2.0

	SNPRINTF(httpBody, sizeof(httpBody), ONEM2M_BODY_FORMAT_POLLING_CHANNEL_CREATE, xM2M_AttrLBL,
			xM2M_AttrRN, xM2M_AttrET);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTAINER_CREATE,
				pReqParam->xM2M_Origin, gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_POLLING_CHANNEL, STRLEN(httpBody),
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s%s", httpHead, httpBody);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# POLLING CHANNEL CREATE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum == HTTP_STATUS_CODE_CONFLICT)GRIB_LOGD("# %s: POLLING CHANNEL ALREADY EXIST ...\n", pReqParam->xM2M_Origin);
		else GRIB_LOGD("# %s: POLLING CHANNEL CREATE ERROR\n", pReqParam->xM2M_Origin);
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# POLLING CHANNEL CREATE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}

	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_Origin xM2M_Func
int Grib_SubsciptionCreate(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};
	char  httpBody[HTTP_MAX_SIZE_BODY] = {'\0',};

	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  xM2M_AttrENC[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  xM2M_AttrNU[ONEM2M_MAX_SIZE_URI] = {'\0',};

	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRN = NULL;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqSubsciptionCreate", pReqParam->xM2M_Origin);

	STRINIT(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI));
	SNPRINTF(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI), "%s/%s/%s", 
		pReqParam->xM2M_Origin, pReqParam->xM2M_Func, ONEM2M_URI_CONTENT_EXECUTE);

	STRINIT(pReqParam->xM2M_NM, sizeof(pReqParam->xM2M_NM));
	STRNCPY(pReqParam->xM2M_NM, ONEM2M_URI_CONTENT_SUBSCRIPTION, STRLEN(ONEM2M_URI_CONTENT_SUBSCRIPTION));

	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_Label", pReqParam->xM2M_NM);
	STRNCPY(xM2M_AttrENC, ONEM2M_FIX_ATTR_ENC, STRLEN(ONEM2M_FIX_ATTR_ENC));
	
	SNPRINTF(xM2M_AttrNU, sizeof(xM2M_AttrNU), ONEM2M_FIX_ATTR_NU_FORMAT,
		gSiServerIp, gSiServerPort, pReqParam->xM2M_Origin);

	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);
	xM2M_AttrRN = (char *)pReqParam->xM2M_NM; //shbaek: "X-M2M-NM" Change to "rn" on v2.0

	SNPRINTF(httpBody, sizeof(httpBody), ONEM2M_BODY_FORMAT_SUBSCRIPTION_CREATE, 
		xM2M_AttrLBL, xM2M_AttrENC, xM2M_AttrNU, xM2M_AttrRN, xM2M_AttrET);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_SUBSCRIPTION_CREATE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_SUBSCRIPTION, STRLEN(httpBody), 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s%s", httpHead, httpBody);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# SUBSCRIPTION CREATE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum == HTTP_STATUS_CODE_CONFLICT)GRIB_LOGD("# %s: SUBSCRIPTION ALREADY EXIST ...\n", pReqParam->xM2M_Origin);
		else GRIB_LOGD("# %s: SUBSCRIPTION CREATE ERROR\n", pReqParam->xM2M_Origin);
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# SUBSCRIPTION CREATE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}
	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}
	
	return GRIB_SUCCESS;
}

#define __ONEM2M_CONTENT_INSTANCE_FUNC__

//2 shbaek: NEED: xM2M_URI, xM2M_Origin, xM2M_CNF[If NULL, Set Default "text/plain:0"], xM2M_CON
int Grib_ContentInstanceCreate(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};
	char  httpBody[HTTP_MAX_SIZE_BODY] = {'\0',};

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_BRIEF] = {NULL,};
	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};

	TimeInfo pTime;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqContentInstanceCreate", pReqParam->xM2M_Origin);

	if(STRLEN(pReqParam->xM2M_CNF) <= 1)
	{//shbaek: Default Type
		STRINIT(pReqParam->xM2M_CNF, sizeof(pReqParam->xM2M_CNF));
		SNPRINTF(pReqParam->xM2M_CNF, sizeof(pReqParam->xM2M_CNF), "%s:0", HTTP_CONTENT_TYPE_TEXT);
	}

	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_ReportLabel", pReqParam->xM2M_Origin);

#if __NOT_USED__
	MEMSET(&pTime, 0x00, sizeof(TimeInfo));
	pTime.tm_year = 1;
//	pTime.tm_mon  = 1;
//	pTime.tm_mday = 1;
//	pTime.tm_hour = 1;
//	pTime.tm_min  = 1;
//	pTime.tm_sec  = 1;

	Grib_GetAttrExpireTime(xM2M_AttrET, &pTime);
#endif
	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);

	SNPRINTF(httpBody, sizeof(httpBody), ONEM2M_BODY_FORMAT_CONTENT_INSTANCE_CREATE,
				xM2M_AttrLBL, xM2M_AttrET, pReqParam->xM2M_CNF, pReqParam->xM2M_CON);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTENT_INSTANCE_CREATE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_CONTENT_INSTANCE, STRLEN(httpBody), 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s%s", httpHead, httpBody);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTENT INSTANCE CREATE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# %s: INSTANCE CREATE ERROR: %s [%d]\n", pReqParam->xM2M_Origin, pResParam->http_ResMsg, pResParam->http_ResNum);
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTENT INSTANCE CREATE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}

	return GRIB_SUCCESS;
}

//2 shbaek: NEED: xM2M_URI, xM2M_Origin
int Grib_ContentInstanceRetrieve(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;
	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqContentInstanceRetrieve", pReqParam->xM2M_Origin);

#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_CONTENT_INSTANCE_RETRIEVE,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTENT INST RETRIEVE SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# CONTENT INST RETRIEVE ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# CONTENT INST RETRIEVE RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}

	return GRIB_SUCCESS;
}

#define __ONEM2M_LONG_POLLING_FUNC__

int Grib_LongPollingResParser(OneM2M_ResParam *pResParam)
{
	int i = 0;
	int iRes = GRIB_ERROR;
	int iLoopMax = 128;
	int iDBG = FALSE;

	char* strToken		= NULL;
	char* str1Line		= NULL;
	char* strTemp		= NULL;

	char* strTagStart	= NULL;
	char* strTagEnd	= NULL;
	char* strValue		= NULL;

	char* strResponse	= NULL;

	if( (pResParam==NULL) || (pResParam->http_RecvData==NULL) )
	{
		GRIB_LOGD("# PARAM IS NULL\n");
		return GRIB_ERROR;
	}

	strToken = GRIB_CRLN;
	strResponse = STRDUP(pResParam->http_RecvData);
	if(strResponse == NULL)
	{
		GRIB_LOGD("# RESPONSE COPY ERROR\n");
		goto FINAL;
	}

	if(iDBG)GRIB_LOGD("===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n");
	do{
		//shbaek: Cut 1 Line
		if(i==0)
		{
			str1Line = STRTOK(strResponse, strToken);
		}
		else
		{
			str1Line = STRTOK(NULL, strToken);
		}

		i++;
		if(iDBG)GRIB_LOGD("[%03d]%s\n", i, str1Line);
		if(str1Line == NULL)
		{
			if(iDBG)GRIB_LOGD("END LINE: %d\n", i);
			break;
		}

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: Find Resource ID Tag
		strTagStart = "<ri>";
		strTagEnd	= "</ri>";
		strTemp = STRSTR(str1Line, strTagStart);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strTagStart);
			if(iDBG)GRIB_LOGD("[%03d] TAG:[%s] TEMP VALUE:[%s]\n", i, strTagStart, strValue);

			strValueEnd = STRSTR(str1Line, strTagEnd);
			strValueEnd[0] = NULL;
			
			STRINIT(pResParam->xM2M_RsrcID, sizeof(pResParam->xM2M_RsrcID));
			STRNCPY(pResParam->xM2M_RsrcID, strValue, STRLEN(strValue));
			
			if(iDBG)GRIB_LOGD("[%03d] RID:[%s]\n", i, pResParam->xM2M_RsrcID);
			continue;//3 shbaek: Next Line
		}

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: Find Parents ID Tag
		strTagStart = "<pi>";
		strTagEnd	= "</pi>";
		strTemp = STRSTR(str1Line, strTagStart);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strTagStart);
			if(iDBG)GRIB_LOGD("[%03d] TAG:[%s] TEMP VALUE:[%s]\n", i, strTagStart, strValue);

			strValueEnd = STRSTR(str1Line, strTagEnd);
			strValueEnd[0] = NULL;

			STRINIT(pResParam->xM2M_PrntID, sizeof(pResParam->xM2M_PrntID));
			STRNCPY(pResParam->xM2M_PrntID, strValue, STRLEN(strValue));

			if(iDBG)GRIB_LOGD("[%03d] PID:[%s]\n", i, pResParam->xM2M_PrntID);
			continue;//3 shbaek: Next Line
		}

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: Find Expire Time Tag
		strTagStart = "<et>";
		strTagEnd	= "</et>";
		strTemp = STRSTR(str1Line, strTagStart);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strTagStart);
			if(iDBG)GRIB_LOGD("[%03d] TAG:[%s] TEMP VALUE:[%s]\n", i, strTagStart, strValue);

			strValueEnd = STRSTR(str1Line, strTagEnd);
			strValueEnd[0] = NULL;

			STRINIT(pResParam->xM2M_ExpireTime, sizeof(pResParam->xM2M_ExpireTime));
			STRNCPY(pResParam->xM2M_ExpireTime, strValue, STRLEN(strValue));

			if(iDBG)GRIB_LOGD("[%03d] ET:[%s]\n", i, pResParam->xM2M_ExpireTime);
			continue;//3 shbaek: Next Line
		}

		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
		//shbaek: Find Content Tag
		strTagStart = "<con>";
		strTagEnd	= "</con>";
		strTemp = STRSTR(str1Line, strTagStart);
		if(strTemp != NULL)
		{
			char *strValueEnd = NULL;

			//shbaek: Copy Value
			strValue = strTemp+STRLEN(strTagStart);
			if(iDBG)GRIB_LOGD("[%03d] TAG:[%s] TEMP VALUE:[%s]\n", i, strTagStart, strValue);

			strValueEnd = STRSTR(str1Line, strTagEnd);
			strValueEnd[0] = NULL;

			STRINIT(pResParam->xM2M_Content, sizeof(pResParam->xM2M_Content));
			STRNCPY(pResParam->xM2M_Content, strValue, STRLEN(strValue));

			if(iDBG)GRIB_LOGD("[%03d] CON:[%s]\n", i, pResParam->xM2M_Content);
			break;//3 shbaek: Search More?
		}
		//shbaek: ##### ##### ##### ##### ##### ##### ##### ##### ##### #####

	}while(i < iLoopMax);
	if(iDBG)GRIB_LOGD("===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n");

FINAL:
	if(strResponse!=NULL)FREE(strResponse);

	return GRIB_DONE;
}

//2 shbaek: NEED: xM2M_Origin, [xM2M_URI: If NULL, Auto Set]
int Grib_LongPolling(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int iRes = GRIB_ERROR;
	int iDBG = gDebugOneM2M;
	char httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqLongPolling", pReqParam->xM2M_Origin);

	if(STRLEN(pReqParam->xM2M_URI) < STRLEN(pReqParam->xM2M_Origin))
	{
		STRINIT(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI));
		SNPRINTF(pReqParam->xM2M_URI, sizeof(pReqParam->xM2M_URI), "%s/%s/%s", 
			pReqParam->xM2M_Origin, ONEM2M_URI_CONTENT_POLLING_CHANNEL, ONEM2M_URI_CONTENT_PCU);

		GRIB_LOGD("# LONG POLLING URI: %s\n", pReqParam->xM2M_URI);
	}


#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);

	if(iDBG)
	{
		GRIB_LOGD("# LONG POLLING BASE64 SRC: %s\n", pAuthBase64Src);
		GRIB_LOGD("# LONG POLLING BASE64 ENC: %s\n", pAuthBase64Enc);
	}
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_LONG_POLLING,
				pReqParam->xM2M_URI, gSiServerIp, gSiServerPort, 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	STRINIT(pReqParam->http_SendData, sizeof(pReqParam->http_SendData));
	SNPRINTF(pReqParam->http_SendData, sizeof(pReqParam->http_SendData), "%s", httpHead);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# LONG POLLING SEND[%d]:\n%s", STRLEN(pReqParam->http_SendData), pReqParam->http_SendData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum != HTTP_STATUS_CODE_REQUEST_TIME_OUT)GRIB_LOGD("# LONG POLLING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# LONG POLLING RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_LongPollingResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}
	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
		GRIB_LOGD("# CONTENT    : [%s]\n", pResParam->xM2M_Content);
	}
	return GRIB_SUCCESS;
}

#define __ONEM2M_SEMANTIC_FUNC__
//2 shbaek: NEED: xM2M_Origin
int Grib_SemanticDescriptorUpload(OneM2M_ReqParam *pReqParam, OneM2M_ResParam *pResParam)
{
	int   iRes = GRIB_ERROR;
	int   iDBG = gDebugOneM2M;

	char  httpHead[HTTP_MAX_SIZE_HEAD] = {'\0',};

	char  xM2M_AttrLBL[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  xM2M_AttrDCRP[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};

	char  xM2M_AttrET[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char* xM2M_AttrRN = NULL;

//	char  smdBuff[SDA_MAX_DEVICE_INFO] = {'\0',};
//	char  xM2M_AttrDSP[SDA_MAX_DEVICE_INFO] = {'\0',};
//	char  httpBody[HTTP_MAX_SIZE_BODY+SDA_MAX_DEVICE_INFO] = {'\0',};

	char* smdBuff = NULL;
	char* xM2M_AttrDSP = NULL;
	char* httpBody = NULL;
	char* httpSendMsg = NULL;
	int   httpSendMsgByte = 0;

#ifdef FEATURE_CAS
	char  pAuthBase64Src[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
	char  pAuthBase64Enc[ONEM2M_MAX_SIZE_BRIEF] = {'\0',};
#endif

	//shbaek: Semantic Descriptor
	smdBuff 	 = (char *)MALLOC(SDA_MAX_DEVICE_INFO);
	xM2M_AttrDSP = (char *)MALLOC(SDA_MAX_DEVICE_INFO);

	MEMSET(smdBuff, 0x00, SDA_MAX_DEVICE_INFO);
	MEMSET(xM2M_AttrDSP, 0x00, SDA_MAX_DEVICE_INFO);

	iRes = Grib_SdaGetDeviceInfo(pReqParam->xM2M_Origin, smdBuff);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# %s: GET DEVICE INFO ERROR !!!\n", pReqParam->xM2M_Origin);
		return GRIB_ERROR;
	}
	
	Grib_Base64Encode(smdBuff, xM2M_AttrDSP, GRIB_NOT_USED);
	if(smdBuff)FREE(smdBuff);

	pReqParam->xM2M_ResourceType = ONEM2M_RESOURCE_TYPE_SEMANTIC_DESCRIPTOR;

	xM2M_AttrRN = ONEM2M_URI_CONTENT_SEM_DEC;
	if(iDBG)GRIB_LOGD("# RNAME: %s\n", xM2M_AttrRN);

	SNPRINTF(xM2M_AttrLBL, sizeof(xM2M_AttrLBL), "%s_SemanticDescriptorLabel", pReqParam->xM2M_Origin);
	if(iDBG)GRIB_LOGD("# LABEL: %s\n", xM2M_AttrLBL);

	SNPRINTF(xM2M_AttrDCRP, sizeof(xM2M_AttrDCRP), "%s:1",	  HTTP_CONTENT_TYPE_RDF_XML);
	if(iDBG)GRIB_LOGD("# DCRP: %s\n", xM2M_AttrDCRP);

	STRINIT(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID));
	SNPRINTF(pReqParam->xM2M_ReqID, sizeof(pReqParam->xM2M_ReqID), "%s_ReqSemanticDescriptorUpload", pReqParam->xM2M_Origin);
	if(iDBG)GRIB_LOGD("# REQ ID: %s\n", pReqParam->xM2M_ReqID);

	Grib_GetAttrExpireTime(xM2M_AttrET, GRIB_NOT_USED);
	if(iDBG)GRIB_LOGD("# Ex TIME: %s\n", xM2M_AttrET);

	//shbaek: HTTP Body
	httpBody = (char *)MALLOC(HTTP_MAX_SIZE_BODY+STRLEN(xM2M_AttrDSP)+1);
	MEMSET(httpBody, 0x00, HTTP_MAX_SIZE_BODY+STRLEN(xM2M_AttrDSP)+1);
	SNPRINTF(httpBody, HTTP_MAX_SIZE_BODY+STRLEN(xM2M_AttrDSP)+1, ONEM2M_BODY_FORMAT_SEMANTIC_DESCRIPTOR_UPLOAD,
				xM2M_AttrLBL, xM2M_AttrRN, xM2M_AttrET, xM2M_AttrDCRP, xM2M_AttrDSP);
	if(xM2M_AttrDSP)FREE(xM2M_AttrDSP);


#ifdef FEATURE_CAS
	SNPRINTF(pAuthBase64Src, sizeof(pAuthBase64Src), "%s:%s", pReqParam->xM2M_Origin, pReqParam->authKey);
	Grib_Base64Encode(pAuthBase64Src, pAuthBase64Enc, GRIB_NOT_USED);
#endif

	SNPRINTF(httpHead, sizeof(httpHead), ONEM2M_HEAD_FORMAT_SEMANTIC_DESCRIPTOR_UPLOAD,
				pReqParam->xM2M_Origin, gSiServerIp, gSiServerPort, ONEM2M_RESOURCE_TYPE_SEMANTIC_DESCRIPTOR, STRLEN(httpBody), 
#ifdef FEATURE_CAS
				pAuthBase64Enc,
#endif
				pReqParam->xM2M_Origin, pReqParam->xM2M_ReqID);

	//shbaek: Send Message 
	httpSendMsgByte = STRLEN(httpHead)+STRLEN(httpBody)+1;
	httpSendMsg		= (char*)MALLOC(httpSendMsgByte);
	STRINIT(httpSendMsg, httpSendMsgByte);
	SNPRINTF(httpSendMsg, httpSendMsgByte, "%s%s", httpHead, httpBody);

	if(httpBody)FREE(httpBody);
	pReqParam->http_SendDataEx = httpSendMsg;

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# SEMANTIC DESCRIPTOR SEND[%d]:\n%s", STRLEN(pReqParam->http_SendDataEx), pReqParam->http_SendDataEx);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MSendMsg(pReqParam, pResParam);
	if(iRes != GRIB_DONE)
	{
		if(pResParam->http_ResNum == HTTP_STATUS_CODE_CONFLICT)GRIB_LOGD("# %s: SEMANTIC DESCRIPTOR ALREADY EXIST ...\n", pReqParam->xM2M_Origin);
		else GRIB_LOGD("# %s: SEMANTIC DESCRIPTOR UPLOAD ERROR\n", pReqParam->xM2M_Origin);
		if(httpSendMsg)FREE(httpSendMsg);
		return GRIB_ERROR;
	}
	if(httpSendMsg)FREE(httpSendMsg);

	if(iDBG)
	{
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
		GRIB_LOGD("# SEMANTIC DESCRIPTOR RECV[%d]:\n%s\n", STRLEN(pResParam->http_RecvData), pResParam->http_RecvData);
		GRIB_LOGD(ONEM2M_1LINE_SEPERATOR);
	}

	iRes = Grib_OneM2MResParser(pResParam);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# RESPONSE PARSING ERROR\n");
		return GRIB_ERROR;
	}

	if(iDBG)
	{
		GRIB_LOGD("# RESOURCE ID: [%s]\n", pResParam->xM2M_RsrcID);
		GRIB_LOGD("# PARENTS  ID: [%s]\n", pResParam->xM2M_PrntID);
	}

	return iRes;
}


#define __ONEM2M_ETC_FUNC__

#ifdef FEATURE_CAS
int Grib_CreateOneM2MTree(Grib_DbRowDeviceInfo* pRowDeviceInfo, char* pAuthKey)
#else
int Grib_CreateOneM2MTree(Grib_DbRowDeviceInfo* pRowDeviceInfo)
#endif
{
	int i		= 0;
	int iRes	= GRIB_ERROR;
	int iDBG   	= gDebugOneM2M;

	OneM2M_ReqParam reqParam;
	OneM2M_ResParam resParam;

	if( (pRowDeviceInfo==NULL) || (pRowDeviceInfo->deviceID==NULL))
	{
		GRIB_LOGD("# PARAMETER IS NULL\n");
		return GRIB_ERROR;
	}

	GRIB_LOGD("# CREATE-TREE: %s\n", pRowDeviceInfo->deviceID);

	MEMSET(&reqParam, GRIB_INIT, sizeof(OneM2M_ReqParam));
	MEMSET(&resParam, GRIB_INIT, sizeof(OneM2M_ResParam));

#ifdef FEATURE_CAS
	reqParam.authKey = pAuthKey;
#endif

	//1 shbaek: 1.App Entity
	//shbaek: Your Device ID
	STRINIT(&reqParam.xM2M_NM, sizeof(reqParam.xM2M_NM));
	STRNCPY(&reqParam.xM2M_NM, pRowDeviceInfo->deviceID, STRLEN(pRowDeviceInfo->deviceID));
	//shbaek: Create Device ID Container
	iRes = Grib_AppEntityCreate(&reqParam, &resParam);
	if(iRes == GRIB_ERROR)
	{
		if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
		{//shbaek: Already Exist is Not Error.
			// TODO: shbaek: What Can I Do ... ?
		}
		else
		{
			goto ERROR;
		}
	}

	//1 shbaek: 2-1.Polling Channel
	MEMSET(&resParam, 0x00, sizeof(resParam));
	//shbaek: Create Polling Channel Container(need xM2M_Origin)
	iRes = Grib_PollingChannelCreate(&reqParam, &resParam);
	if(iRes == GRIB_ERROR)
	{
		if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
		{//shbaek: Already Exist is Not Error.
			// TODO: shbaek: What Can I Do ... ?
		}
		else
		{
			goto ERROR;
		}
	}

	for(i=0; i<pRowDeviceInfo->deviceFuncCount; i++)
	{
		Grib_DbRowDeviceFunc* pRowDeviceFunc = pRowDeviceInfo->ppRowDeviceFunc[i];
		char* pFuncName = pRowDeviceFunc->funcName;

		//1 shbaek: 2-2.Function
		MEMSET(&resParam, 0x00, sizeof(resParam));
		STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
		STRNCPY(&reqParam.xM2M_URI, reqParam.xM2M_Origin, STRLEN(reqParam.xM2M_Origin));
		STRINIT(&reqParam.xM2M_NM, sizeof(reqParam.xM2M_NM));
		STRNCPY(&reqParam.xM2M_NM, pFuncName, STRLEN(pFuncName));
		iRes = Grib_ContainerCreate(&reqParam, &resParam);
		if(iRes == GRIB_ERROR)
		{
			if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
			{//shbaek: Already Exist is Not Error.
				STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
				SNPRINTF(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s", reqParam.xM2M_Origin, pFuncName);
				Grib_ContainerRetrieve(&reqParam, &resParam);
			}
			else
			{
				goto ERROR;
			}
		}

		//1 shbaek: 3-1.Execute
		//shbaek: Set URI -> in/cse/"Device ID"/"Func"
		MEMSET(&resParam, 0x00, sizeof(resParam));
		STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
		SNPRINTF(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s", reqParam.xM2M_Origin, pFuncName);
		STRINIT(&reqParam.xM2M_NM, sizeof(reqParam.xM2M_NM));
		STRNCPY(&reqParam.xM2M_NM, ONEM2M_URI_CONTENT_EXECUTE, STRLEN(ONEM2M_URI_CONTENT_EXECUTE));
		//shbaek: Create Execute Container
		iRes = Grib_ContainerCreate(&reqParam, &resParam);
		if(iRes == GRIB_ERROR)
		{
			if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
			{//shbaek: Already Exist is Not Error.
				MEMSET(&resParam, 0x00, sizeof(resParam));
				STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
				SNPRINTF(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s/%s", reqParam.xM2M_Origin, pFuncName, reqParam.xM2M_NM);
				Grib_ContainerRetrieve(&reqParam, &resParam);
			}
			else
			{
				goto ERROR;
			}
		}

		//2 shbaek: NEED EXECUTE's RESOURCE ID
		STRINIT(pRowDeviceFunc->exRsrcID, sizeof(pRowDeviceFunc->exRsrcID));
		STRNCPY(pRowDeviceFunc->exRsrcID, resParam.xM2M_RsrcID, STRLEN(resParam.xM2M_RsrcID));
		if(iDBG)GRIB_LOGD("# %s: %s EXECUTE RESOURCE ID: %s\n", pRowDeviceInfo->deviceID, pFuncName, pRowDeviceFunc->exRsrcID);

		//1 shbaek: 3-2.Status
		//shbaek: Set URI -> in/cse/"Device ID"/"Func"
		MEMSET(&resParam, 0x00, sizeof(resParam));
		STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
		SNPRINTF(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s", reqParam.xM2M_Origin, pFuncName);
		STRINIT(&reqParam.xM2M_NM, sizeof(reqParam.xM2M_NM));
		STRNCPY(&reqParam.xM2M_NM, ONEM2M_URI_CONTENT_STATUS, STRLEN(ONEM2M_URI_CONTENT_STATUS));
		//shbaek: Create Status Container
		iRes = Grib_ContainerCreate(&reqParam, &resParam);
		if(iRes == GRIB_ERROR)
		{
			if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
			{//shbaek: Already Exist is Not Error.
				STRINIT(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
				SNPRINTF(&reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s/%s", reqParam.xM2M_Origin, pFuncName, reqParam.xM2M_NM);
				Grib_ContainerRetrieve(&reqParam, &resParam);
			}
			else
			{
				goto ERROR;
			}
		}

		//1 shbaek: 3-3.Subscription
		MEMSET(&resParam, 0x00, sizeof(resParam));
		STRINIT(&reqParam.xM2M_Func, sizeof(reqParam.xM2M_Func));
		STRNCPY(&reqParam.xM2M_Func, pFuncName, STRLEN(pFuncName));
		//shbaek: Create Subscription(need xM2M_Origin, xM2M_Func)
		iRes = Grib_SubsciptionCreate(&reqParam, &resParam);
		if(iRes == GRIB_ERROR)
		{
			if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
			{//shbaek: Already Exist is Not Error.
				// TODO: shbaek: What Can I Do ... ?
			}
			else
			{
				goto ERROR;
			}
		}

	}

	if(pRowDeviceInfo->deviceInterface == DEVICE_IF_TYPE_INTERNAL)
	{//3 shbaek: No Have Semantic Descriptor Type
		if(gDebugOneM2M)GRIB_LOGD("# %s TREE: NO HAVE SEMANTIC DESCRIPTOR ...\n", pRowDeviceInfo->deviceID);
		return GRIB_DONE;
	}

	//1 shbaek: 2-3.Semantic Descriptor
	//shbaek: Upload Semantic Descriptor(need xM2M_Origin)
	iRes = Grib_SemanticDescriptorUpload(&reqParam, &resParam);
	if(iRes != GRIB_DONE)
	{
		if(resParam.http_ResNum == HTTP_STATUS_CODE_CONFLICT)
		{//shbaek: Already Exist is Not Error.
			// TODO: shbaek: What Can I Do ... ?
		}
		else
		{
			GRIB_LOGD("# %s TREE: SEMANTIC DESCRIPTOR ERROR !!!\n", pRowDeviceInfo->deviceID);
			//goto ERROR;
		}
	}

	return GRIB_DONE;

ERROR:
	GRIB_LOGD("# CREATE ONE M2M TREE ERROR\n");
	return GRIB_ERROR;
}

#ifdef FEATURE_CAS
int Grib_UpdateHubInfo(char* pAuthKey)
#else
int Grib_UpdateHubInfo(void)
#endif
{
	int  iRes = GRIB_ERROR;
	char pIpAddr[GRIB_MAX_SIZE_IP_STR] = {'\0', };

	Grib_ConfigInfo* pConfigInfo = NULL;

	OneM2M_ReqParam reqParam;
	OneM2M_ResParam resParam;

	STRINIT(pIpAddr, sizeof(pIpAddr));

	MEMSET(&reqParam, 0x00, sizeof(reqParam));
	MEMSET(&resParam, 0x00, sizeof(resParam));

	pConfigInfo = Grib_GetConfigInfo();
	if(pConfigInfo == NULL)
	{
		GRIB_LOGD("LOAD CONFIG ERROR !!!\n");
		return GRIB_ERROR;
	}

	GRIB_LOGD("# UPDATE HUB INFO: %s: %s\n", pConfigInfo->hubID, GRIB_HUB_VERSION);

	iRes = Grib_GetIPAddr(pIpAddr);
	if(iRes != GRIB_DONE)
	{
		GRIB_LOGD("# UPDATE HUB INFO: GET HOST NAME ERROR !!!\n");
	}
	GRIB_LOGD("# UPDATE HUB INFO: HUB IP: %s\n", pIpAddr);

	if(STRLEN(pIpAddr) == 0)
	{
		STRNCPY(pIpAddr, "0.0.0.0", STRLEN("0.0.0.0"));
	}

	STRINIT(reqParam.xM2M_Origin, sizeof(reqParam.xM2M_Origin));
	STRNCPY(reqParam.xM2M_Origin, pConfigInfo->hubID, STRLEN(pConfigInfo->hubID));

	STRINIT(reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI));
	SNPRINTF(reqParam.xM2M_URI, sizeof(reqParam.xM2M_URI), "%s/%s/%s", 
		reqParam.xM2M_Origin, ONEM2M_URI_CONTENT_HUB, ONEM2M_URI_CONTENT_STATUS);

	STRINIT(reqParam.xM2M_CON, sizeof(reqParam.xM2M_CON));
	SNPRINTF(reqParam.xM2M_CON, sizeof(reqParam.xM2M_CON), ONEM2M_FORMAT_CONTENT_VALUE_HUB_INFO, 
		GRIB_HUB_VERSION, pConfigInfo->hubID, pIpAddr);

#ifdef FEATURE_CAS
	reqParam.authKey = pAuthKey;
#endif

	iRes = Grib_ContentInstanceCreate(&reqParam, &resParam);
	if(iRes == GRIB_ERROR)
	{
		GRIB_LOGD("# UPDATE HUB INFO: CREATE INSTANCE ERROR !!!\n");
		return GRIB_ERROR;
	}

	GRIB_LOGD("# UPDATE HUB INFO: DONE\n");

	return GRIB_SUCCESS;

}

#ifdef FEATURE_CAS
int Grib_UpdateDeviceInfo(Grib_DbAll *pDbAll, char* pAuthKey)
#else
int Grib_UpdateDeviceInfo(Grib_DbAll *pDbAll)
#endif
{
	int  i = 0;
	int  iRes = GRIB_ERROR;
	Grib_ConfigInfo* pConfigInfo = NULL;

	if(pDbAll == NULL)
	{
		GRIB_LOGD("# DEVICE INFO NULL ERROR !!!\n");
		return GRIB_ERROR;
	}

#ifdef FEATURE_CAS
	if(STRLEN(pAuthKey)<=1)
	{
		GRIB_LOGD("# AUTH INFO NULL ERROR !!!\n");
		return GRIB_ERROR;
	}
#endif

	MEMSET(&gReqParam, 0x00, sizeof(gReqParam));
	MEMSET(&gResParam, 0x00, sizeof(gResParam));

	pConfigInfo = Grib_GetConfigInfo();
	if(pConfigInfo == NULL)
	{
		GRIB_LOGD("LOAD CONFIG ERROR !!!\n");
		return GRIB_ERROR;
	}
	GRIB_LOGD("# UPDATE DEVICE COUNT: %d EA\n", pDbAll->deviceCount);

	STRINIT(gReqParam.xM2M_Origin, sizeof(gReqParam.xM2M_Origin));
	STRNCPY(gReqParam.xM2M_Origin, pConfigInfo->hubID, STRLEN(pConfigInfo->hubID));

	STRINIT(gReqParam.xM2M_URI, sizeof(gReqParam.xM2M_URI));
	SNPRINTF(gReqParam.xM2M_URI, sizeof(gReqParam.xM2M_URI), "%s/%s/%s", 
		gReqParam.xM2M_Origin, ONEM2M_URI_CONTENT_DEVICE, ONEM2M_URI_CONTENT_STATUS);

	STRINIT(gReqParam.xM2M_CON, sizeof(gReqParam.xM2M_CON));

	for(i=0; i<pDbAll->deviceCount; i++)
	{
		if(i != 0)
		{
			STRCAT(gReqParam.xM2M_CON, ", ");
		}

		STRCAT(gReqParam.xM2M_CON, pDbAll->ppRowDeviceInfo[i]->deviceID);
		STRCAT(gReqParam.xM2M_CON, "(");
		STRCAT(gReqParam.xM2M_CON, pDbAll->ppRowDeviceInfo[i]->deviceAddr);
		STRCAT(gReqParam.xM2M_CON, ")");
	}

#ifdef FEATURE_CAS
	gReqParam.authKey = pAuthKey;
#endif

	iRes = Grib_ContentInstanceCreate(&gReqParam, &gResParam);
	if(iRes == GRIB_ERROR)
	{
		GRIB_LOGD("# UPDATE DEVICE INFO: CREATE INSTANCE ERROR !!!\n");
		return GRIB_ERROR;
	}

	GRIB_LOGD("# UPDATE DEVICE INFO: DONE\n");

	return GRIB_SUCCESS;

}

