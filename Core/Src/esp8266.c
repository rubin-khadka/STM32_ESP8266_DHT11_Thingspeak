/*
 * esp8266.c
 *
 *  Created on: Mar 7, 2026
 *      Author: Rubin Khadka
 */

#include "esp8266.h"
#include <stdio.h>
#include <string.h>

ESP8266_ConnectionState ESP_ConnState = ESP8266_DISCONNECTED;
static char esp_rx_buffer[2048];
static ESP8266_Status ESP_GetIP(char *ip_buffer, uint16_t buffer_len);
static ESP8266_Status ESP_SendCommand(const char *cmd, const char *ack, uint32_t timeout);

ESP8266_Status ESP_Init(void)
{
  ESP8266_Status res;
  HAL_Delay(1000);

  res = ESP_SendCommand("AT\r\n", "OK", 2000);
  if(res != ESP8266_OK)
  {
    return res;
  }

  res = ESP_SendCommand("ATE0\r\n", "OK", 2000); // Disable echo
  if(res != ESP8266_OK)
  {
    return res;
  }
  return ESP8266_OK;
}

ESP8266_Status ESP_ConnectWiFi(const char *ssid, const char *password, char *ip_buffer, uint16_t buffer_len)
{
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "AT+CWMODE=1\r\n");

  ESP8266_Status result = ESP_SendCommand(cmd, "OK", 2000); // wait up to 2s
  if(result != ESP8266_OK)
  {
    return result;
  }

  printf("Connecting to WIFI: %s...\n", ssid);
  // Send join command
  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

  result = ESP_SendCommand(cmd, "WIFI CONNECTED", 10000); // wait up to 10s
  if(result != ESP8266_OK)
  {
    printf("WiFi connection failed\n");
    ESP_ConnState = ESP8266_NOT_CONNECTED;
    return result;
  }

  printf("Connected to WiFi !!!\n");
  ESP_ConnState = ESP8266_CONNECTED_NO_IP;
  // Fetch IP with retries inside ESP_GetIP
  result = ESP_GetIP(ip_buffer, buffer_len);
  if(result != ESP8266_OK)
  {
    return result;
  }

  printf("WiFi + IP ready: %s\n", ip_buffer);
  return ESP8266_OK;
}

ESP8266_ConnectionState ESP_GetConnectionState(void)
{
  return ESP_ConnState;
}

static ESP8266_Status ESP_GetIP(char *ip_buffer, uint16_t buffer_len)
{
  for(int attempt = 1; attempt <= 3; attempt++)
  {
    ESP8266_Status result = ESP_SendCommand("AT+CIFSR\r\n", "OK", 5000);
    if(result != ESP8266_OK)
    {
      continue;
    }

    char *search = esp_rx_buffer;
    char *last_ip = NULL;

    while((search = strstr(search, "STAIP,")) != NULL)
    {
      char *ip_start = strstr(search, "STAIP,\"");
      if(ip_start)
      {
        ip_start += 7;
        char *end = strchr(ip_start, '"');
        if(end && ((end - ip_start) < buffer_len))
        {
          last_ip = ip_start;
        }
      }
      search += 6;
    }

    if(last_ip)
    {
      char *end = strchr(last_ip, '"');
      strncpy(ip_buffer, last_ip, end - last_ip);
      ip_buffer[end - last_ip] = '\0';

      if(strcmp(ip_buffer, "0.0.0.0") == 0)
      {
        ESP_ConnState = ESP8266_CONNECTED_NO_IP;
        HAL_Delay(1000);
        continue;
      }

      ESP_ConnState = ESP8266_CONNECTED_IP;
      return ESP8266_OK;
    }

    HAL_Delay(500);
  }

  ESP_ConnState = ESP8266_CONNECTED_NO_IP;  // still connected, but no IP
  return ESP8266_ERROR;
}

static ESP8266_Status ESP_SendCommand(const char *cmd, const char *ack, uint32_t timeout)
{
  uint8_t ch;
  uint16_t idx = 0;
  uint32_t tickstart;
  int found = 0;

  memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
  tickstart = HAL_GetTick();

  if(strlen(cmd) > 0)
  {
    if(HAL_UART_Transmit(&ESP_UART, (uint8_t*) cmd, strlen(cmd), HAL_MAX_DELAY) != HAL_OK)
      return ESP8266_ERROR;
  }

  while((HAL_GetTick() - tickstart) < timeout && idx < sizeof(esp_rx_buffer) - 1)
  {
    if(HAL_UART_Receive(&ESP_UART, &ch, 1, 10) == HAL_OK)
    {
      esp_rx_buffer[idx++] = ch;
      esp_rx_buffer[idx] = '\0';

      // check for ACK
      if(!found && strstr(esp_rx_buffer, ack))
      {
        found = 1; // mark as found but keep reading
      }

      // handle busy response
      if(strstr(esp_rx_buffer, "busy"))
      {
        HAL_Delay(1500);
        idx = 0;
        memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
        continue;
      }
    }
  }

  if(found)
  {
    return ESP8266_OK;
  }

  if(idx == 0)
    return ESP8266_NO_RESPONSE;

  return ESP8266_TIMEOUT;
}
