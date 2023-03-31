/**
 *
 * Copyright 2022 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: ZJH <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "network.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/node/convert.h"
#include <arpa/inet.h>


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include <string>
//using namespace std;
/*********************************************************************
Function: 
Description:
    获取网络数据流量统计信息
Example:
    int64_t recvByte, sendByte;
    int32_t ret = get_dataflow_statistics("eth0", &recvByte, &snedByte);
parameter:
    *device  : 网卡
    *total_recv : 累计已接收(下行)的数据量，单位：字节(Byte)
    *total_send : 累计已发送(上行)的数据量，单位：字节(Byte)
Return:
    成功：0
    失败：-1 
********************************************************************/
int32_t get_dataflow_statistics(const char *device, int64_t *total_recv, int64_t *total_send) 
{
     if((NULL == device)||(NULL == total_recv)||(NULL == total_send)) {
        printf("bad param\n");
        return -1;    
    }
    
    FILE *netdev_Fp = NULL;
    netdev_Fp = fopen("/proc/net/dev", "r");
    if (NULL == netdev_Fp) {
        printf("open file /proc/net/dev/ error!\n");
        return -1;
    }
    
    int counter = 0;
    char *match = NULL;    //用以保存所匹配字符串及之后的内容
    char buffer[1024];  //文件中的内容暂存在字符缓冲区里
    char tmp_value[128];
    
    memset(buffer, 0, sizeof(buffer));
    while(NULL != fgets(buffer, sizeof(buffer), netdev_Fp))
    {
        match = strstr(buffer, device);
        if(NULL == match) {
            //printf("no eth0 keyword to find!\n");
            continue;
        } else {
            //printf("%s\n",buffer);
            match = match + strlen(device) + strlen(":");/*地址偏移到冒号*/
            sscanf(match, "%lld ", total_recv);
            memset(tmp_value, 0, sizeof(tmp_value));
            sscanf(match, "%s ", tmp_value);
            match = match + strlen(tmp_value);
            for(size_t i = 0; i < strlen(buffer); i++)
            {
                if(0x20 == *match) {
                    match++;
                } else {
                    if(8 == counter) {
                        sscanf(match,"%lld ", total_send);
                    }
                    memset(tmp_value,0,sizeof(tmp_value));
                    sscanf(match,"%s ",tmp_value);
                    match = match + strlen(tmp_value);
                    counter++;
                }
            }
            //printf("%s save_rate:%ld tx_rate:%ld\n",netname,*save_rate,*tx_rate);
        }
    }
    fclose(netdev_Fp);
    return 0;
}

class network
{
public:
    static network *instance(){
        if(NULL == pSelf){
            pSelf =  new network;
        }
        return pSelf;
    }
    network(){
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        mbEth0IsDHCP = config["network"]["ethernets"]["eth0"]["dhcp4"].as<bool>();
        mbWlan0IsDHCP = config["network"]["wifis"]["wlan0"]["dhcp4"].as<bool>();
    }
    ~network();

    bool get_eth0_bIsDHCP(){return mbEth0IsDHCP;}
    void set_eth0_bIsDHCP(bool bIsDHCP){this->mbEth0IsDHCP = bIsDHCP;};

    bool get_wifi_bIsDHCP(){return mbWlan0IsDHCP;}
    void set_wifi_bIsDHCP(bool bIsDHCP){this->mbWlan0IsDHCP = bIsDHCP;};

    static network *pSelf;
private:
    bool mbEth0IsDHCP;
    bool mbWlan0IsDHCP;
};
network* network::pSelf = nullptr;


