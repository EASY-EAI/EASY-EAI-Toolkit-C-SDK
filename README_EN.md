<br/>
<br/>


[中文](README_CN.md)

<br />
<br />

If you first come into contact with this project. You can do:  
[reference the Quick Start Guide](https://www.easy-eai.com/document_details/3/133)

If you have extensive experience in embedded C development. You can do:  
[update the application development environment first](https://www.easy-eai.com/document_details/3/135)  
[reference the API development manual](https://www.easy-eai.com/document_details/3/129)


How to use：  
[1] - Clone this Git Storage to Local   
[2] - Execute in the root directory of this repository: ./build.sh   
[3] - All libraries will be generated to:               ./easyeai-api



important update log:
---
> 2024-09-24 : 
> * Release easyeai-api-2.0.0
>   * compatibility：
> 	  * Ubuntu system firmware for versions 20240920 and later, using easyai-api-2.x.x
> 	  * Ubuntu system firmware for versions 20240227 and later, using easyai-api-1.x.x
> 	  * Incompatible buildroot firmware
>   * algorithm_api
> 	  * [update] body_pose_detect
> 	  * [update] car_detect
> 	  * [update] OCR
> 	  * [update] smoke_detect
>   * common_api
> 	  * [update] Inter-Process Communication
> 	  * [update] data check（Fixed the bug where md5 validation caused program crashes due to array out of bounds）
>   * media_api
> 	  * [update] codec
>   * netProtocol_api
> 	  * [update] http（Use Ubuntu's official libcurl）
> 	  * [update] rtsp (Fix bug where live555 Segmentation fault when URL is empty)
>   * peripheral_api
> 	  * [update] camera (Add data format configuration for USB camera)
>
> 2024-01-17 : 
> * Release easyeai-api-1.2.10
>   * media_api
> 	  * [update] stream media encode & decode (1，Solved the problem of uneven decoding output；2，Added the EOS flag；3，The decoding method for sending memory has added a reference frame rate)
>   * netProtocol_api
> 	  * [update] rtsp (1，Added feature: Configurable waiting time for stream timeout；2，Added H265 frame rate analysis)
>   * peripheral_api
> 	  * [update] audio (Improve audio library)
> 	  * [update] camera (1，Solved the memory leakage problem caused by frequent switching of ispServer；2，Opened mipicamera API, allowing it to open 2 RGB cameras)
> 	  * [update] display (Optimized the calling performance of the normal mode API group)
>
> 2023-09-07 : 
> * Release easyeai-api-1.1.0
>   * media_api
> 	  * [update] stream media encode & decode (1,add audio codec interface; 2, Add video encoding interface: zero copy submission & request for I frame)
>   * netProtocol_api
> 	  * [update] rtsp (Improve rtsp library)
>   * peripheral_api
> 	  * [update] audio (Rewrite using the alsa interface)
>
> 2023-09-06 : 
> * Release easyeai-api-1.0.2
>   * algorithm_api
> 	  * [new] ocr
>   * netProtocol_api
> 	  * [update] https: Built-in libcurl.a into libhttps.a (unconventional approach, which will need to be separated in the future)
>   * peripheral_api
> 	  * [update] wdt (Resolve the issue of watchdog failure caused by Ctrl+C stopping dog feeding)
>
> 2023-08-14 : 
> * Release easyeai-api-1.0.1
>   * peripheral_api
> 	  * [new] GPIO Easy to operate interface
> 	  * [new] UART Easy to operate interface
> 	  * [update] camera (Fix the issue of not being able to repeat the switch)
> 	  * [update] display (Screen start coordinates can be specified)
> 	  * [update] network (Add a SSID and Password interface for configuring wlan)
>
> 2023-03-31 : 
> * Release easyeai-api-1.0.0
>   * Adapting ubuntu firmware
>   * Incompatible buildroot firmware
>   * The sdk of the buildroot version is stored in the buildroot branch of this repository
>   * common_api can be referenced by algorithm_api, media_api, netProtocol_api, and peripheral_api(Remember to append the common_api library reference of the document when calling)
>
> 2023-01-09 : 
> * Release easyeai-api-0.2.2
>   * algorithm_api
> 	  * [new] body_pose_detect
> 	  * [new] face_landmark98
> 	  * [new] face_mask_judgement
> 	  * [new] face_pose_estimation
> 	  * [new] moving_detect
> 	  * [update] fire_detect
> 	  * [update] helmet_detect
> 	  * [update] person_detect
> 	  * [del] site_safety
>   * common_api
> 	  * [update] Linux system operation (move network parameter to peripheral_api)
>   * media_api
> 	  * [update] stream media encode & decode (Unbinding decoder and frame queue)
>   * netProtocol_api
> 	  * [update] rtsp (modify rtsp server: Input with hook function)
>   * peripheral_api
> 	  * [new] network
> 	  * [new] spi
> 	  * [del] socket
>
> 2022-11-02 : 
> * Release easyeai-api-0.2.1
>   * algorithm_api
> 	  * [new] fire_detect
> 	  * [new] site_safety
> 	  * [update] helmet_detect
> 	  * [update] person_detect
>   * common_api
> 	  * [new] log manager (print to file or terminal can be specified)
> 	  * [update] base64 (Fix the bug of memory leak)
> 	  * [update] font engine (can write chinese on picture)
> 	  * [update] ini wrapper (Put the dynamic library into tool chain)
> 	  * [update] json parser (Improve api and Fix the bug of memory leak)
> 	  * [update] Linux system operation (Fix some bug for IPC and Increase heartbeat mechanism)
>   * media_api
> 	  * [update] stream media encode & decode (Improve encoder)
>   * netProtocol_api
> 	  * [update] rtsp (add create rtsp server api)
>   * peripheral_api
> 	  * [new] iic
> 	  * [new] touch screen
> 	  * [new] watch dog timer
> 	  * [update] camera (default output BGR888)
> 	  * [update] display (default input BGR888)
> 	  * [update] pwm (more easy use)
>
> 2022-08-25 : 
> * Release easyeai-api-0.2.0
>   * common_api
> 	  * [new] font engine (can write chinese on picture)
> 	  * [update] Linux system operation (fixed some bugs for IPC)
> 	  * [update] json parser (support multithreading calls)
>   * media_api
> 	  * [update] stream media encode & decode
>   * netProtocol_api
> 	  * [update] rtsp (support tcp capture rtsp stream)
>   * peripheral_api
> 	  * [new] pwm
> 	  * [update] camera (can set output data format)
>
> 2022-05-24 : 
> * Release easyeai-api-0.1.3
>   * algorithm_api
> 	  * [new] helmet_detect
> 	  * [new] geometry
>   * common_api
> 	  * [update] Linux system operation
>   * media_api
> 	  * [update] stream media encode & decode
>   * peripheral_api
> 	  * [update] display
>
> 2022-04-13 : 
> * Release easyeai-api-0.1.2
>   * algorithm_api
> 	  * [new] face_detect
> 	  * [new] face_alignment
> 	  * [new] face_recognition
> 	  * [new] person_detect
> 	  * [new] self_learning
> 	  * [open] decode QRCode
>   * media_api
> 	  * [update] dedicated shared memory
>   * peripheral_api
> 	  * [new] audio
> 	  * [update] display
> 	  * [update] camera
>
> 2022-03-16 : 
> * Release easyeai-api-0.1.1
>   * media_api
> 	  * [new] bmp file operate
> 	  * [update] stream media encode & decode
>   * netProtocol_api
> 	  * [update] rtsp
>
> 2022-01-28 : 
> * Release easyeai-api-0.1.0
>   * peripheral_api
> 	  * [new] backlight
> 	  * [new] display
> 	  * [new] camera
> 	  * [new] socket
>   * common_api
> 	  * [new] string operation
> 	  * [new] Linux system operation
> 	  * [new] base64 encode & decode
> 	  * [new] data check
> 	  * [new] encode QRCode
> 	  * [new] json parser
> 	  * [new] *.ini file operation
>   * media_api
> 	  * [new] dedicated shared memory operation(H.264、H.265、AAC)
> 	  * [new] stream media encode & decode
>   * netProtocol_api
> 	  * [new] https
> 	  * [new] rtsp
>   * algorithm_api
> 	  * [new] decode QRCode
>
> 2021-11-01 : 
> * create this project
