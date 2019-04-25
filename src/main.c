#include <wups.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <coreinit/systeminfo.h>
#include <nsysnet/socket.h>
#include <vpad/input.h>
#include <utils/logger.h>

#define TARGET_WIDTH (854)
#define TARGET_HEIGHT (480)
static int log_socket = -1;
static struct sockaddr_in connect_addr;

WUPS_PLUGIN_NAME("Vpad input display");
WUPS_PLUGIN_DESCRIPTION("Sends inputs over the network");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("GPL");

ON_APPLICATION_START(args) {
    socket_lib_init();
    int broadcastEnable = 1;
    log_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (log_socket >= 0)
    {
        setsockopt(log_socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

        memset(&connect_addr, 0, sizeof(struct sockaddr_in));
        connect_addr.sin_family = AF_INET;
        connect_addr.sin_port = 4406;
        connect_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    }
}
void send_data(const char *str) {
    // socket is always 0 initially as it is in the BSS
    if(log_socket < 0) {
        return;
    }


    int len = strlen(str);
    int ret;
    while (len > 0) {
        int block = len < 1400 ? len : 1400; // take max 1400 bytes per UDP packet
        ret = sendto(log_socket, str, block, 0, (struct sockaddr *)&connect_addr, sizeof(struct sockaddr_in));
        if(ret < 0)
            break;

        len -= ret;
        str += ret;
    }
}
void send_dataf(const char *format, ...) {
    if(log_socket < 0) {
        return;
    }

    char tmp[512];
    tmp[0] = 0;

    va_list va;
    va_start(va, format);
    if((vsprintf(tmp, format, va) >= 0)) {
        send_data(tmp);
    }
    va_end(va);
}
uint8_t counter = 0;

DECL_FUNCTION(int32_t, VPADRead, int32_t chan, VPADStatus *buffer, uint32_t buffer_size, int32_t *error) {
    int32_t result = real_VPADRead(chan, buffer, buffer_size, error);
    
    if(result > 0 && OSIsHomeButtonMenuEnabled()){
                                                      
        VPADTouchData tpCalib;
        VPADGetTPCalibratedPoint(0, &tpCalib, &buffer->tpNormal);
        float tpXPos = (((float)tpCalib.x / 1280.0f) * (float)TARGET_WIDTH);
        float tpYPos = (float)TARGET_HEIGHT - (((float)tpCalib.y / 720.0f) * (float)TARGET_HEIGHT);
        BOOL tpTouched = tpCalib.touched;

        if(tpTouched){
            send_dataf("Touch x: %-+3.2f y: %-+3.2f touched %d\n",tpXPos,tpYPos,tpTouched);
        }
        send_dataf("Input %i %f %f  %f %f\n",buffer->hold,buffer->leftStick.x,buffer->leftStick.y,buffer->rightStick.x,buffer->rightStick.y);


    }
    return result;
}



WUPS_MUST_REPLACE(VPADRead,     WUPS_LOADER_LIBRARY_VPAD,       VPADRead);