/*********************************************************************
Function:重启网卡
Description:
Example:
parameter:
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t restart_network_device()
{
    system("sudo netplan generate");
    return system("sudo netplan apply");
}


//将mask地址转换为CIDR形式
static std::string mask_transition_cidr(std::string ip_str , std::string mask_str )
{
    struct in_addr mask_addr;
    inet_pton(AF_INET, mask_str.c_str(), &mask_addr);
    int mask = 0;
    for (int i = 0; i < 32; i++) {
        if ((mask_addr.s_addr >> i) & 1) {
        mask++;
        }
    }
    std::string cidr = ip_str + "/" + std::to_string(mask);
    printf("ip_str:%s , mask_str:%s , cidr:%s , mask:%d\n",ip_str.c_str(),mask_str.c_str(),cidr.c_str(),mask);
    return cidr;
}

/*********************************************************************
Function:
Description:
    设置：eth0,wifi的DHCP
Example:
    int ret =set_net_dhcp("eth0", true);
parameter:
    *device  : 网卡
      enable : dhcp开关
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t set_ipv4_dhcp(const char *device)
{
    if(0 == strlen(device))
    return -1;
    // 加载YAML文件
    YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
    if(0 == strcmp(device, "eth0")){
        config["network"]["ethernets"]["eth0"]["dhcp4"] = "true";
        //删除静态ip地址,网关
        config["network"]["ethernets"]["eth0"].remove("addresses");
        config["network"]["ethernets"]["eth0"].remove("routes");
        network::instance()->set_eth0_bIsDHCP(true);
    }else if(0 == strcmp(device, "wifi")){
        config["network"]["wifis"]["wlan0"]["dhcp4"] = "true"; 
        config["network"]["wifis"]["wlan0"].remove("addresses");
        config["network"]["wifis"]["wlan0"].remove("routes");
        network::instance()->set_wifi_bIsDHCP(true);
    }
    // 将修改后的YAML文档写回到文件中
    std::ofstream fout("/etc/netplan/99_config.yaml");
    fout << config;
    fout.close();
    return 0;
}
/*********************************************************************
Function:
Description:
    设置：ip地址、子网掩码、网关
Example:
    int ret = set_net_ipv4("eth0","192.168.1.151", "255.255.255.0", "192.168.1.1");
parameter:
    *device  : 网卡
    *ip      : 点分十进制的IP地址
    *mask    : 点分十进制的子网掩码
    *gateway : 点分十进制的网关
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t set_ipv4_static(const char *device, const char *ip, const char *mask, const char *gateway)
{
	if(0 == strlen(device))
		return -1;
	
	if( address_invaild(ip)||address_invaild(mask) )
		return -1;

	/*判断IP与网关是否处于同一网段*/
	bool bIsSetGateway = true;
	if(address_invaild(gateway)){
		bIsSetGateway = false;
	}else{
		uint8_t binIP[4], binMask[4], binGateWay[4];
		ipv4_str_to_bin((char *)ip, (char *)binIP);
		ipv4_str_to_bin((char *)mask, (char *)binMask);
		ipv4_str_to_bin((char *)gateway, (char *)binGateWay);
		uint8_t netIP, netGW;
		for(int i = 0; i < 4; i++){
			netIP = binMask[i] & binIP[i];
			netGW = binMask[i] & binGateWay[i];
			if(netIP != netGW){
				bIsSetGateway = false;
				break;
			}
		}
	}
    if(0 == strcmp(device, "eth0")){
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        //关闭dhcp
        config["network"]["ethernets"]["eth0"]["dhcp4"] = "false";
        // 添加 CIDR 表示法的 IP 地址
        std::string cidr = mask_transition_cidr(ip,mask);
        YAML::Node eth0 = config["network"]["ethernets"]["eth0"];
        // 获取 addresses 序列
        YAML::Node addresses = eth0["addresses"];

        // 如果地址存在，则替换原有的地址；否则，创建一个新的地址
        if (addresses[0]) {
            addresses[0] = cidr;
        } else {
            addresses.push_back(cidr);
        }

        // 修改eth0的routes地址
        if(bIsSetGateway){
            config["network"]["ethernets"]["eth0"]["routes"][0]["to"] = "0.0.0.0/0";
            config["network"]["ethernets"]["eth0"]["routes"][0]["via"] = gateway;
            config["network"]["ethernets"]["eth0"]["routes"][0]["metric"] = "100";
        }
        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
        return 0;
    }else if(0 == strcmp(device, "wifi")){
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        config["network"]["wifis"]["wlan0"]["dhcp4"] = "false"; 
        // 添加 CIDR 表示法的 IP 地址
        std::string cidr = mask_transition_cidr(ip,mask);
        YAML::Node wlan0 = config["network"]["wifis"]["wlan0"];
        // 获取 addresses 序列
        YAML::Node addresses = wlan0["addresses"];

        // 如果地址存在，则替换原有的地址；否则，创建一个新的地址
        if (addresses[0]) {
            addresses[0] = cidr;
        } else {
            addresses.push_back(cidr);
        }

        // 修改eth0的routes地址
        if(bIsSetGateway){
            config["network"]["wifis"]["wlan0"]["routes"][0]["to"] = "0.0.0.0/0";
            config["network"]["wifis"]["wlan0"]["routes"][0]["via"] = gateway;
            config["network"]["wifis"]["wlan0"]["routes"][0]["metric"] = "100";
        }
        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
        return 0;
    }else{
		return -1;
	}
}

