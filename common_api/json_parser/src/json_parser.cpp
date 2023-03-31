/**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

//===========================================system===========================================
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <iconv.h>
#include <fcntl.h>
#include <dirent.h>
#include <dirent.h>
#include <semaphore.h>

//=========================================== C++ ===========================================
#include <string>
#include <queue>

//======================================= json parser =======================================
#include "cJSON.h"
#include "json_parser.h"

using namespace std;


static int GetJSONInt(const char * const contents, const char *key)
{
    const cJSON *data = NULL;
    int val = 0;
    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if (contents_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        val = 0;
        goto end;
    }

    data = cJSON_GetObjectItemCaseSensitive(contents_json, key);
    if (cJSON_IsNumber(data)) {
        val = data->valueint;
    }

end:
    cJSON_Delete(contents_json);
    return val;
}

static std::string GetJSONStr(const char * const contents, const char *key)
{
    const cJSON *data = NULL;
    std::string subStr;
    subStr.clear();
    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(contents_json){
        data = cJSON_GetObjectItemCaseSensitive(contents_json, key);
        if (cJSON_IsString(data)) {
            subStr.append(data->valuestring);
        }
        cJSON_Delete(contents_json);
    }else{
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
    }
    return subStr;
}

static std::string GetJSONObject(const char * const contents, const char *key)
{
    const cJSON *data = NULL;
    std::string subStr;
    subStr.clear();
    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(contents_json){
        data = cJSON_GetObjectItemCaseSensitive(contents_json, key);
        if (cJSON_IsObject(data)) {
            subStr.append(cJSON_PrintUnformatted(data));
        }
        cJSON_Delete(contents_json);
    }else{
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
    }
    return subStr;
}
static std::string GetJSONList(const char * const contents, const char *section)
{
    std::string subStr;
    subStr.clear();
    
    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(!contents_json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return subStr;
    }

    cJSON *section_json = cJSON_GetObjectItem(contents_json, section);
    if(!section_json){
        goto end;
    }
    
    subStr.append(cJSON_PrintUnformatted(section_json));
end:
    cJSON_Delete(contents_json);
    return subStr;
}
static int GetJSONListSize(const char * const contents, const char *section)
{
    int listSize = 0;

    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(!contents_json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return listSize;
    }

    cJSON *section_json = cJSON_GetObjectItem(contents_json, section);
    if(!section_json){
        goto end;
    }

    listSize = cJSON_GetArraySize(section_json);
end:
    cJSON_Delete(contents_json);
    return listSize;
}
static int GetJSONListInt(const char * const contents, const char *section, int pos, const char *key)
{
    const cJSON *data = NULL;
    int val = 0;

    cJSON *contents_json = cJSON_Parse(contents); //转成json数据	
    if(!contents_json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return val;
    }

    int listSize = 0;
	cJSON *section_json;
	cJSON *orderSub_json;
	
    section_json = cJSON_GetObjectItem(contents_json, section);
    if(!section_json){
        goto end;
    }

    listSize = cJSON_GetArraySize(section_json);
    if((0 == listSize) || (listSize <= pos)){
        goto end;
    }

    orderSub_json = cJSON_GetArrayItem(section_json, pos);
    if(!orderSub_json){
        goto end;
    }

    data = cJSON_GetObjectItemCaseSensitive(orderSub_json, key);
    if (cJSON_IsNumber(data)) {
        val = data->valueint;
    }

end:
    cJSON_Delete(contents_json);
    return val;
}

static std::string GetJSONListStr(const char * const contents, const char *section, int pos, const char *key)
{
    const cJSON *data = NULL;
    std::string subStr;
    subStr.clear();

    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(!contents_json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return subStr;
    }

    int listSize = 0;
	cJSON *section_json;
	cJSON *orderSub_json;
	
    section_json = cJSON_GetObjectItem(contents_json, section);
    if(!section_json){
        goto end;
    }
    
	orderSub_json = cJSON_GetArrayItem(section_json, pos);
    if(!orderSub_json){
        goto end;
    }

    listSize = cJSON_GetArraySize(section_json);
    if((0 == listSize) || (listSize <= pos)){
        goto end;
    }

    data = cJSON_GetObjectItemCaseSensitive(orderSub_json, key);
    if (cJSON_IsString(data)) {
        subStr.append(data->valuestring);
    }else{
        printf("date is not string\n");
    }

end:
    cJSON_Delete(contents_json);
    return subStr;
}

static std::string GetJSONListObj(const char * const contents, const char *section, int pos)
{
    const cJSON *data = NULL;
    char *pObjStr = NULL;
    std::string subStr;
    subStr.clear();

    cJSON *contents_json = cJSON_Parse(contents); //转成json数据
    if(!contents_json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return subStr;
    }

    int listSize = 0;
	cJSON *section_json;
	cJSON *orderSub_json;
	
    section_json = cJSON_GetObjectItem(contents_json, section);
    if(!section_json){
        goto end;
    }
    
	orderSub_json = cJSON_GetArrayItem(section_json, pos);
    if(!orderSub_json){
        goto end;
    }

    listSize = cJSON_GetArraySize(section_json);
    if((0 == listSize) || (listSize <= pos)){
        goto end;
    }

#if 0
    data = cJSON_GetObjectItemCaseSensitive(orderSub_json, key);
    if (cJSON_IsString(data)) {
        subStr.append(data->valuestring);
    }else{
        printf("date is not string\n");
    }
#else
    pObjStr = cJSON_Print(orderSub_json);
    if(pObjStr){
        subStr.append(pObjStr);
        cJSON_free(pObjStr);
    }
#endif

end:
    cJSON_Delete(contents_json);
    return subStr;
}

bool string_is_json_object(const char *json_str)
{
    if(NULL == json_str)
        return false;

    if(!(0 < strlen(json_str)))
        return false;

    cJSON *pJSONObj = cJSON_Parse(json_str); //转成json数据
    if(NULL == pJSONObj)
        return false;

    if(cJSON_IsObject(pJSONObj)||cJSON_IsArray(pJSONObj)){
        return true;
    }else{
        return false;
    }
}


int32_t get_int32_from_json(const char *json_str, const char *key)
{
	int32_t retValue;
	retValue = (int32_t)GetJSONInt(json_str, key);
	return retValue;
}

int32_t get_string_from_json(const char *json_str, const char *key, char *data, uint32_t dataLen)
{
	uint32_t minDataLength = dataLen;
	
	std::string strContents;
	strContents.clear();
	
	if((NULL == data) || (0 == dataLen))
		return -1;
	
	strContents = GetJSONStr(json_str, key);
	if(strContents.empty()){
		memset(data, 0, minDataLength);
		minDataLength = 0;
	}else{
		if(strContents.length() < dataLen){
			minDataLength = strContents.length();
		}
		
		memcpy(data, strContents.c_str(), minDataLength);
	}
	
	return minDataLength;
}

int32_t get_object_from_json(const char *json_str, const char *key, char *data, uint32_t dataLen)
{
	uint32_t minDataLength = dataLen;
	
	std::string strContents;
	strContents.clear();
	
	if((NULL == data) || (0 == dataLen))
		return -1;
	
	strContents = GetJSONObject(json_str, key);
	if(strContents.empty()){
		memset(data, 0, minDataLength);
		minDataLength = 0;
	}else{
		if(strContents.length() >= dataLen){
			memset(data, 0, minDataLength);
			minDataLength = -1;
		}else{
			minDataLength = strContents.length();
			memcpy(data, strContents.c_str(), minDataLength);
		}
	}
	
	return minDataLength;
}
int32_t get_list_from_json(const char *json_str, const char *key, char *data, uint32_t dataLen)
{
	uint32_t minDataLength = dataLen;
	
	std::string strContents;
	strContents.clear();
	
	if((NULL == data) || (0 == dataLen))
		return -1;
	
	strContents = GetJSONList(json_str, key);
	if(strContents.empty()){
		memset(data, 0, minDataLength);
		minDataLength = 0;
	}else{
		if(strContents.length() >= dataLen){
			memset(data, 0, minDataLength);
			minDataLength = -1;
		}else{
			minDataLength = strContents.length();
			memcpy(data, strContents.c_str(), minDataLength);
		}
	}
	
	return minDataLength;
}


int32_t get_list_size_from_json(const char *json_str, const char *list_key)
{
	int32_t listSize;
	listSize = (int32_t)GetJSONListSize(json_str, list_key);
	return listSize;
}

int32_t get_object_from_list(const char *json_str, const char *list_name, int pos, char *data, uint32_t dataLen)
{
	uint32_t minDataLength = dataLen;
	
	std::string strContents;
	strContents.clear();
	
	if((NULL == data) || (0 == dataLen))
		return -1;
	
	strContents = GetJSONListObj(json_str, list_name, pos);
	if(strContents.empty()){
        printf("strContents is empty\n");
		memset(data, 0, minDataLength);
		minDataLength = 0;
	}else{
		if(strContents.length() <=dataLen){
			minDataLength = strContents.length();
		}
		
		memcpy(data, strContents.c_str(), minDataLength);
	}
	
	return minDataLength;
}

int32_t get_int32_from_list(const char *json_str, const char *list_name, int pos, const char *key)
{
	int32_t retValue;
	retValue = (int32_t)GetJSONListInt(json_str, list_name, pos, key);
	return retValue;
}

int32_t get_string_from_list(const char *json_str, const char *list_name, int pos, const char *key, char *data, uint32_t dataLen)
{
	uint32_t minDataLength = dataLen;
	
	std::string strContents;
	strContents.clear();
	
	if((NULL == data) || (0 == dataLen))
		return -1;
	
	strContents = GetJSONListStr(json_str, list_name, pos, key);
	if(strContents.empty()){
        printf("strContents is empty\n");
		memset(data, 0, minDataLength);
		minDataLength = 0;
	}else{
		if(strContents.length() <=dataLen){
			minDataLength = strContents.length();
		}
		
		memcpy(data, strContents.c_str(), minDataLength);
	}
	
	return minDataLength;
}

void *create_json_object()
{
    void *pObj = NULL;

	cJSON *pContents = cJSON_CreateObject();
    pObj = (void *)pContents;

    return pObj;
}
void  add_null_to_object(void *pObj, const char * const key)
{
    if(pObj) {
        cJSON_AddNullToObject((cJSON *)pObj, key);
    }
}

void add_bool_to_object(void *pObj, const char * const key, bool bTorF)
{
    if(pObj) {
        cJSON_AddBoolToObject((cJSON *)pObj, key, (const cJSON_bool)bTorF);
    }
}

void add_number_to_object(void *pObj, const char * const key, double number)
{
    if(pObj) {
        cJSON_AddNumberToObject((cJSON *)pObj, key, number);
    }
}

void add_string_to_object(void *pObj, const char * const key, const char * const string)
{
    if(pObj) {
        cJSON_AddStringToObject((cJSON *)pObj, key, string);
    }
}

void *add_object_to_object(void *pParentObj, const char * const subObjName)
{
    void *pSubObj = NULL;
	cJSON *pContents = NULL;
    
    if(pParentObj) {
        pContents = cJSON_AddObjectToObject((cJSON *)pParentObj, subObjName);
        pSubObj = (void *)pContents;
    }

    return pSubObj;
}

void  add_object_to_object2(void *pParentObj, const char * const subObjName, void *pSubObj)
{
    if((pParentObj)&&(pSubObj)) {
        cJSON_AddItemToObject((cJSON *)pParentObj, subObjName, (cJSON *)pSubObj);
    }
}

void  add_object_to_object3(void *pParentObj, const char * const subObjName, char *pSubObjSrt)
{
    if(pParentObj) {
        cJSON *pSubObj = cJSON_Parse(pSubObjSrt);
        if(pSubObj){
            if(false == cJSON_AddItemToObject((cJSON *)pParentObj, subObjName, (cJSON *)pSubObj)) {
                cJSON_Delete(pSubObj);
            }
        }else{
            printf("SubObjSrt is invaild json\n");
        }
    }
}

void *create_item_object(const char *contents)
{
    void *pObj = NULL;
    cJSON *pContents = NULL;

    bool bNeedCreateEmptyObj = true;
    if(contents){
        if(0 <= strlen(contents)){
            bNeedCreateEmptyObj = false;
        }
    }

    if(bNeedCreateEmptyObj){
	    pContents = cJSON_CreateObject();
    }else{
        pContents = cJSON_Parse(contents); //转成json数据
        if(NULL == pContents){
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                fprintf(stderr, "Error before: %s\n", error_ptr);
            }
        }
    }
    pObj = (void *)pContents;

    return pObj;
}

void *create_list_object()
{
    void *pObj = NULL;

	cJSON *pContents = cJSON_CreateArray();
    pObj = (void *)pContents;

    return pObj;
}

void  add_item_to_list(void *pList, void *pItem)
{
    if((pList)&&(pItem)) {
        cJSON_AddItemToArray((cJSON *)pList, (cJSON *)pItem);
    }
}

void *add_list_to_object(void *pParentObj, const char * const listName)
{
    void *pList = NULL;
	cJSON *pContents = NULL;
    
    if(pParentObj) {
        pContents = cJSON_AddArrayToObject((cJSON *)pParentObj, listName);
        pList = (void *)pContents;
    }

    return pList;
}

void add_list_to_object2(void *pParentObj, const char * const listName, void *pListObj)
{  
    if((NULL != pParentObj)&&(NULL != pListObj)) {
        cJSON_AddItemToObject((cJSON *)pParentObj, listName, (cJSON *)pListObj);
    }
}

char *object_data(void *pObject, bool bIsUnformatted)
{
    if(pObject){
        if (cJSON_IsObject((cJSON *)pObject)||cJSON_IsArray((cJSON *)pObject)) {
            if(bIsUnformatted){
                return cJSON_PrintUnformatted((cJSON *)pObject);
            }else{
                return cJSON_Print((cJSON *)pObject);
            }
        }else{
            return NULL;
        }
    }else{
        return NULL;
    }
}

void free_data(char *pData)
{
    if(pData){
        cJSON_free(pData);
    }
}

int32_t delete_json_object(void *pObject)
{
    int32_t ret = -1;

    if(pObject) {
        cJSON_Delete((cJSON *)pObject);
        ret = 0;
    }
    
    return ret;
}

