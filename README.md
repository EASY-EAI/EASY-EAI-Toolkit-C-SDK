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
> 	  * [update] camera (defalut output BGR888)
> 	  * [update] display (defalut input BGR888)
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