/*********************************************************************
Function:
Description:
    设置：ip地址、子网掩码、网关
Example:
    int ret = add_second_net("eth0","192.168.1.151", "255.255.255.0");
parameter:
    *device  : 网卡
    *ip      : 点分十进制的IP地址
    *mask    : 点分十进制的子网掩码
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t add_second_ipv4(const char *device, const char *ip, const char *mask)
{
	if(0 == strlen(device))
		return -1;

	if( address_invaild(ip)||address_invaild(mask) )
		return -1;

    if(0 == strcmp(device, "eth0")){
        if(network::instance()->get_eth0_bIsDHCP()){
            return -1;
        }
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        // 添加 CIDR 表示法的 IP 地址
        std::string cidr = mask_transition_cidr(ip,mask);
        YAML::Node eth0 = config["network"]["ethernets"]["eth0"];
        eth0["addresses"][1] = cidr;
        config["network"]["ethernets"]["eth0"] = eth0;
        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
    }else if(0 == strcmp(device, "wifi")){
        if(network::instance()->get_wifi_bIsDHCP()){
            return -1;
        }
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        // 添加 CIDR 表示法的 IP 地址
        std::string cidr = mask_transition_cidr(ip,mask);
        YAML::Node wifi = config["network"]["wifis"]["wlan0"];
        wifi["addresses"][1] = cidr;
        config["network"]["wifis"]["wlan0"] = wifi;
        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
    }
    return 0;    
}

/*********************************************************************
Function:
Description:
    设置：ip地址、子网掩码、网关
Example:
    int ret = delete_second_net("eth0");
parameter:
    *device  : 网卡
    *ip      : 点分十进制的IP地址
    *mask    : 点分十进制的子网掩码
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t delete_second_ipv4(const char *device)
{
	if(0 == strlen(device))
		return -1;


    if(0 == strcmp(device, "eth0")){
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
        // 获取 eth0 的配置
        YAML::Node eth0_addresses = config["network"]["ethernets"]["eth0"]["addresses"];

        std::vector<std::string> addresses;
        for (std::size_t i = 0; i < eth0_addresses.size(); i++) {
            std::string address = eth0_addresses[i].as<std::string>();
            addresses.push_back(address);
            std::cout << "Address " << i << ": " << address << std::endl;
        }

        if (addresses.size() > 1) { // 如果有多于一个地址
            // 清除第二个地址
            std::cout << "Before removal: " << addresses[1] << std::endl;
            addresses.erase(addresses.begin() + 1);
            std::cout << "After removal: " << addresses[1] << std::endl;
            // 更新 YAML 节点
            YAML::Node new_addresses;
            for (const auto& address : addresses) {
                new_addresses.push_back(address);
            }
            config["network"]["ethernets"]["eth0"]["addresses"] = new_addresses;
        }

        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
    }else if(0 == strcmp(device, "wifi")){
        // 加载YAML文件
        YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
       // 获取 wlan0 的配置
        YAML::Node wlan0_addresses = config["network"]["wifis"]["wlan0"]["addresses"];

       std::vector<std::string> addresses;
        for (std::size_t i = 0; i < wlan0_addresses.size(); i++) {
            std::string address = wlan0_addresses[i].as<std::string>();
            addresses.push_back(address);
            std::cout << "Address " << i << ": " << address << std::endl;
        }

        if (addresses.size() > 1) { // 如果有多于一个地址
            // 清除第二个地址
            std::cout << "Before removal: " << addresses[1] << std::endl;
            addresses.erase(addresses.begin() + 1);
            std::cout << "After removal: " << addresses[1] << std::endl;
            // 更新 YAML 节点
            YAML::Node new_addresses;
            for (const auto& address : addresses) {
                new_addresses.push_back(address);
            }
            config["network"]["wifis"]["wlan0"]["addresses"] = new_addresses;
        }

        if (wlan0_addresses.size() > 1) { // 如果有多于一个地址
            // 清除第二个地址
            std::cout << "Before removal: " << wlan0_addresses[1] << std::endl;
            wlan0_addresses.remove(1);
            std::cout << "After removal: " << wlan0_addresses[1] << std::endl;
        }
        // 将修改后的YAML文档写回到文件中
        std::ofstream fout("/etc/netplan/99_config.yaml");
        fout << config;
        fout.close();
    }
    return 0;
}






//判断dns地址合法性
//地址只能是1.0.0.0~223.255.255.255的数字
static bool is_valid_dns_server(const std::string& server) {
    std::regex pattern(R"((1\d{0,2}|2[0-1]\d|2[2-3][0-5]|\d{1,2})(\.(1\d{0,2}|2[0-1]\d|2[2-3][0-5]|\d{1,2})){3})");
    return std::regex_match(server, pattern);
}
/*********************************************************************
Function:
Description:
    设置：eth0,wifi的dns
Example:
    int ret =set_net_dns("eth0",true);
parameter:
    *device  : 网卡
      enable : dns_dhcp开关
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t set_ipv4_dns_dhcp(const char *device,bool enable)
{
    if(0 == strlen(device))
        return -1;

    //若开启dns_dhcp---->必须对应ip_dhcp开启
    if(true == enable){
        if((0 == strcmp(device, "eth0"))  &&  (false == network::instance()->get_eth0_bIsDHCP())){
            return -1;
        }else if(0 == strcmp(device, "wifi")   &&  (false == network::instance()->get_wifi_bIsDHCP())){
            return -1;
        }
    }

    // 加载YAML文件
    YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
    if(0 == strcmp(device, "eth0")){
        if(true == enable ){
            config["network"]["ethernets"]["eth0"]["nameservers"].remove("addresses");
            config["network"]["ethernets"]["eth0"]["nameservers"]["dhcp4"] = true;
        }else{

        }
    }else if(0 == strcmp(device, "wifi")){
        if(true == enable ){
            config["network"]["wifis"]["wlan0"]["nameservers"].remove("addresses");
            config["network"]["wifis"]["wlan0"]["nameservers"]["dhcp4"] = true;
        }else{
            
        }
    }
    // 将修改后的YAML文档写回到文件中
    std::ofstream fout("/etc/netplan/99_config.yaml");
    fout << config;
    fout.close();

    return 0;
}

/*********************************************************************
Function:
Description:
    设置：eth0,wifi的dns
Example:
    int ret =set_net_dns("eth0", "8.8.8.8","8.8.4.4");
parameter:
    *device  : 网卡
      enable : dhcp开关
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t set_ipv4_dns_static(const char *device, const char *primary_dns,const char *alternative_dns)
{
    if(0 == strlen(device))
        return -1;

    std::string primary_dns_str(primary_dns);
    if (false == is_valid_dns_server(primary_dns_str)) 
        return -1;

    if(alternative_dns != NULL){
        std::string alternative_dns_str(alternative_dns);
        if (false == is_valid_dns_server(alternative_dns_str)) 
            return -1;
    }    

    // 加载YAML文件
    YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
    if(0 == strcmp(device, "eth0")){
        // 修改eth0的DNS地址
        YAML::Node addresses_eht0_dns;//[8.8.8.8, 8.8.4.4]这是一个序列，需要转换
        if(alternative_dns != NULL){
            addresses_eht0_dns.push_back(primary_dns);
            addresses_eht0_dns.push_back(alternative_dns);
        }else{
            addresses_eht0_dns.push_back(primary_dns);
        }
        config["network"]["ethernets"]["eth0"]["nameservers"].remove("dhcp4");
        config["network"]["ethernets"]["eth0"]["nameservers"]["addresses"] = addresses_eht0_dns;
    }else if(0 == strcmp(device, "wifi")){
        // 修改eth0的DNS地址
        YAML::Node addresses_wifi_dns;//[8.8.8.8, 8.8.4.4]这是一个序列，需要转换
        if(alternative_dns != NULL){
            addresses_wifi_dns.push_back(primary_dns);
            addresses_wifi_dns.push_back(alternative_dns);
        }else{
            addresses_wifi_dns.push_back(primary_dns);
        }
        config["network"]["wifis"]["wlan0"]["nameservers"].remove("dhcp4");
        config["network"]["wifis"]["wlan0"]["nameservers"]["addresses"] = addresses_wifi_dns;
    }
    // 将修改后的YAML文档写回到文件中
    std::ofstream fout("/etc/netplan/99_config.yaml");
    fout << config;
    fout.close();

    return 0;
}


/*********************************************************************
Function:
Description:
    设置：wifi名字，密码
Example:
    int ret =  set_wifi_WAP2("HUAWEI-0H1YW8", "lmo12345678");
parameter:
    *device  : 网卡
    *ip      : 点分十进制的IP地址
    *mask    : 点分十进制的子网掩码
    *gateway : 点分十进制的网关
Return:
	重启网络脚本的执行结果
********************************************************************/
int32_t set_wifi_WAP2(const char *access, const char *password)
{
    if(0 == strlen(access))
        return -1;

    // std::string access_str = "\"" + std::string(access) + "\""; // 加上双引号
    // std::string password_str = "\"" + std::string(password) + "\""; // 加上双引号
    // printf("access:%s , access_str:%s  ;;;;; password:%s , password_str:%s",access,access_str.c_str(),password,password_str.c_str());
    YAML::Node config = YAML::LoadFile("/etc/netplan/99_config.yaml");
    YAML::Node accessPoint;
    accessPoint[access]["password"] = password;
    config["network"]["wifis"]["wlan0"]["access-points"] = accessPoint;
    // 将修改后的YAML文档写回到文件中
    std::ofstream fout("/etc/netplan/99_config.yaml");
    fout << config;
    fout.close();
    return 0;
}


