
//-----------------------------------------

void mqtt_callback(char* topic, char* data, int topiclen, int datalen)
{
    char *top = (char *) calloc(1,topiclen+2);
    char *dat = (char *) calloc(1,datalen+2);
    memcpy(top,topic,topiclen);
    memcpy(dat,data,datalen);

   // printf("TOPIC %s\n",top);
    ESP_LOGI(TAG,"MQTT << %s",dat);
    cJSON *rcv_json = cJSON_Parse(dat);
    char *command = (char *)calloc(1,15);

    JSON_getstring(rcv_json,"com", command); 
    if (strcmp(command,"message")==0) {
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(dat);
    }
    
    command_transfer(rcv_json, SEND_MQTT);
    free(command);
    cJSON_Delete(rcv_json);
    free(top);
    free(dat);
}

