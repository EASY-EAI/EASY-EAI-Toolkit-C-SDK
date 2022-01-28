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

    data = cJSON_GetObjectItemCaseSensitive(orderSub_json, key);
    if (cJSON_IsString(data)) {
        subStr.append(data->valuestring);
    }

end:
    cJSON_Delete(contents_json);
    return subStr;
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

int32_t get_list_size_from_json(const char *json_str, const char *list_key)
{
	int32_t listSize;
	listSize = (int32_t)GetJSONListSize(json_str, list_key);
	return listSize;
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


static std::queue<std::string> g_keyList;
static std::queue<std::string> g_typeList;
static std::queue<std::string> g_valueList;
void clear_json_cache()
{
	std::queue<std::string> keyEmpty;
	std::queue<std::string> typeEmpty;
	std::queue<std::string> valEmpty;
	swap(keyEmpty,  g_keyList);
	swap(typeEmpty, g_typeList);
	swap(valEmpty,  g_valueList);
	
	return ;
}

//type为空，默认按string处理
void add_json_cache(const char *key, const char *type, const char *value)
{
	if( (NULL == key)||(0 == strlen(key)) )
		return ;
	
	std::string strKey;
	strKey.clear();
	strKey.append(key);
	
	std::string strType;
	strType.clear();
	if( (NULL == type) || (0 == strlen(type)) ){
		strType.append("string");
	}else{
		strType.append(type);
	}
	
	std::string strValue;
	strValue.clear();
	if(NULL != value){
		strValue.append(value);
	}
	
	
	g_keyList.push(strKey);
	g_typeList.push(strType);
	g_valueList.push(strValue);
	
	return ;
}

int32_t create_json_string(char *json_string, uint32_t dataLen)
{
    char *string = NULL;
	int minDataLength = 0;
	if(NULL == json_string)
		return -1;
	
	cJSON *contents = cJSON_CreateObject();

	std::string key;
	std::string type;
	std::string value;

    if(g_keyList.empty() || g_typeList.empty() || g_valueList.empty())
		return -1;
		
    while(1){
        if(g_keyList.empty() || g_typeList.empty() || g_valueList.empty())
            break;

        key = g_keyList.front();
        g_keyList.pop();

        type = g_typeList.front();
        g_typeList.pop();
		
        value = g_valueList.front();
        g_valueList.pop();
	
		//只要类型不是number，一律按字符串处理
		if(0 == strcmp(type.c_str(), "number")){
			if (cJSON_AddNumberToObject(contents, key.c_str(), atoi(value.c_str())) == NULL){
				goto end;
			}
		}else{
			if (cJSON_AddStringToObject(contents, key.c_str(), value.c_str()) == NULL){
				goto end;
			}
		}
    }

    string = cJSON_Print(contents);
    if (string == NULL) {
        fprintf(stderr, "Failed to print monitor.\n");
    }else{
		minDataLength = strlen(string);
		if(minDataLength > dataLen){
			minDataLength = dataLen;
		}
		memcpy(json_string, string, minDataLength);
	}
end:
	clear_json_cache();
    cJSON_Delete(contents);
    return minDataLength;
}