/*********************************************************************
Function:
Description:
    读取 MAC 地址
Example:
	#define MAX_MAC_LEN 8
    char mac[MAX_MAC_LEN];
    if(0 == get_local_Mac("eth0", mac, MAX_MAC_LEN)) {
		printf("get mac succ\n");
	}
parameter:
    *device : 网卡
    *mac    : 用于存放读出来的mac地址
    ip_len  : 用于存放mac地址的缓存长度
Return:
	-1：失败
	0 ：成功
********************************************************************/
int32_t get_local_Mac(const char *device, char *mac, int mac_len)
{
	int sockfd;
    struct ifreq req;

    memset(mac, 0, mac_len);
    if (-1 == (sockfd = socket(PF_INET, SOCK_DGRAM, 0)))
    {
        fprintf(stderr,"Sock Error: %s \n\a", strerror(errno));
        return -1;
    }

    bzero(&req, sizeof(req));
    strncpy(req.ifr_name, device, sizeof(req.ifr_name));

    if (-1 == ioctl(sockfd, SIOCGIFHWADDR, (char *)&req))
    {
        fprintf ( stderr,"ioctl SIOCGIFHWADDR:%s\n\a",strerror ( errno ) );
        close(sockfd);
        return -1;
    }
    
    memcpy(mac, req.ifr_hwaddr.sa_data, 6);

    return 0;
}

