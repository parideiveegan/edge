#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init_sta(void);
bool wifi_is_connected(void);   // optional helper

#ifdef __cplusplus
}
#endif