/*********************************************************************
Function:
Description:
    读取IP地址
Example:
	#define MAX_IP_LEN 20
    char ip[MAX_IP_LEN];
    if(0 == get_local_Ip("eth0", ip, MAX_IP_LEN)) {
		printf("get ip succ\n");
	}
parameter:
    *device : 网卡
    *ip     : 用于存放读出来的ip地址
    ip_len  : 用于存放ip地址的缓存长度
Return:
	-1：失败
	0 ：成功
********************************************************************/
int32_t get_local_Ip(const char *device, char *ip, int ip_len)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    memset(ip, 0, ip_len);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, device, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf(ip, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

/*********************************************************************
Function:
Description:
    读取子网掩码
Example:
	#define MAX_MASK_LEN 20
    char netMask[MAX_MASK_LEN];
    if(0 == get_local_NetMask("eth0", netMask, MAX_MASK_LEN)) {
		printf("get netMask succ\n");
	}
parameter:
    *device     : 网卡
    *netMask    : 用于存放读出来的子网掩码
    netMask_len : 用于存放子网掩码的缓存长度
Return:
	-1：失败
	0 ：成功
********************************************************************/
int32_t get_local_NetMask(const char *device, char *netMask, int netMask_len)
{ 
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;
	
    memset(netMask, 0, netMask_len);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }
    
    ifr.ifr_addr.sa_family = AF_INET;
    
    strncpy(ifr.ifr_name, device, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sd, SIOCGIFNETMASK, &ifr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf(netMask, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
	return 0;
}

/*********************************************************************
Function:
Description:
    读取默认网关
Example:
	#define MAX_GW_LEN 20
    char gateWay[MAX_GW_LEN];
    if(0 == get_local_GateWay("eth0", gateWay, MAX_GW_LEN)) {
		printf("get gateWay succ\n");
	}
parameter:
    *device     : 网卡
    *gateWay    : 用于存放读出来的默认网关
    gateWay_len : 用于存放默认网关的缓存长度
Return:
	-1：失败
	0 ：成功
********************************************************************/
int32_t get_local_GateWay(const char *device, char *gateWay, int gateWay_len)
{
    memset(gateWay, 0, gateWay_len);
	
	char cmd[128] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"route -n|grep %s|grep 0.0.0.0|awk \'NR==1 {print $2}\'", device);
    FILE* fp = popen( cmd, "r" );
    if(NULL == fp) {
        printf("popen error: %s\n", strerror(errno));
        return -1;
    }
    
	while ( NULL != fgets(gateWay, gateWay_len, fp)) {
        if(gateWay[strlen(gateWay)-1] == '\n') {
           gateWay[strlen(gateWay)-1] = 0;
        }
        break;
    }
    pclose(fp);
	return 0;
}

/*********************************************************************
Function:
Description:
	把字符串形式的IP地址转换为数字形式
Example:
	char *strIP = "192.168.1.233";
	char ipAddr[4] = {0};
	if(ipv4_str_to_bin(strIP, ipAddr)){
		printf("[ipAddr] = %02x:%02x:%02x:%02x\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
	}
parameter:
    *strIP : 输入的 ip地址字符串
    *binIP : 输出的 ip地址数组
Return:
	true 成功 false 失败
********************************************************************/
bool ipv4_str_to_bin(const char *strIP, char *binIP)
{
    int i;
    char *s, *e;

    if ((binIP == NULL) || (strIP == NULL))
    {
        return false;
    }

    s = (char *) strIP;
    for (i = 0; i < 4; ++i)
    {
        binIP[i] = s ? strtoul(s, &e, 10) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }
    return true;
}

/*********************************************************************
Function:
Description:
	把字符串形式的mac地址转换为数字形式
Example:
	char *strMac = "BC:8D:0A:7F:85:5E";
	char mac[6] = {0};
	if(mac_str_to_bin(strMac, mac)){
		printf("[mac] = %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
parameter:
    *strMac : 输入的mac地址字符串
    *binMac : 输出的mac地址数组
Return:
	true 成功 false 失败
********************************************************************/
bool mac_str_to_bin(const char *strMac, char *binMac)
{
    int i;
    char *s, *e;

    if ((binMac == NULL) || (strMac == NULL))
    {
        return false;
    }

    s = (char *) strMac;
    for (i = 0; i < 6; ++i)
    {
        binMac[i] = s ? strtoul(s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }
    return true;
}

/*********************************************************************
Function:
Description:
	判断IP地址是否遵循点分十进制
Example:
	if(!address_invaild("192.168.001.110"){
		printf("ip is vaild\n");
	}else{
		printf("ip is invaild\n");		
	}
parameter:
    *addr : 待判定的Ipv4地址
Return:
	true 无效的地址
	false 有效的地址
********************************************************************/
bool address_invaild(const char *addr_IPv4)
{
	int8_t pointNum = 0;
	for(uint32_t i = 0; i < strlen(addr_IPv4); i++){
		if('.' == addr_IPv4[i]){
			pointNum++;
		}
	}
	
	if(3 != pointNum)
		return true;
	
#if 0 //这段没啥用，遇到非数字就会分割。超出255就会取余
	uint8_t ip[4];
	ipv4_str_to_bin((char *)addr_IPv4, (char *)ip);
	for(uint32_t i = 0; i < sizeof(ip); i++){
		if((ip[i] <= 0) || (ip[i] >= 255))
			return true;
	}
#endif
	
	return false;
}

/*********************************************************************
Function:
Description:
	比较为域名形式或为点分十进制的字符串形式的IP地址，
	忽略点分十进制前面或中间无效的0
Example:
	if(compare_IpAddr("192.168.001.110", "192.168.01.110")){
		printf("ip is sam\n");
	}else{
		printf("ip is not  sam\n");		
	}
parameter:
    *addr1 : 待比对Ip地址
    *addr2 : 待比对Ip地址
Return:
	true 相同 false 不同
********************************************************************/
bool compare_IpAddr(char *addr1, char *addr2)
{
	char tmpAddr1[64] = ".";
	char tmpAddr2[64] = ".";
	uint32_t i = 0, j = 0;
	bool ret = false;

	if ((addr1[0] < '0' || addr1[0] > '9') || (addr2[0] < '0' || addr2[0] > '9'))
	{
		if (0 == strcmp(addr1, addr2))
		{
			ret = true;
		}
	}
	else
	{
		for (j = 0, i = 0; i < strlen(addr1) + 1; i++)
		{
			if (addr1[i] == '0' && tmpAddr1[j] == '.')
				continue;
			tmpAddr1[++j] = addr1[i];
		}
		
		for (j = 0, i = 0; i < strlen(addr2) + 1; i++)
		{
			if (addr2[i] == '0' && tmpAddr2[j] == '.')
				continue;
			tmpAddr2[++j] = addr2[i];
		}
		
		if (0 == strcmp(tmpAddr1, tmpAddr2))
		{
			ret = true;
		}
	}
	
	return ret;
}

char* netAddrToStr(uint64_t netAddr)
{
	struct sockaddr_in addr = {0};
	addr.sin_addr.s_addr = netAddr;
	char *str;
	str = inet_ntoa(addr.sin_addr);
	return str;
}

uint64_t strToNetAddr(const char *p_str)
{
	char str[36] = {0};
	strncpy(str, p_str, 36);
	struct in_addr addr = {0};
	if(inet_pton(AF_INET, str, (void *)&addr) != 1)
	{
		printf("fun:%s, line:%d, failed!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	return addr.s_addr;
}

